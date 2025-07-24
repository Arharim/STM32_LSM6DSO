#include <stdio.h>
// #include <stdbool.h>
#include "stm32f10x.h"
#include "utils.h"

// SPI Chip Select Pin (PA4)
#define CS_PIN_HIGH()   (GPIOA->BSRR = BIT(4U))
#define CS_PIN_LOW()    (GPIOA->BSRR = (BIT(4U) << 16U))

#define LSM6DSO_STABILIZATION_MS  (100U)
#define LSM6DSO_RETRY_DELAY_MS    (10U)
#define UART_TX_BUFFER_SIZE       (256U)
#define SPI_BURST_READ_LEN        (6U)
#define MAX_INIT_RETRIES          (3U)
#define FORMAT_BUFFER_SIZE        (64U)
#define GPIO_SPEED_2MHZ           (2U)
#define SPI_DUMMY_BYTE            (0xFFU)
#define UART_DUMMY_BYTE           (0x00U)
#define DELAY_LOOP_COUNT          (10U)
#define TIMEOUT_ZERO              (0U)
#define ARRAY_INDEX_ZERO          (0U)
#define ARRAY_INDEX_ONE           (1U)
#define ARRAY_INDEX_TWO           (2U)
#define ARRAY_INDEX_THREE         (3U)
#define ARRAY_INDEX_FOUR          (4U)
#define ARRAY_INDEX_FIVE          (5U)
#define SHIFT_8_BITS              (8U)
#define SHIFT_16_BITS             (16U)

// Error Codes
typedef enum 
{
    ERROR_NONE = 0,
    ERROR_SPI_TIMEOUT = -1,
    ERROR_INVALID_PARAM = -2,
    ERROR_INIT_FAILED = -3,
    ERROR_CHIP_ID_MISMATCH = -4
} ErrorCode_t;

// FSM States
typedef enum 
{
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
    u8 retry_count;
    u32 stabilization_start_time;
    u32 last_retry_time;
    volatile u32 system_tick;
} SystemState_t;

typedef struct 
{
    volatile char buffer[UART_TX_BUFFER_SIZE];
    volatile u16 write_idx;
    volatile u16 read_idx;
} UartBuffer_t;

// Global instances - organized and minimal
static SystemState_t g_system = {STATE_INIT, 0, 0, 0, 0U, 0U, 0U, 0U};
static UartBuffer_t g_uart_tx = {{0}, 0U, 0U};

// Static buffers to avoid stack allocation in frequently called functions
static char g_format_buffer[FORMAT_BUFFER_SIZE];   // For snprintf operations
static u8 g_spi_buffer[SPI_BURST_READ_LEN];        // For SPI burst reads

// static ErrorCode_t read_gyro_data(void);
// static void set_state(State_t new_state);static ErrorCode_t read_gyro_data(void);
// static void set_state(State_t new_state);
// static void handle_init_state(void);
// static void handle_stabilizing_state(void);
// static void handle_read_state(void);
// static void handle_process_state(void);
// static void handle_output_state(void);
// static void handle_error_state(void);
// static void clear_spi_rx_buffer(void);
// static bool is_uart_enabled(void);
// static bool is_timer_running(u32 timer_base);

void SysTick_Handler(void) 
{
    g_system.system_tick++;
}

u32 get_system_tick(void) 
{
    return g_system.system_tick;
}

void TIM2_IRQHandler(void) 
{
   if ((TIM2->SR & TIM_SR_UIF) != 0U) 
   {
       TIM2->SR &= ~TIM_SR_UIF;
       if (g_system.current_state == STATE_IDLE) 
       {
           g_system.current_state = STATE_READ;
       }
   }
}

// UART Interrupt Service Routine
void USART1_IRQHandler(void) 
{
    u32 sr = USART1->SR;
    
    if (((USART1->CR1 & USART_CR1_TXEIE) != 0U) && ((sr & USART_SR_TXE) != 0U)) {
        if (g_uart_tx.read_idx != g_uart_tx.write_idx) {
            USART1->DR = (u16)g_uart_tx.buffer[g_uart_tx.read_idx];
            g_uart_tx.read_idx = (g_uart_tx.read_idx + 1U) % UART_TX_BUFFER_SIZE;
        } else {
            USART1->CR1 &= ~USART_CR1_TXEIE;
        }
    }
    
    if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U) {
        volatile u32 dummy = USART1->DR;
        (void)dummy;
        
        // might want to increment error counters here
        // or take other recovery actions
    }
    
    if ((sr & USART_SR_RXNE) != 0) {
        volatile u8 received_data = (u8)USART1->DR;
        // Process received data here if needed
        (void)received_data; // Suppress warning if not used
    }
}

