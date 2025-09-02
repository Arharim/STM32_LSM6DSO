#include "stm32f10x.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

// SPI Chip Select Pin (PA4)
#define CS_PIN_HIGH() (GPIOA->BSRR = BIT_MASK(4U))
#define CS_PIN_LOW()  (GPIOA->BSRR = (BIT_MASK(4U) << 16U))

#define LSM6DSO_STABILIZATION_MS     (100U)
#define LSM6DSO_RETRY_DELAY_MS       (10U)
#define UART_TX_BUFFER_SIZE          (256U)
#define SPI_BURST_READ_LEN           (6U)
#define MAX_INIT_RETRIES             (3U)
#define FORMAT_BUFFER_SIZE           (128U)
#define GPIO_SPEED_2MHZ              (2U)
#define SPI_DUMMY_BYTE               (0xFFU)
#define UART_DUMMY_BYTE              (0x00U)
#define DELAY_LOOP_COUNT             (10U)
#define TIMEOUT_ZERO                 (0U)
#define ARRAY_INDEX_ZERO             (0U)
#define ARRAY_INDEX_ONE              (1U)
#define ARRAY_INDEX_TWO              (2U)
#define ARRAY_INDEX_THREE            (3U)
#define ARRAY_INDEX_FOUR             (4U)
#define ARRAY_INDEX_FIVE             (5U)
#define SHIFT_8_BITS                 (8U)
#define SHIFT_16_BITS                (16U)
#define OUTPUT_FORMAT_CSV            (1U)
#define LSM6DSO_ACC_mg_X1000_PER_LSB (61)

// Error Codes
typedef enum {
	ERROR_NONE = 0,
	ERROR_SPI_TIMEOUT = -1,
	ERROR_INVALID_PARAM = -2,
	ERROR_INIT_FAILED = -3,
	ERROR_CHIP_ID_MISMATCH = -4
} ErrorCode_t;

// FSM States
typedef enum {
	STATE_INIT,        // Init hardware and gyroscope
	STATE_STABILIZING, // New state for non-blocking delay
	STATE_READ,        // Read sensor data
	STATE_PROCESS,     // Process data into str
	STATE_OUTPUT,      // Output data via UART
	STATE_IDLE,        // Wait for timer interrupt
	STATE_ERROR        // Error state for recovery or halt
} State_t;

typedef struct {
	volatile State_t current_state;
	volatile s16 gyro_x;
	volatile s16 gyro_y;
	volatile s16 gyro_z;
	volatile s16 accel_x;
	volatile s16 accel_y;
	volatile s16 accel_z;
	u8 retry_count;
	u32 stabilization_start_time;
	u32 last_retry_time;
	volatile u32 system_tick;
} SystemState_t;

typedef struct {
	volatile char buffer[UART_TX_BUFFER_SIZE];
	volatile u16 write_idx;
	volatile u16 read_idx;
} UartBuffer_t;

// Global instances - organized and minimal
static SystemState_t g_system = {STATE_INIT, 0, 0, 0, 0, 0, 0, 0U, 0U, 0U, 0U};
static UartBuffer_t g_uart_tx = {{0}, 0U, 0U};

// Static buffers to avoid stack allocation in frequently called functions
static char g_format_buffer[FORMAT_BUFFER_SIZE]; // For snprintf operations
static u8 g_spi_buffer[SPI_BURST_READ_LEN];      // For SPI burst reads

static ErrorCode_t read_gyro_data(void);
static void set_state(State_t new_state);
static ErrorCode_t read_gyro_data(void);
static void set_state(State_t new_state);
static void handle_init_state(void);
static void handle_stabilizing_state(void);
static void handle_read_state(void);
static void handle_process_state(void);
static void handle_output_state(void);
static bool format_gyro_text(s32 x100, s32 y100, s32 z100);
static bool format_gyro_csv(s32 x100, s32 y100, s32 z100);
static void handle_error_state(void);
static void clear_spi_rx_buffer(void);
static bool is_uart_enabled(void);
static bool is_timer_running(u32 timer_base);
static ErrorCode_t read_accel_data(void);
static bool format_imu_csv(s32 gx100, s32 gy100, s32 gz100, s32 ax_mg,
                           s32 ay_mg, s32 az_mg);
static void print_csv_header_once(void);

void SysTick_Handler(void) { g_system.system_tick++; }

u32 get_system_tick(void) {
	u32 tick_value;
	__disable_irq();
	tick_value = g_system.system_tick;
	__enable_irq();
	return tick_value;
}

