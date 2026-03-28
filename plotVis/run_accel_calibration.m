% Скрипт для запуска калибровки акселерометра методом 6-ти осей
% 
% Этот скрипт выполняет калибровку акселерометра используя данные
% из 6 различных ориентаций датчика относительно силы тяжести.
%
% Требуемые файлы данных:
% - accel_X_down_calibration_data.csv (X ось направлена вниз, +1g)
% - accel_X_up_calibration_data.csv   (X ось направлена вверх, -1g)
% - accel_Y_down_calibration_data.csv (Y ось направлена вниз, +1g)
% - accel_Y_up_calibration_data.csv   (Y ось направлена вверх, -1g)
% - accel_Z_down_calibration_data.csv (Z ось направлена вниз, +1g)
% - accel_Z_up_calibration_data.csv   (Z ось направлена вверх, -1g)

clear all;
close all;
clc;

fprintf('=== ЗАПУСК КАЛИБРОВКИ АКСЕЛЕРОМЕТРА ===\n\n');

% Проверка наличия необходимых файлов
required_files = {
    '../logs/accel_X_down_calibration_data.csv',
    '../logs/accel_X_up_calibration_data.csv',
    '../logs/accel_Y_down_calibration_data.csv',
    '../logs/accel_Y_up_calibration_data.csv',
    '../logs/accel_Z_down_calibration_data.csv',
    '../logs/accel_Z_up_calibration_data.csv'
};

missing_files = {};
for i = 1:length(required_files)
    if ~exist(required_files{i}, 'file')
        missing_files{end+1} = required_files{i};
    end
end

if ~isempty(missing_files)
    fprintf('ОШИБКА: Отсутствуют следующие файлы данных калибровки:\n');
    for i = 1:length(missing_files)
        fprintf('  - %s\n', missing_files{i});
    end
    fprintf('\nПожалуйста, убедитесь, что все файлы калибровки находятся в папке logs/\n');
    return;
end

fprintf('Все необходимые файлы найдены. Начинаем калибровку...\n\n');

% Выполнение калибровки
try
    [bias_x, bias_y, bias_z, scale_x, scale_y, scale_z] = accel_6axis_calibration();
    
    fprintf('\n=== КАЛИБРОВКА УСПЕШНО ЗАВЕРШЕНА ===\n');
    fprintf('Коэффициенты калибровки:\n');
    fprintf('  Смещения: X=%.6f, Y=%.6f, Z=%.6f (g)\n', bias_x, bias_y, bias_z);
    fprintf('  Масштаб:  X=%.6f, Y=%.6f, Z=%.6f\n', scale_x, scale_y, scale_z);
    
catch ME
    fprintf('ОШИБКА при выполнении калибровки:\n');
    fprintf('  %s\n', ME.message);
    fprintf('  В файле: %s, строка %d\n', ME.stack(1).file, ME.stack(1).line);
end

fprintf('\nДля применения калибровки используйте функцию apply_accel_calibration()\n');
fprintf('или загрузите коэффициенты из файла accel_calibration_coefficients.txt\n');