// Use static buffer to avoid stack allocation
static int read_gyro_data(void) 
{
   if (spi_read_burst(LSM6DSO_OUTX_L_G, g_spi_buffer, SPI_BURST_READ_LEN) != 0) {
       return -1;
   }
   g_system.gyro_x = (s16)(((s16)g_spi_buffer[1] << 8) | g_spi_buffer[0]);
   g_system.gyro_y = (s16)(((s16)g_spi_buffer[3] << 8) | g_spi_buffer[2]);
   g_system.gyro_z = (s16)(((s16)g_spi_buffer[5] << 8) | g_spi_buffer[4]);
   return 0;
}

// Helper function to change state safely
static inline void set_state(State_t new_state) 
{
    __disable_irq();
    g_system.current_state = new_state;
    __enable_irq();
}

static void handle_init_state(void) 
{
    g_system.retry_count = MAX_INIT_RETRIES;
    init_clocks();
    init_gpio();
    init_spi();
    init_uart();
    init_timer();

    // Configure LSM6DSO Gyroscope
    if ((spi_write(LSM6DSO_CTRL2_G, LSM6DSO_GYRO_CONFIG) != 0) || 
        (spi_write(LSM6DSO_CTRL3_C, LSM6DSO_CTRL3_BDU) != 0)) {
        set_state(STATE_ERROR);
        return;
    }

    g_system.stabilization_start_time = get_system_tick();
    g_system.last_retry_time = get_system_tick();
    set_state(STATE_STABILIZING);
}

static void handle_stabilizing_state(void) 
{
    if ((get_system_tick() - g_system.stabilization_start_time) < LSM6DSO_STABILIZATION_MS) {
        return;
    }

    if ((g_system.retry_count > 0U) && 
        ((get_system_tick() - g_system.last_retry_time) >= LSM6DSO_RETRY_DELAY_MS)) {
        
        u8 chip_id = 0U;
        if (spi_read(LSM6DSO_WHO_AM_I, &chip_id) != 0) {
            set_state(STATE_ERROR);
            return;
        }

        if (chip_id == LSM6DSO_ID_VAL) {
            set_state(STATE_IDLE);
        } else {
            g_system.retry_count--;
            g_system.last_retry_time = get_system_tick();
            
            (void)snprintf(g_format_buffer, sizeof(g_format_buffer),
                    "ERR:ID 0x%02X, retries left: %u\r\n", chip_id, g_system.retry_count);
            uart_puts(g_format_buffer);
            
            if (g_system.retry_count == 0U) {
                set_state(STATE_ERROR);
            }
        }
    }
}

static void handle_read_state(void) 
{
    if (read_gyro_data() != 0) {
        set_state(STATE_ERROR);
    } else {
        set_state(STATE_PROCESS);
    }
}

static void handle_process_state(void) 
{
    (void)snprintf(g_format_buffer, sizeof(g_format_buffer), 
            "X:%6d Y:%6d Z:%6d\r\n", 
            g_system.gyro_x, g_system.gyro_y, g_system.gyro_z);
    set_state(STATE_OUTPUT);
}

static void handle_output_state(void) 
{
    uart_puts(g_format_buffer);
    set_state(STATE_IDLE);
}

static void handle_error_state(void) 
{
    uart_puts("System halted due to error\r\n");
    while (1) {
        __NOP();
    }
}

int main(void) 
{
    g_system.current_state = STATE_INIT;
    
    while (1) 
    {
        switch (g_system.current_state) 
        {
            case STATE_INIT:        handle_init_state();        break;
            case STATE_STABILIZING: handle_stabilizing_state(); break;
            case STATE_READ:        handle_read_state();        break;
            case STATE_PROCESS:     handle_process_state();     break;
            case STATE_OUTPUT:      handle_output_state();      break;
            case STATE_IDLE:        __WFI();                    break;
            case STATE_ERROR:       handle_error_state();       break;
            default:
                set_state(STATE_ERROR);
                break;
        }
    }
}