void TIM2_IRQHandler(void) {
	if ((TIM2->SR & TIM_SR_UIF) != 0U) {
		TIM2->SR &= ~TIM_SR_UIF;
		if (g_system.current_state == STATE_IDLE) {
			g_system.current_state = STATE_READ;
		}
	}
}

// UART Interrupt Service Routine
void USART1_IRQHandler(void) {
	const u32 sr = USART1->SR;

	if (((USART1->CR1 & USART_CR1_TXEIE) != 0U) && ((sr & USART_SR_TXE) != 0U)) {
		if (g_uart_tx.read_idx != g_uart_tx.write_idx) {
			USART1->DR = (u16)g_uart_tx.buffer[g_uart_tx.read_idx];
			g_uart_tx.read_idx = (g_uart_tx.read_idx + 1U) % UART_TX_BUFFER_SIZE;
		} else {
			USART1->CR1 &= ~USART_CR1_TXEIE;
		}
	}

	if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U) {
		const volatile u32 dummy = USART1->DR;
		(void)dummy;
		// Error counters could be incremented here
	}

	if ((sr & USART_SR_RXNE) != 0U) {
		const volatile u8 received_data = (u8)USART1->DR;
		(void)received_data; // Process received data here if needed
	}
}

static void clear_spi_rx_buffer(void) {
	while ((SPI1->SR & SPI_SR_RXNE) != 0U) {
		const volatile u8 dummy = (u8)SPI1->DR;
		(void)dummy;
	}
}

static bool is_uart_enabled(void) {
	return ((USART1->CR1 & USART_CR1_UE) != 0U);
}

static bool is_timer_running(const u32 timer_base) {
	bool is_running = false;

	if (timer_base == (u32)TIM2) {
		is_running = ((TIM2->CR1 & TIM_CR1_CEN) != 0U);
	} else if (timer_base == (u32)TIM3) {
		is_running = ((TIM3->CR1 & TIM_CR1_CEN) != 0U);
	} else {
		// Invalid timer base
	}

	return is_running;
}

// Use static buffer to avoid stack allocation
static ErrorCode_t read_gyro_data(void) {
	ErrorCode_t result = ERROR_NONE;

	if (spi_read_burst(LSM6DSO_OUTX_L_G, g_spi_buffer, SPI_BURST_READ_LEN) != 0) {
		result = ERROR_SPI_TIMEOUT;
	} else {
		g_system.gyro_x =
		    (s16)(((s16)g_spi_buffer[ARRAY_INDEX_ONE] << SHIFT_8_BITS) |
		          (s16)g_spi_buffer[ARRAY_INDEX_ZERO]);
		g_system.gyro_y =
		    (s16)(((s16)g_spi_buffer[ARRAY_INDEX_THREE] << SHIFT_8_BITS) |
		          (s16)g_spi_buffer[ARRAY_INDEX_TWO]);
		g_system.gyro_z =
		    (s16)(((s16)g_spi_buffer[ARRAY_INDEX_FIVE] << SHIFT_8_BITS) |
		          (s16)g_spi_buffer[ARRAY_INDEX_FOUR]);
	}

	return result;
}

static ErrorCode_t read_accel_data(void) {
	ErrorCode_t result = ERROR_NONE;

	if (spi_read_burst(LSM6DSO_OUTX_L_XL, g_spi_buffer, SPI_BURST_READ_LEN) !=
	    0) {
		result = ERROR_SPI_TIMEOUT;
	} else {
		g_system.accel_x =
		    (s16)(((s16)g_spi_buffer[ARRAY_INDEX_ONE] << SHIFT_8_BITS) |
		          (s16)g_spi_buffer[ARRAY_INDEX_ZERO]);
		g_system.accel_y =
		    (s16)(((s16)g_spi_buffer[ARRAY_INDEX_THREE] << SHIFT_8_BITS) |
		          (s16)g_spi_buffer[ARRAY_INDEX_TWO]);
		g_system.accel_z =
		    (s16)(((s16)g_spi_buffer[ARRAY_INDEX_FIVE] << SHIFT_8_BITS) |
		          (s16)g_spi_buffer[ARRAY_INDEX_FOUR]);
	}

	return result;
}

// Helper function to change state safely
static void set_state(const State_t new_state) {
	__disable_irq();
	g_system.current_state = new_state;
	__enable_irq();
}

