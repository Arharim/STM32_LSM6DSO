function [bias_x, bias_y, bias_z, scale_x, scale_y, scale_z] = accel_6axis_calibration()
    % Калибровка акселерометра методом 6-ти осей
    % Возвращает коэффициенты смещения (bias) и масштаба (scale) для каждой оси
    
    % Пути к файлам с данными калибровки
    files = {
        '../logs/accel_X_down_calibration_data.csv',  % X вниз (+1g по X)
        '../logs/accel_X_up_calibration_data.csv',    % X вверх (-1g по X)
        '../logs/accel_Y_down_calibration_data.csv',  % Y вниз (+1g по Y)
        '../logs/accel_Y_up_calibration_data.csv',    % Y вверх (-1g по Y)
        '../logs/accel_Z_down_calibration_data.csv',  % Z вниз (+1g по Z)
        '../logs/accel_Z_up_calibration_data.csv'     % Z вверх (-1g по Z)
    };
    
    % Названия позиций для отображения
    position_names = {
        'X down (+1g)', 'X up (-1g)',
        'Y down (+1g)', 'Y up (-1g)', 
        'Z down (+1g)', 'Z up (-1g)'
    };
    
    % Время записи для каждой позиции (в секундах)
    recording_times = [47.03, 67.60, 66.53, 69.85, 47.48, 51.12];
    
    % Массив для хранения усредненных значений
    avg_values = zeros(6, 3); % 6 позиций, 3 оси (X, Y, Z)
    
    fprintf('=== Калибровка акселерометра методом 6-ти осей ===\n\n');
    
    % Обработка каждого файла
    for i = 1:length(files)
        if exist(files{i}, 'file')
            fprintf('Обработка файла: %s\n', files{i});
            fprintf('Позиция: %s (время записи: %.2f сек)\n', position_names{i}, recording_times(i));
            
            % Чтение данных из CSV файла
            data = csvread(files{i}, 1, 0); % Пропускаем заголовок
            
            if size(data, 1) == 0
                fprintf('ОШИБКА: Файл %s пуст!\n\n', files{i});
                continue;
            end
            
            % Извлечение данных акселерометра (колонки 5, 6, 7 - ax_g, ay_g, az_g)
            ax = data(:, 5); % Ускорение по X в g
            ay = data(:, 6); % Ускорение по Y в g  
            az = data(:, 7); % Ускорение по Z в g
            
            % Вычисление средних значений
            avg_ax = mean(ax);
            avg_ay = mean(ay);
            avg_az = mean(az);
            
            % Сохранение усредненных значений
            avg_values(i, 1) = avg_ax;
            avg_values(i, 2) = avg_ay;
            avg_values(i, 3) = avg_az;
            
            fprintf('  Количество измерений: %d\n', length(ax));
            fprintf('  Средние значения: X=%.6f g, Y=%.6f g, Z=%.6f g\n', avg_ax, avg_ay, avg_az);
            fprintf('  Стандартные отклонения: X=%.6f g, Y=%.6f g, Z=%.6f g\n\n', std(ax), std(ay), std(az));
        else
            fprintf('ОШИБКА: Файл %s не найден!\n\n', files{i});
        end
    end
    
    % Проверка, что все файлы были обработаны
    if any(all(avg_values == 0, 2))
        fprintf('ОШИБКА: Не все файлы калибровки были успешно обработаны!\n');
        bias_x = 0; bias_y = 0; bias_z = 0;
        scale_x = 1; scale_y = 1; scale_z = 1;
        return;
    end
    
    fprintf('=== Усредненные значения по позициям ===\n');
    for i = 1:6
        fprintf('%s: X=%.6f g, Y=%.6f g, Z=%.6f g\n', ...
                position_names{i}, avg_values(i, 1), avg_values(i, 2), avg_values(i, 3));
    end
    fprintf('\n');
    
    % Расчет коэффициентов калибровки
    fprintf('=== Расчет коэффициентов калибровки ===\n');
    
    % Ось X: позиции 1 (X_down) и 2 (X_up)
    X_down = avg_values(1, 1); % Должно быть +1g
    X_up = avg_values(2, 1);   % Должно быть -1g
    
    bias_x = (X_down + X_up) / 2;
    measured_sensitivity_x = (X_down - X_up) / 2;
    scale_x = 1.0 / measured_sensitivity_x;
    
    fprintf('Ось X:\n');
    fprintf('  X_down = %.6f g (ожидается +1.0 g)\n', X_down);
    fprintf('  X_up   = %.6f g (ожидается -1.0 g)\n', X_up);
    fprintf('  Смещение (bias_x) = %.6f g\n', bias_x);
    fprintf('  Измеренная чувствительность = %.6f g/g\n', measured_sensitivity_x);
    fprintf('  Масштабный коэффициент (scale_x) = %.6f\n\n', scale_x);
    
    % Ось Y: позиции 3 (Y_down) и 4 (Y_up)
    Y_down = avg_values(3, 2); % Должно быть +1g
    Y_up = avg_values(4, 2);   % Должно быть -1g
    
    bias_y = (Y_down + Y_up) / 2;
    measured_sensitivity_y = (Y_down - Y_up) / 2;
    scale_y = 1.0 / measured_sensitivity_y;
    
    fprintf('Ось Y:\n');
    fprintf('  Y_down = %.6f g (ожидается +1.0 g)\n', Y_down);
    fprintf('  Y_up   = %.6f g (ожидается -1.0 g)\n', Y_up);
    fprintf('  Смещение (bias_y) = %.6f g\n', bias_y);
    fprintf('  Измеренная чувствительность = %.6f g/g\n', measured_sensitivity_y);
    fprintf('  Масштабный коэффициент (scale_y) = %.6f\n\n', scale_y);
    
    % Ось Z: позиции 5 (Z_down) и 6 (Z_up)
    Z_down = avg_values(5, 3); % Должно быть +1g
    Z_up = avg_values(6, 3);   % Должно быть -1g
    
    bias_z = (Z_down + Z_up) / 2;
    measured_sensitivity_z = (Z_down - Z_up) / 2;
    scale_z = 1.0 / measured_sensitivity_z;
    
    fprintf('Ось Z:\n');
    fprintf('  Z_down = %.6f g (ожидается +1.0 g)\n', Z_down);
    fprintf('  Z_up   = %.6f g (ожидается -1.0 g)\n', Z_up);
    fprintf('  Смещение (bias_z) = %.6f g\n', bias_z);
    fprintf('  Измеренная чувствительность = %.6f g/g\n', measured_sensitivity_z);
    fprintf('  Масштабный коэффициент (scale_z) = %.6f\n\n', scale_z);
    
    % Итоговые коэффициенты калибровки
    fprintf('=== ИТОГОВЫЕ КОЭФФИЦИЕНТЫ КАЛИБРОВКИ ===\n');
    fprintf('Смещения (bias):\n');
    fprintf('  bias_x = %.6f g\n', bias_x);
    fprintf('  bias_y = %.6f g\n', bias_y);
    fprintf('  bias_z = %.6f g\n', bias_z);
    fprintf('\nМасштабные коэффициенты (scale):\n');
    fprintf('  scale_x = %.6f\n', scale_x);
    fprintf('  scale_y = %.6f\n', scale_y);
    fprintf('  scale_z = %.6f\n', scale_z);
    
    % Формула применения калибровки
    fprintf('\n=== ФОРМУЛА ПРИМЕНЕНИЯ КАЛИБРОВКИ ===\n');
    fprintf('Для калибровки сырых данных используйте:\n');
    fprintf('  ax_calibrated = (ax_raw - %.6f) * %.6f\n', bias_x, scale_x);
    fprintf('  ay_calibrated = (ay_raw - %.6f) * %.6f\n', bias_y, scale_y);
    fprintf('  az_calibrated = (az_raw - %.6f) * %.6f\n', bias_z, scale_z);
    
    % Коэффициенты для кода на C
    fprintf('\n=== КОЭФФИЦИЕНТЫ ДЛЯ КОДА НА C ===\n');
    fprintf('#define ACCEL_BIAS_X  %.6ff\n', bias_x);
    fprintf('#define ACCEL_BIAS_Y  %.6ff\n', bias_y);
    fprintf('#define ACCEL_BIAS_Z  %.6ff\n', bias_z);
    fprintf('#define ACCEL_SCALE_X %.6ff\n', scale_x);
    fprintf('#define ACCEL_SCALE_Y %.6ff\n', scale_y);
    fprintf('#define ACCEL_SCALE_Z %.6ff\n', scale_z);
    
    % Оценка качества калибровки
    fprintf('\n=== ОЦЕНКА КАЧЕСТВА КАЛИБРОВКИ ===\n');
    
    % Проверяем, насколько близки измеренные значения к ожидаемым ±1g
    error_x_down = abs(X_down - 1.0);
    error_x_up = abs(X_up - (-1.0));
    error_y_down = abs(Y_down - 1.0);
    error_y_up = abs(Y_up - (-1.0));
    error_z_down = abs(Z_down - 1.0);
    error_z_up = abs(Z_up - (-1.0));
    
    max_error = max([error_x_down, error_x_up, error_y_down, error_y_up, error_z_down, error_z_up]);
    avg_error = mean([error_x_down, error_x_up, error_y_down, error_y_up, error_z_down, error_z_up]);
    
    fprintf('Максимальная ошибка: %.6f g\n', max_error);
    fprintf('Средняя ошибка: %.6f g\n', avg_error);
    
    if max_error < 0.05
        fprintf('Качество калибровки: ОТЛИЧНОЕ (ошибка < 0.05g)\n');
    elseif max_error < 0.1
        fprintf('Качество калибровки: ХОРОШЕЕ (ошибка < 0.1g)\n');
    elseif max_error < 0.2
        fprintf('Качество калибровки: УДОВЛЕТВОРИТЕЛЬНОЕ (ошибка < 0.2g)\n');
    else
        fprintf('Качество калибровки: ПЛОХОЕ (ошибка >= 0.2g)\n');
        fprintf('РЕКОМЕНДАЦИЯ: Повторите калибровку с более точным позиционированием датчика\n');
    end
    
    % Сохранение коэффициентов в файл
    save_coefficients_to_file(bias_x, bias_y, bias_z, scale_x, scale_y, scale_z);
    
    fprintf('\n=== КАЛИБРОВКА ЗАВЕРШЕНА ===\n');