// Initialize peripheral clocks
void init_clocks(void) 
{
    RCC->CR |= RCC_CR_HSION;
    while((RCC->CR & RCC_CR_HSIRDY) == 0U) { /* Wait for HSI */ }

    RCC->CFGR = 0x00000000;
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CIR = 0x00000000;    
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
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { /* Wait for switch */ }

    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);
    
    RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART1EN);
    RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN);
}

void init_gpio(void) 
{
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0 | 
                    GPIO_CRL_MODE1 | GPIO_CRL_CNF1 |
                    GPIO_CRL_MODE2 | GPIO_CRL_CNF2 |
                    GPIO_CRL_MODE3 | GPIO_CRL_CNF3);
    GPIOA->CRL |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1 | 
                   GPIO_CRL_CNF2_1 | GPIO_CRL_CNF3_1);
    
    GPIOA->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8 |
                    GPIO_CRH_MODE11 | GPIO_CRH_CNF11 |
                    GPIO_CRH_MODE12 | GPIO_CRH_CNF12 |
                    GPIO_CRH_MODE13 | GPIO_CRH_CNF13 |
                    GPIO_CRH_MODE14 | GPIO_CRH_CNF14 |
                    GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    GPIOA->CRH |= (GPIO_CRH_CNF8_1 | GPIO_CRH_CNF11_1 |
                   GPIO_CRH_CNF12_1 | GPIO_CRH_CNF13_1 |
                   GPIO_CRH_CNF14_1 | GPIO_CRH_CNF15_1);
    
    GPIOA->ODR &= ~(BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(8) | 
                    BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15));

    
    // PA4 (CS): Output, 2 MHz speed, push-pull
    GPIOA->CRL &= ~(GPIO_CRL_MODE4 | GPIO_CRL_CNF4);
    GPIOA->CRL |= GPIO_CRL_MODE4_1;  // 2MHz output
    GPIOA->ODR |= BIT(4);  // Set CS high initially

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
    GPIOB->CRL = 0x88888888;
    GPIOB->CRH = 0x88888888;
    GPIOB->ODR = 0x0000;

    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRL = 0x88888888;
    GPIOC->CRH = 0x88888888;
    GPIOC->ODR = 0x0000;
}

void init_spi(void) 
{
    RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
    
    SPI1->CR1 &= ~SPI_CR1_SPE;
    
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR1 |= SPI_CR1_BR_2;      // Baud rate: fPCLK/32 (~750kHz at 24MHz) - safer for LSM6DSO
    SPI1->CR1 |= SPI_CR1_CPOL;      // Clock polarity: idle high (Mode 3)
    SPI1->CR1 |= SPI_CR1_CPHA;      // Clock phase: capture on 2nd edge (Mode 3)
    SPI1->CR1 |= SPI_CR1_SSM;       // Software slave management
    SPI1->CR1 |= SPI_CR1_SSI;       // Internal slave select high
    
    SPI1->CR2 = 0;
    
    volatile u32 dummy = SPI1->DR;
    dummy = SPI1->SR;
    (void)dummy;
    
    SPI1->CR1 |= SPI_CR1_SPE;
    
    u32 timeout = 1000;
    while (!(SPI1->CR1 & SPI_CR1_SPE) && timeout--) {
        __NOP();
    }
    
    CS_PIN_HIGH();
}

// Initialize UART1 for 115200 baud
void init_uart(void) 
{
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;
    
    USART1->CR1 &= ~USART_CR1_UE;
    
    USART1->BRR = UART_BRR;
    
    USART1->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);
    
    USART1->CR2 &= ~USART_CR2_STOP;
    
    USART1->CR3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);
    
    USART1->SR = 0;
    
    USART1->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_TCIE | USART_CR1_TXEIE | 
                     USART_CR1_PEIE | USART_CR1_IDLEIE);
    
    USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    
    USART1->CR1 |= USART_CR1_UE;
    
    u32 timeout = 1000;
    while (!(USART1->SR & USART_SR_TC) && timeout--) {
        __NOP();
    }
    
    volatile u32 dummy = USART1->SR;
    USART1->DR = 0;
    (void)dummy;
    
    NVIC_SetPriority(USART1_IRQn, 2);
    NVIC_EnableIRQ(USART1_IRQn);
    
    if (!(USART1->CR1 & USART_CR1_UE) || 
        !(USART1->CR1 & USART_CR1_TE) ||
        USART1->BRR != UART_BRR) {
        //TODO: proper error handling
        // Configuration failed - you might want to set an error flag
        // or retry initialization
        while(1); // For now, halt on error
    }
}