static void handle_init_state(void) {
	g_system.retry_count = MAX_INIT_RETRIES;
	init_clocks();
	init_gpio();
	init_spi();
	init_uart();
	init_timer();

	// Configure LSM6DSO: Gyroscope + Accelerometer with HPF to remove gravity
	if ((spi_write(LSM6DSO_CTRL2_G, LSM6DSO_GYRO_208HZ_2000DPS) != 0) ||
	    (spi_write(LSM6DSO_CTRL1_XL, LSM6DSO_ACC_208HZ_2G) != 0) ||
	    (spi_write(LSM6DSO_CTRL8_XL, LSM6DSO_XL_HP_EN) != 0) ||
	    (spi_write(LSM6DSO_CTRL3_C, (LSM6DSO_BDU_ENABLE | 0x04U)) != 0)) {
		set_state(STATE_ERROR);
		return;
	}

	g_system.stabilization_start_time = get_system_tick();
	g_system.last_retry_time = get_system_tick();
	set_state(STATE_STABILIZING);
}

static void handle_stabilizing_state(void) {
	const u32 current_tick = get_system_tick();

	if ((current_tick - g_system.stabilization_start_time) <
	    LSM6DSO_STABILIZATION_MS) {
		return;
	}

	if ((g_system.retry_count > 0U) &&
	    ((current_tick - g_system.last_retry_time) >= LSM6DSO_RETRY_DELAY_MS)) {

		u8 chip_id = 0U;
		if (spi_read(LSM6DSO_WHO_AM_I, &chip_id) != 0) {
			set_state(STATE_ERROR);
			return;
		}

		if (chip_id == LSM6DSO_EXPECTED_ID) {
			set_state(STATE_IDLE);
		} else {
			g_system.retry_count--;
			g_system.last_retry_time = current_tick;

			const int snprintf_result =
			    snprintf(g_format_buffer, sizeof(g_format_buffer),
			             "ERR:ID 0x%02X, retries left: %u\r\n", (unsigned int)chip_id,
			             (unsigned int)g_system.retry_count);

			if ((snprintf_result > 0) &&
			    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
				uart_puts(g_format_buffer);
			}

			if (g_system.retry_count == 0U) {
				set_state(STATE_ERROR);
			}
		}
	}
}

static void handle_read_state(void) {
	u8 status = 0U;
	if (spi_read(LSM6DSO_STATUS_REG, &status) != 0) {
		set_state(STATE_ERROR);
		return;
	}

	if ((status & (LSM6DSO_STATUS_GDA | LSM6DSO_STATUS_XLDA)) == 0U) {
		set_state(STATE_IDLE);
		return;
	}

	ErrorCode_t r1 = read_gyro_data();
	ErrorCode_t r2 = read_accel_data();
	if ((r1 != ERROR_NONE) || (r2 != ERROR_NONE)) {
		set_state(STATE_ERROR);
	} else {
		set_state(STATE_PROCESS);
	}
}

static bool format_gyro_text(s32 x100, s32 y100, s32 z100) {
	char sx = (x100 < 0) ? '-' : '+';
	char sy = (y100 < 0) ? '-' : '+';
	char sz = (z100 < 0) ? '-' : '+';

	u32 xabs = (u32)((x100 < 0) ? -x100 : x100);
	u32 yabs = (u32)((y100 < 0) ? -y100 : y100);
	u32 zabs = (u32)((z100 < 0) ? -z100 : z100);

	u32 xi = xabs / 100U, xf = xabs % 100U;
	u32 yi = yabs / 100U, yf = yabs % 100U;
	u32 zi = zabs / 100U, zf = zabs % 100U;

	const int snprintf_result =
	    snprintf(g_format_buffer, sizeof(g_format_buffer),
	             "G[dps] X:%c%lu.%02lu Y:%c%lu.%02lu Z:%c%lu.%02lu\r\n", sx,
	             (unsigned long)xi, (unsigned long)xf, sy, (unsigned long)yi,
	             (unsigned long)yf, sz, (unsigned long)zi, (unsigned long)zf);

	return ((snprintf_result > 0) &&
	        ((size_t)snprintf_result < sizeof(g_format_buffer)));
}

