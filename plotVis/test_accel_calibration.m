% Скрипт для тестирования калибровки акселерометра на данных из monitor.csv

clear all;
close all;
clc;

fprintf('=== ТЕСТИРОВАНИЕ КАЛИБРОВКИ АКСЕЛЕРОМЕТРА ===\n\n');

try
    % Загружаем коэффициенты калибровки
    [bias_x, bias_y, bias_z, scale_x, scale_y, scale_z] = load_accel_calibration();
    
    % Читаем данные из monitor.csv
    monitor_file = '../logs/monitor.csv';
    if ~exist(monitor_file, 'file')
        error('Файл %s не найден!', monitor_file);
    end
    
    fprintf('\nЧтение данных из %s...\n', monitor_file);
    data = csvread(monitor_file, 1, 0); % Пропускаем заголовок
    
    % Извлекаем данные акселерометра (столбцы 4, 5, 6: ax_g, ay_g, az_g)
    ax_raw = data(:, 4);
    ay_raw = data(:, 5);
    az_raw = data(:, 6);
    
    fprintf('Загружено %d измерений\n', length(ax_raw));
    
    % Применяем калибровку
    [ax_cal, ay_cal, az_cal] = apply_accel_calibration(ax_raw, ay_raw, az_raw, ...
                                                       bias_x, bias_y, bias_z, ...
                                                       scale_x, scale_y, scale_z);
    
    % Статистика до калибровки
    fprintf('\n=== СТАТИСТИКА ДО КАЛИБРОВКИ ===\n');
    fprintf('ax_raw: mean=%.4f, std=%.4f, min=%.4f, max=%.4f\n', ...
            mean(ax_raw), std(ax_raw), min(ax_raw), max(ax_raw));
    fprintf('ay_raw: mean=%.4f, std=%.4f, min=%.4f, max=%.4f\n', ...
            mean(ay_raw), std(ay_raw), min(ay_raw), max(ay_raw));
    fprintf('az_raw: mean=%.4f, std=%.4f, min=%.4f, max=%.4f\n', ...
            mean(az_raw), std(az_raw), min(az_raw), max(az_raw));
    
    % Статистика после калибровки
    fprintf('\n=== СТАТИСТИКА ПОСЛЕ КАЛИБРОВКИ ===\n');
    fprintf('ax_cal: mean=%.4f, std=%.4f, min=%.4f, max=%.4f\n', ...
            mean(ax_cal), std(ax_cal), min(ax_cal), max(ax_cal));
    fprintf('ay_cal: mean=%.4f, std=%.4f, min=%.4f, max=%.4f\n', ...
            mean(ay_cal), std(ay_cal), min(ay_cal), max(ay_cal));
    fprintf('az_cal: mean=%.4f, std=%.4f, min=%.4f, max=%.4f\n', ...
            mean(az_cal), std(az_cal), min(az_cal), max(az_cal));
    
    % Вычисляем модуль ускорения
    accel_magnitude_raw = sqrt(ax_raw.^2 + ay_raw.^2 + az_raw.^2);
    accel_magnitude_cal = sqrt(ax_cal.^2 + ay_cal.^2 + az_cal.^2);
    
    fprintf('\n=== МОДУЛЬ УСКОРЕНИЯ ===\n');
    fprintf('До калибровки:  mean=%.4f, std=%.4f (идеал ≈ 1.0)\n', ...
            mean(accel_magnitude_raw), std(accel_magnitude_raw));
    fprintf('После калибровки: mean=%.4f, std=%.4f (идеал ≈ 1.0)\n', ...
            mean(accel_magnitude_cal), std(accel_magnitude_cal));
    
    % Строим графики сравнения
    figure('Position', [100, 100, 1200, 800]);
    
    % График 1: Сырые данные
    subplot(2, 2, 1);
    plot(ax_raw, 'r-', 'LineWidth', 1);
    hold on;
    plot(ay_raw, 'g-', 'LineWidth', 1);
    plot(az_raw, 'b-', 'LineWidth', 1);
    title('Сырые данные акселерометра');
    xlabel('Отсчет');
    ylabel('Ускорение (g)');
    legend('ax\_raw', 'ay\_raw', 'az\_raw');
    grid on;
    
    % График 2: Откалиброванные данные
    subplot(2, 2, 2);
    plot(ax_cal, 'r-', 'LineWidth', 1);
    hold on;
    plot(ay_cal, 'g-', 'LineWidth', 1);
    plot(az_cal, 'b-', 'LineWidth', 1);
    title('Откалиброванные данные акселерометра');
    xlabel('Отсчет');
    ylabel('Ускорение (g)');
    legend('ax\_cal', 'ay\_cal', 'az\_cal');
    grid on;
    
    % График 3: Модуль ускорения
    subplot(2, 2, 3);
    plot(accel_magnitude_raw, 'k-', 'LineWidth', 1);
    hold on;
    plot(accel_magnitude_cal, 'r-', 'LineWidth', 1);
    plot([1, length(accel_magnitude_raw)], [1, 1], 'g--', 'LineWidth', 2);
    title('Модуль ускорения');
    xlabel('Отсчет');
    ylabel('|a| (g)');
    legend('До калибровки', 'После калибровки', 'Идеал (1g)');
    grid on;
    
    % График 4: Гистограмма модуля ускорения
    subplot(2, 2, 4);
    [n1, x1] = hist(accel_magnitude_raw, 50);
    [n2, x2] = hist(accel_magnitude_cal, 50);
    bar(x1, n1, 'FaceColor', [0.7 0.7 0.7], 'EdgeColor', 'k', 'FaceAlpha', 0.7);
    hold on;
    bar(x2, n2, 'FaceColor', [1 0.3 0.3], 'EdgeColor', 'r', 'FaceAlpha', 0.7);
    plot([1, 1], [0, max(max(n1), max(n2))], 'g--', 'LineWidth', 2);
    title('Распределение модуля ускорения');
    xlabel('|a| (g)');
    ylabel('Количество');
    legend('До калибровки', 'После калибровки', 'Идеал (1g)');
    grid on;
    
    % Сохраняем график
    print('accel_calibration_test.png', '-dpng', '-r300');
    fprintf('\nГрафик сохранен в файл: accel_calibration_test.png\n');
    
    fprintf('\nТестирование завершено!\n');
    
catch err
    fprintf('ОШИБКА при тестировании калибровки:\n');
    fprintf('%s\n', err.message);
end