// Initialize TIM2 for 100 Hz interrupts and TIM3 for delays
void init_timer(void) 
{
    RCC->APB1RSTR |= (RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);
    RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);
    
    for (volatile int i = 0; i < 100; i++) __NOP();
    
    TIM2->CR1 = 0;
    TIM2->CR2 = 0;
    TIM2->SMCR = 0;
    TIM2->DIER = 0;
    TIM2->SR = 0;
    TIM2->CNT = 0;
    
    TIM2->PSC = TIM2_PSC;
    TIM2->ARR = TIM2_ARR;
    
    TIM2->EGR |= TIM_EGR_UG;
    
    TIM2->SR &= ~TIM_SR_UIF;
    
    TIM2->DIER |= TIM_DIER_UIE;
    
    NVIC_SetPriority(TIM2_IRQn, 3);
    NVIC_EnableIRQ(TIM2_IRQn);
    
    TIM2->CR1 |= TIM_CR1_CEN;
    
    u32 timeout = TIMER_CONFIG_TIMEOUT;
    u16 initial_count = TIM2->CNT;
    while (TIM2->CNT == initial_count && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        // TODO: proper error handling
        // TIM2 failed to start - handle error
        snprintf(g_format_buffer, sizeof(g_format_buffer), 
                "ERR: TIM2 failed to start\r\n");
        uart_puts(g_format_buffer);
        set_state(STATE_ERROR);
        return;
    }
    
    TIM3->CR1 = 0;
    TIM3->CR2 = 0;
    TIM3->SMCR = 0;
    TIM3->DIER = 0;
    TIM3->SR = 0;
    TIM3->CNT = 0;
    
    TIM3->PSC = TIM3_PSC;
    TIM3->ARR = TIM3_ARR;
    
    TIM3->EGR |= TIM_EGR_UG;
    
    TIM3->SR &= ~TIM_SR_UIF;
    
    TIM3->CR1 |= TIM_CR1_CEN;
    
    timeout = TIMER_CONFIG_TIMEOUT;
    initial_count = TIM3->CNT;
    while (TIM3->CNT == initial_count && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        // TODO: proper error handling
        // TIM3 failed to start - handle error
        snprintf(g_format_buffer, sizeof(g_format_buffer), 
                "ERR: TIM3 failed to start\r\n");
        uart_puts(g_format_buffer);
        set_state(STATE_ERROR);
        return;
    }
    
    // Optional: Print timer configuration for debugging
    u32 tim2_freq = SYSTEM_CLOCK_HZ / ((TIM2->PSC + 1) * (TIM2->ARR + 1));
    u32 tim3_freq = SYSTEM_CLOCK_HZ / ((TIM3->PSC + 1) * (TIM3->ARR + 1));
    
    snprintf(g_format_buffer, sizeof(g_format_buffer), 
            "Timers OK: TIM2=%luHz TIM3=%luHz\r\n", tim2_freq, tim3_freq);
    uart_puts(g_format_buffer);
}

// delay function using TIM3 (1 ms resolution, blocking) // use with caution !!!
void delay_ms(u32 ms) 
{
    if (ms == 0) return;
    
    if (!(TIM3->CR1 & TIM_CR1_CEN)) {
        for (volatile u32 i = 0; i < (ms * (SYSTEM_CLOCK_HZ / 1000)); i++) {
            __NOP();
        }
        return;
    }
    
    for (u32 i = 0; i < ms; i++) {
        TIM3->CNT = 0;
        
        u32 timeout = 10000;
        while (TIM3->CNT == 0 && timeout--) {
            __NOP();
        }
        
        while (TIM3->CNT < 1 && timeout--) {
            __NOP();
        }
        
        if (timeout == 0) {
            break;
        }
    }
}

// Non-blocking delay function using system tick
u8 delay_ms_nb(u32 ms, u32 *start_time) 
{
    if (*start_time == 0U) {
        *start_time = get_system_tick();
        return 0;
    }
    
    if ((get_system_tick() - *start_time) >= ms) {
        *start_time = 0U;
        return 1;
    }
    
    return 0;
}