static bool format_gyro_csv(s32 x100, s32 y100, s32 z100) {
	char sx = (x100 < 0) ? '-' : '+';
	char sy = (y100 < 0) ? '-' : '+';
	char sz = (z100 < 0) ? '-' : '+';

	u32 xabs = (u32)((x100 < 0) ? -x100 : x100);
	u32 yabs = (u32)((y100 < 0) ? -y100 : y100);
	u32 zabs = (u32)((z100 < 0) ? -z100 : z100);

	u32 xi = xabs / 100U, xf = xabs % 100U;
	u32 yi = yabs / 100U, yf = yabs % 100U;
	u32 zi = zabs / 100U, zf = zabs % 100U;

	const int snprintf_result =
	    snprintf(g_format_buffer, sizeof(g_format_buffer),
	             "%c%lu.%02lu,%c%lu.%02lu,%c%lu.%02lu\r\n", sx, (unsigned long)xi,
	             (unsigned long)xf, sy, (unsigned long)yi, (unsigned long)yf, sz,
	             (unsigned long)zi, (unsigned long)zf);

	return ((snprintf_result > 0) &&
	        ((size_t)snprintf_result < sizeof(g_format_buffer)));
}

static bool format_imu_csv(s32 gx100, s32 gy100, s32 gz100, s32 ax_mg,
                           s32 ay_mg, s32 az_mg) {
	char sgx = (gx100 < 0) ? '-' : '+';
	char sgy = (gy100 < 0) ? '-' : '+';
	char sgz = (gz100 < 0) ? '-' : '+';

	u32 gxabs = (u32)((gx100 < 0) ? -gx100 : gx100);
	u32 gyabs = (u32)((gy100 < 0) ? -gy100 : gy100);
	u32 gzabs = (u32)((gz100 < 0) ? -gz100 : gz100);

	u32 gxi = gxabs / 100U, gxf = gxabs % 100U;
	u32 gyi = gyabs / 100U, gyf = gyabs % 100U;
	u32 gzi = gzabs / 100U, gzf = gzabs % 100U;

	char sax = (ax_mg < 0) ? '-' : '+';
	char say = (ay_mg < 0) ? '-' : '+';
	char saz = (az_mg < 0) ? '-' : '+';

	u32 axabs = (u32)((ax_mg < 0) ? -ax_mg : ax_mg);
	u32 ayabs = (u32)((ay_mg < 0) ? -ay_mg : ay_mg);
	u32 azabs = (u32)((az_mg < 0) ? -az_mg : az_mg);

	u32 axi = axabs / 1000U, axf = axabs % 1000U;
	u32 ayi = ayabs / 1000U, ayf = ayabs % 1000U;
	u32 azi = azabs / 1000U, azf = azabs % 1000U;

	const int snprintf_result = snprintf(
	    g_format_buffer, sizeof(g_format_buffer),
	    "%c%lu.%02lu,%c%lu.%02lu,%c%lu.%02lu,%c%lu.%03lu,%c%lu.%03lu,%c%lu.%"
	    "03lu\r\n",
	    sgx, (unsigned long)gxi, (unsigned long)gxf, sgy, (unsigned long)gyi,
	    (unsigned long)gyf, sgz, (unsigned long)gzi, (unsigned long)gzf, sax,
	    (unsigned long)axi, (unsigned long)axf, say, (unsigned long)ayi,
	    (unsigned long)ayf, saz, (unsigned long)azi, (unsigned long)azf);

	return ((snprintf_result > 0) &&
	        ((size_t)snprintf_result < sizeof(g_format_buffer)));
}

static void print_csv_header_once(void) {
	static bool header_printed = false;
	if (!header_printed) {
		const int snprintf_result =
		    snprintf(g_format_buffer, sizeof(g_format_buffer),
		             "gx_dps,gy_dps,gz_dps,ax_g,ay_g,az_g\r\n");
		if ((snprintf_result > 0) &&
		    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
			uart_puts(g_format_buffer);
		}
		header_printed = true;
	}
}

static void handle_process_state(void) {
	// Convert raw LSB to dps with FS=2000 dps: 0.07 dps/LSB -> dps*100 = raw*7
	s32 x100 = (s32)g_system.gyro_x * (s32)LSM6DSO_GYRO_DPSx100_PER_LSB;
	s32 y100 = (s32)g_system.gyro_y * (s32)LSM6DSO_GYRO_DPSx100_PER_LSB;
	s32 z100 = (s32)g_system.gyro_z * (s32)LSM6DSO_GYRO_DPSx100_PER_LSB;

	bool ok;
#if OUTPUT_FORMAT_CSV
	print_csv_header_once();
	s32 ax_uG = (s32)g_system.accel_x * LSM6DSO_ACC_mg_X1000_PER_LSB; // micro-g
	s32 ay_uG = (s32)g_system.accel_y * LSM6DSO_ACC_mg_X1000_PER_LSB;
	s32 az_uG = (s32)g_system.accel_z * LSM6DSO_ACC_mg_X1000_PER_LSB;
	// Convert micro-g to g*1000 with sign-aware rounding: g*1000 = (uG)/1000
	s32 ax_gx1000 = (ax_uG >= 0) ? ((ax_uG + 500) / 1000) : ((ax_uG - 500) / 1000);
	s32 ay_gx1000 = (ay_uG >= 0) ? ((ay_uG + 500) / 1000) : ((ay_uG - 500) / 1000);
	s32 az_gx1000 = (az_uG >= 0) ? ((az_uG + 500) / 1000) : ((az_uG - 500) / 1000);
	ok = format_imu_csv(x100, y100, z100, ax_gx1000, ay_gx1000, az_gx1000);
#else
	ok = format_gyro_text(x100, y100, z100);
#endif

	if (ok) {
		set_state(STATE_OUTPUT);
	} else {
		set_state(STATE_ERROR);
	}
}