end

function save_coefficients_to_file(bias_x, bias_y, bias_z, scale_x, scale_y, scale_z)
    % Сохранение коэффициентов калибровки в файл
    filename = '../logs/accel_calibration_coefficients.txt';
    
    fid = fopen(filename, 'w');
    if fid == -1
        fprintf('ОШИБКА: Не удалось создать файл %s\n', filename);
        return;
    end
    
    fprintf(fid, '# Коэффициенты калибровки акселерометра (метод 6-ти осей)\n');
    fprintf(fid, '# Дата создания: %s\n', datestr(now));
    fprintf(fid, '\n');
    fprintf(fid, '# Смещения (bias) в g\n');
    fprintf(fid, 'ACCEL_BIAS_X=%.6f\n', bias_x);
    fprintf(fid, 'ACCEL_BIAS_Y=%.6f\n', bias_y);
    fprintf(fid, 'ACCEL_BIAS_Z=%.6f\n', bias_z);
    fprintf(fid, '\n');
    fprintf(fid, '# Масштабные коэффициенты\n');
    fprintf(fid, 'ACCEL_SCALE_X=%.6f\n', scale_x);
    fprintf(fid, 'ACCEL_SCALE_Y=%.6f\n', scale_y);
    fprintf(fid, 'ACCEL_SCALE_Z=%.6f\n', scale_z);
    fprintf(fid, '\n');
    fprintf(fid, '# Формула применения:\n');
    fprintf(fid, '# ax_calibrated = (ax_raw - ACCEL_BIAS_X) * ACCEL_SCALE_X\n');
    fprintf(fid, '# ay_calibrated = (ay_raw - ACCEL_BIAS_Y) * ACCEL_SCALE_Y\n');
    fprintf(fid, '# az_calibrated = (az_raw - ACCEL_BIAS_Z) * ACCEL_SCALE_Z\n');
    
    fclose(fid);
    
    fprintf('Коэффициенты калибровки сохранены в файл: %s\n', filename);
end