void get_timer_status(void) 
{
    snprintf(g_format_buffer, sizeof(g_format_buffer), 
            "TIM2: EN=%d CNT=%u PSC=%u ARR=%u\r\n", 
            (TIM2->CR1 & TIM_CR1_CEN) ? 1 : 0,
            (unsigned int)TIM2->CNT, 
            (unsigned int)TIM2->PSC, 
            (unsigned int)TIM2->ARR);
    uart_puts(g_format_buffer);
    
    snprintf(g_format_buffer, sizeof(g_format_buffer), 
            "TIM3: EN=%d CNT=%u PSC=%u ARR=%u\r\n", 
            (TIM3->CR1 & TIM_CR1_CEN) ? 1 : 0,
            (unsigned int)TIM3->CNT, 
            (unsigned int)TIM3->PSC, 
            (unsigned int)TIM3->ARR);
    uart_puts(g_format_buffer);
}

// Write a single byte via SPI with timeout
int spi_write(u8 reg, u8 data) 
{
    u32 timeout;
    
    while (SPI1->SR & SPI_SR_RXNE) {
        volatile u8 dummy = SPI1->DR;
        (void)dummy;
    }
    
    CS_PIN_LOW();
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { 
        CS_PIN_HIGH(); 
        return -1; 
    }
    SPI1->DR = reg;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    (void)SPI1->DR;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    SPI1->DR = data;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    (void)SPI1->DR;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    
    CS_PIN_HIGH();
    
    for (volatile int i = 0; i < 10; i++) __NOP();
    
    return 0;
}

// Read a single byte via SPI with timeout
int spi_read(u8 reg, u8* data) 
{
    u32 timeout;
    
    while (SPI1->SR & SPI_SR_RXNE) {
        volatile u8 dummy = SPI1->DR;
        (void)dummy;
    }
    
    CS_PIN_LOW();
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    SPI1->DR = (reg | 0x80U);
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    (void)SPI1->DR;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    SPI1->DR = 0xFF;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    *data = (uint8_t)SPI1->DR;
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) { timeout--; }
    
    CS_PIN_HIGH();
    
    for (volatile int i = 0; i < 10; i++) __NOP();
    
    return 0;
}

// Read multiple bytes via SPI (burst mode) with timeout
int spi_read_burst(u8 reg, u8* buffer, u8 len) 
{
    if ((buffer == NULL) || (len == 0U)) {
        return -1;
    }
    
    u32 timeout;
    
    while (SPI1->SR & SPI_SR_RXNE) {
        volatile u8 dummy = SPI1->DR;
        (void)dummy;
    }
    
    CS_PIN_LOW();
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    SPI1->DR = (reg | 0x80U);
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
    (void)SPI1->DR;
    
    for (uint8_t i = 0; i < len; i++)
    {
        timeout = SPI_UART_TIMEOUT;
        while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) { timeout--; }
        if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
        SPI1->DR = 0xFF;

        timeout = SPI_UART_TIMEOUT;
        while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) { timeout--; }
        if (timeout == 0U) { CS_PIN_HIGH(); return -1; }
        buffer[i] = (uint8_t)SPI1->DR;
    }
    
    timeout = SPI_UART_TIMEOUT;
    while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) { timeout--; }
 
    CS_PIN_HIGH();
    
    for (volatile int i = 0; i < 10; i++) __NOP();
    
    return 0;
}

// Send a string via UART using an interrupt-driven circular buffer (non-blocking)
void uart_puts(const char *s) 
{
    if (s == NULL) return;
    
    if (!(USART1->CR1 & USART_CR1_UE)) {
        return;
    }
    
    __disable_irq();
    
    size_t chars_added = 0U;
    while (*s != '\0') {
        u16 next_idx = (g_uart_tx.write_idx + 1U) % UART_TX_BUFFER_SIZE;
        
        if (next_idx == g_uart_tx.read_idx) {
            break;
        }
        
        g_uart_tx.buffer[g_uart_tx.write_idx] = *s;
        g_uart_tx.write_idx = next_idx;
        s++;
        chars_added++;
    }
    
    if ((chars_added > 0U) && ((USART1->CR1 & USART_CR1_TXEIE) == 0U)) {
        USART1->CR1 |= USART_CR1_TXEIE;
    }
    
    __enable_irq();
}