static void handle_output_state(void) {
	uart_puts(g_format_buffer);
	set_state(STATE_IDLE);
}

static void handle_error_state(void) {
	uart_puts("System halted due to error\r\n");
	while (1) {
		__NOP();
	}
}

int main(void) {
	g_system.current_state = STATE_INIT;

	while (1) {
		switch (g_system.current_state) {
		case STATE_INIT:
			handle_init_state();
			break;
		case STATE_STABILIZING:
			handle_stabilizing_state();
			break;
		case STATE_READ:
			handle_read_state();
			break;
		case STATE_PROCESS:
			handle_process_state();
			break;
		case STATE_OUTPUT:
			handle_output_state();
			break;
		case STATE_IDLE:
			__WFI();
			break;
		case STATE_ERROR:
			handle_error_state();
			break;
		default:
			set_state(STATE_ERROR);
			break;
		}
	}
}

// Initialize peripheral clocks
void init_clocks(void) {
	RCC->CR |= RCC_CR_HSION;
	while ((RCC->CR & RCC_CR_HSIRDY) == 0U) { /* Wait for HSI */
	}

	RCC->CFGR = 0x00000000UL;
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
	RCC->CR &= ~RCC_CR_HSEBYP;
	RCC->CIR = 0x00000000UL;
	// RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
	// RCC->CIR = 0x009F0000;

	RCC->CR |= RCC_CR_HSEON;
	u32 timeout = HSE_STARTUP_TIMEOUT;
	while (((RCC->CR & RCC_CR_HSERDY) == 0U) && (timeout > 0U)) {
		timeout--;
	}

	if ((RCC->CR & RCC_CR_HSERDY) == 0U) {
		set_state(STATE_ERROR); // HSE failed to start
		return;
	}

	FLASH->ACR |= FLASH_ACR_PRFTBE;
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_0;

	RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
	RCC->CFGR |= RCC_CFGR_PLLSRC;
	RCC->CFGR |= RCC_CFGR_PLLMULL3;

	RCC->CR |= RCC_CR_PLLON;
	timeout = PLL_LOCK_TIMEOUT;
	while (((RCC->CR & RCC_CR_PLLRDY) == 0U) && (timeout > 0U)) {
		timeout--;
	}

	if ((RCC->CR & RCC_CR_PLLRDY) == 0U) {
		set_state(STATE_ERROR); // PLL failed to lock
		return;
	}

	// RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
	// RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
	// RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;
	// RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { /* Wait for switch */
	}

	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000U);

	RCC->APB2ENR |=
	    (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART1EN);
	RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN);
}

void init_gpio(void) {
	GPIOA->CRL &=
	    ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0 | GPIO_CRL_MODE1 | GPIO_CRL_CNF1 |
	      GPIO_CRL_MODE2 | GPIO_CRL_CNF2 | GPIO_CRL_MODE3 | GPIO_CRL_CNF3);
	GPIOA->CRL |=
	    (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1 | GPIO_CRL_CNF2_1 | GPIO_CRL_CNF3_1);

	GPIOA->CRH &=
	    ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8 | GPIO_CRH_MODE11 | GPIO_CRH_CNF11 |
	      GPIO_CRH_MODE12 | GPIO_CRH_CNF12 | GPIO_CRH_MODE13 | GPIO_CRH_CNF13 |
	      GPIO_CRH_MODE14 | GPIO_CRH_CNF14 | GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
	GPIOA->CRH |= (GPIO_CRH_CNF8_1 | GPIO_CRH_CNF11_1 | GPIO_CRH_CNF12_1 |
	               GPIO_CRH_CNF13_1 | GPIO_CRH_CNF14_1 | GPIO_CRH_CNF15_1);

	GPIOA->ODR &= ~(BIT_MASK(0U) | BIT_MASK(1U) | BIT_MASK(2U) | BIT_MASK(3U) |
	                BIT_MASK(8U) | BIT_MASK(11U) | BIT_MASK(12U) | BIT_MASK(13U) |
	                BIT_MASK(14U) | BIT_MASK(15U));

	// PA4 (CS): Output, 2 MHz speed, push-pull
	GPIOA->CRL &= ~(GPIO_CRL_MODE4 | GPIO_CRL_CNF4);
	GPIOA->CRL |= GPIO_CRL_MODE4_1; // 2MHz output
	GPIOA->ODR |= BIT_MASK(4U);     // Set CS high initially

	// PA5 (SCK): AF push-pull, 2MHz
	GPIOA->CRL &= ~(GPIO_CRL_MODE5 | GPIO_CRL_CNF5);
	GPIOA->CRL |= (GPIO_CRL_MODE5_1 | GPIO_CRL_CNF5_1);

	// PA6 (MISO): Input floating (required for SPI)
	GPIOA->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
	GPIOA->CRL |= GPIO_CRL_CNF6_0;

	// PA7 (MOSI): AF push-pull, 2MHz
	GPIOA->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
	GPIOA->CRL |= (GPIO_CRL_MODE7_1 | GPIO_CRL_CNF7_1);

	// PA9 (TX): AF push-pull, 2MHz
	GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
	GPIOA->CRH |= (GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1);

	// PA10 (RX): Input floating (required for UART)
	GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
	GPIOA->CRH |= GPIO_CRH_CNF10_0;

	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	GPIOB->CRL = 0x88888888UL;
	GPIOB->CRH = 0x88888888UL;
	GPIOB->ODR = 0x0000U;

	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	GPIOC->CRL = 0x88888888UL;
	GPIOC->CRH = 0x88888888UL;
	GPIOC->ODR = 0x0000U;
}

void init_spi(void) {
	RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;

	SPI1->CR1 &= ~SPI_CR1_SPE;

	SPI1->CR1 = 0UL;
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR1 |= SPI_CR1_BR_2; // Baud rate: fPCLK/32 (~750kHz at 24MHz) - safer
	                           // for LSM6DSO
	SPI1->CR1 |= SPI_CR1_CPOL; // Clock polarity: idle high (Mode 3)
	SPI1->CR1 |= SPI_CR1_CPHA; // Clock phase: capture on 2nd edge (Mode 3)
	SPI1->CR1 |= SPI_CR1_SSM;  // Software slave management
	SPI1->CR1 |= SPI_CR1_SSI;  // Internal slave select high

	SPI1->CR2 = 0UL;

	volatile u32 dummy = SPI1->DR;
	dummy = SPI1->SR;
	(void)dummy;

	SPI1->CR1 |= SPI_CR1_SPE;

	u32 timeout = 1000UL;
	while (((SPI1->CR1 & SPI_CR1_SPE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	CS_PIN_HIGH();
}

// Initialize UART1 for 115200 baud
void init_uart(void) {
	RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;

	USART1->CR1 &= ~USART_CR1_UE;

	USART1->BRR = UART_BRR;

	USART1->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);

	USART1->CR2 &= ~USART_CR2_STOP;

	USART1->CR3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);

	USART1->SR = 0UL;

	USART1->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_TCIE | USART_CR1_TXEIE |
	                 USART_CR1_PEIE | USART_CR1_IDLEIE);

	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);

	USART1->CR1 |= USART_CR1_UE;

	u32 timeout = 1000U;
	while (((USART1->SR & USART_SR_TC) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	const volatile u32 dummy_sr = USART1->SR;
	USART1->DR = UART_DUMMY_BYTE;
	(void)dummy_sr;

	NVIC_SetPriority(USART1_IRQn, 2U);
	NVIC_EnableIRQ(USART1_IRQn);

	if (((USART1->CR1 & USART_CR1_UE) == 0U) ||
	    ((USART1->CR1 & USART_CR1_TE) == 0U) || (USART1->BRR != UART_BRR)) {
		set_state(STATE_ERROR);
	}
}

// Initialize TIM2 for 100 Hz interrupts and TIM3 for delays
void init_timer(void) {
	u32 timeout;
	u16 initial_count;

	RCC->APB1RSTR |= (RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);
	RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);

	for (volatile int i = 0; i < 100; i++) {
		__NOP();
	}

	TIM2->CR1 = 0UL;
	TIM2->CR2 = 0UL;
	TIM2->SMCR = 0UL;
	TIM2->DIER = 0UL;
	TIM2->SR = 0UL;
	TIM2->CNT = 0UL;

	TIM2->PSC = TIM2_PSC;
	TIM2->ARR = TIM2_ARR;

	TIM2->EGR |= TIM_EGR_UG;

	TIM2->SR &= ~TIM_SR_UIF;

	TIM2->DIER |= TIM_DIER_UIE;

	NVIC_SetPriority(TIM2_IRQn, 3U);
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM2->CR1 |= TIM_CR1_CEN;

	timeout = TIMER_CONFIG_TIMEOUT;
	initial_count = TIM2->CNT;
	while ((TIM2->CNT == initial_count) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		const int snprintf_result =
		    snprintf(g_format_buffer, sizeof(g_format_buffer),
		             "ERR: TIM2 failed to start\r\n");
		if ((snprintf_result > 0) &&
		    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
			uart_puts(g_format_buffer);
		}
		set_state(STATE_ERROR);
		return;
	}

	TIM3->CR1 = 0UL;
	TIM3->CR2 = 0UL;
	TIM3->SMCR = 0UL;
	TIM3->DIER = 0UL;
	TIM3->SR = 0UL;
	TIM3->CNT = 0UL;

	TIM3->PSC = TIM3_PSC;
	TIM3->ARR = TIM3_ARR;

	TIM3->EGR |= TIM_EGR_UG;

	TIM3->SR &= ~TIM_SR_UIF;

	TIM3->CR1 |= TIM_CR1_CEN;

	timeout = TIMER_CONFIG_TIMEOUT;
	initial_count = TIM3->CNT;
	while ((TIM3->CNT == initial_count) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		const int snprintf_result =
		    snprintf(g_format_buffer, sizeof(g_format_buffer),
		             "ERR: TIM3 failed to start\r\n");
		if ((snprintf_result > 0) &&
		    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
			uart_puts(g_format_buffer);
		}
		set_state(STATE_ERROR);
		return;
	}

	// Optional: Print timer configuration for debugging
	const u32 tim2_freq =
	    SYSTEM_CLOCK_HZ / ((TIM2->PSC + 1UL) * (TIM2->ARR + 1UL));
	const u32 tim3_freq =
	    SYSTEM_CLOCK_HZ / ((TIM3->PSC + 1UL) * (TIM3->ARR + 1UL));

	const int snprintf_result =
	    snprintf(g_format_buffer, sizeof(g_format_buffer),
	             "Timers OK: TIM2=%luHz TIM3=%luHz\r\n", tim2_freq, tim3_freq);
	if ((snprintf_result > 0) &&
	    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
		uart_puts(g_format_buffer);
	}
}

// delay function using TIM3 (1 ms resolution, blocking) // use with caution !!!
void delay_ms(const u32 ms) {
	u32 timeout;

	if (ms == 0U) {
		return;
	}

	if (!is_timer_running((u32)TIM3)) {
		for (volatile u32 i = 0U; i < (ms * (SYSTEM_CLOCK_HZ / 1000UL)); i++) {
			__NOP();
		}
		return;
	}

	for (u32 i = 0U; i < ms; i++) {
		TIM3->CNT = 0UL;

		timeout = 10000U;
		while ((TIM3->CNT == 0U) && (timeout > 0U)) {
			timeout--;
			__NOP();
		}

		while ((TIM3->CNT < 1U) && (timeout > 0U)) {
			timeout--;
			__NOP();
		}

		if (timeout == 0U) {
			break;
		}
	}
}

// Non-blocking delay function using system tick
u8 delay_ms_nb(const u32 ms, u32 *const start_time) {
	u8 delay_complete = 0U;

	if (start_time == NULL) {
		return 0U;
	}

	if (*start_time == 0U) {
		*start_time = get_system_tick();
	} else if ((get_system_tick() - *start_time) >= ms) {
		*start_time = 0U;
		delay_complete = 1U;
	} else {
		// Delay not complete
	}

	return delay_complete;
}

void get_timer_status(void) {
	int snprintf_result =
	    snprintf(g_format_buffer, sizeof(g_format_buffer),
	             "TIM2: EN=%d CNT=%u PSC=%u ARR=%u\r\n",
	             is_timer_running((u32)TIM2) ? 1 : 0, (unsigned int)TIM2->CNT,
	             (unsigned int)TIM2->PSC, (unsigned int)TIM2->ARR);

	if ((snprintf_result > 0) &&
	    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
		uart_puts(g_format_buffer);
	}

	snprintf_result =
	    snprintf(g_format_buffer, sizeof(g_format_buffer),
	             "TIM3: EN=%d CNT=%u PSC=%u ARR=%u\r\n",
	             is_timer_running((u32)TIM3) ? 1 : 0, (unsigned int)TIM3->CNT,
	             (unsigned int)TIM3->PSC, (unsigned int)TIM3->ARR);

	if ((snprintf_result > 0) &&
	    ((size_t)snprintf_result < sizeof(g_format_buffer))) {
		uart_puts(g_format_buffer);
	}
}

// Write a single byte via SPI with timeout
int spi_write(const u8 reg, const u8 data) {
	u32 timeout;

	clear_spi_rx_buffer();

	CS_PIN_LOW();

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	SPI1->DR = (u16)reg;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	const volatile u8 dummy1 = (u8)SPI1->DR;
	(void)dummy1;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	SPI1->DR = (u16)data;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	const volatile u8 dummy2 = (u8)SPI1->DR;
	(void)dummy2;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}

	CS_PIN_HIGH();

	for (volatile int i = 0; i < DELAY_LOOP_COUNT; i++) {
		__NOP();
	}

	return ERROR_NONE;
}

// Read a single byte via SPI with timeout
int spi_read(const u8 reg, u8 *const data) {
	u32 timeout;

	if (data == NULL) {
		return ERROR_INVALID_PARAM;
	}

	clear_spi_rx_buffer();

	CS_PIN_LOW();

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	SPI1->DR = (u16)(reg | 0x80U);

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	(void)SPI1->DR;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	SPI1->DR = (u16)SPI_DUMMY_BYTE;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	*data = (u8)SPI1->DR;

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
		timeout--;
	}

	CS_PIN_HIGH();

	for (volatile int i = 0; i < DELAY_LOOP_COUNT; i++) {
		__NOP();
	}

	return ERROR_NONE;
}

// Read multiple bytes via SPI (burst mode) with timeout
int spi_read_burst(const u8 reg, u8 *const buffer, const u8 len) {
	u32 timeout;

	if ((buffer == NULL) || (len == 0U)) {
		return ERROR_INVALID_PARAM;
	}

	clear_spi_rx_buffer();

	CS_PIN_LOW();

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	SPI1->DR = (u16)(reg | 0x80U);

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		CS_PIN_HIGH();
		return ERROR_SPI_TIMEOUT;
	}
	const volatile u8 dummy = (u8)SPI1->DR;
	(void)dummy;

	for (u8 i = 0U; i < len; i++) {
		timeout = SPI_UART_TIMEOUT;
		while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
			timeout--;
			__NOP();
		}
		if (timeout == 0U) {
			CS_PIN_HIGH();
			return ERROR_SPI_TIMEOUT;
		}
		SPI1->DR = (u16)SPI_DUMMY_BYTE;

		timeout = SPI_UART_TIMEOUT;
		while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
			timeout--;
			__NOP();
		}
		if (timeout == 0U) {
			CS_PIN_HIGH();
			return ERROR_SPI_TIMEOUT;
		}
		buffer[i] = (u8)SPI1->DR;
	}

	timeout = SPI_UART_TIMEOUT;
	while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	CS_PIN_HIGH();

	for (volatile int i = 0; i < DELAY_LOOP_COUNT; i++) {
		__NOP();
	}

	return ERROR_NONE;
}

// Send a string via UART using an interrupt-driven circular buffer
// (non-blocking)
void uart_puts(const char *const s) {
	size_t chars_added = 0U;
	const char *str_ptr = s;

	if ((s == NULL) || !is_uart_enabled()) {
		return;
	}

	__disable_irq();

	while (*str_ptr != '\0') {
		const u16 next_idx = (g_uart_tx.write_idx + 1U) % UART_TX_BUFFER_SIZE;

		if (next_idx == g_uart_tx.read_idx) {
			break;
		}

		g_uart_tx.buffer[g_uart_tx.write_idx] = *str_ptr;
		g_uart_tx.write_idx = next_idx;
		str_ptr++;
		chars_added++;
	}

	if ((chars_added > 0U) && ((USART1->CR1 & USART_CR1_TXEIE) == 0U)) {
		USART1->CR1 |= USART_CR1_TXEIE;
	}

	__enable_irq();
}