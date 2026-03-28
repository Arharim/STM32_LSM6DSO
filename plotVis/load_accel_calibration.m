function [bias_x, bias_y, bias_z, scale_x, scale_y, scale_z] = load_accel_calibration()
    % Загрузка коэффициентов калибровки акселерометра из файла
    % Возвращает коэффициенты смещения и масштаба для каждой оси
    
    filename = '../logs/accel_calibration_coefficients.txt';
    
    % Значения по умолчанию (без калибровки)
    bias_x = 0; bias_y = 0; bias_z = 0;
    scale_x = 1; scale_y = 1; scale_z = 1;
    
    if ~exist(filename, 'file')
        fprintf('Файл калибровки %s не найден. Используются значения по умолчанию.\n', filename);
        fprintf('Запустите run_accel_calibration.m для создания коэффициентов калибровки.\n');
        return;
    end
    
    try
        fid = fopen(filename, 'r');
        if fid == -1
            fprintf('Ошибка открытия файла %s\n', filename);
            return;
        end
        
        % Чтение файла построчно
        while ~feof(fid)
            line = fgetl(fid);
            if ischar(line) && ~isempty(line) && line(1) ~= '#'
                % Парсинг строк вида PARAMETER=VALUE
                eq_pos = strfind(line, '=');
                if ~isempty(eq_pos)
                    param_name = strtrim(line(1:eq_pos-1));
                    param_value = str2double(strtrim(line(eq_pos+1:end)));
                    
                    switch param_name
                        case 'ACCEL_BIAS_X'
                            bias_x = param_value;
                        case 'ACCEL_BIAS_Y'
                            bias_y = param_value;
                        case 'ACCEL_BIAS_Z'
                            bias_z = param_value;
                        case 'ACCEL_SCALE_X'
                            scale_x = param_value;
                        case 'ACCEL_SCALE_Y'
                            scale_y = param_value;
                        case 'ACCEL_SCALE_Z'
                            scale_z = param_value;
                    end
                end
            end
        end
        
        fclose(fid);
        
        fprintf('Коэффициенты калибровки акселерометра загружены из %s:\n', filename);
        fprintf('  Смещения: X=%.6f, Y=%.6f, Z=%.6f (g)\n', bias_x, bias_y, bias_z);
        fprintf('  Масштаб:  X=%.6f, Y=%.6f, Z=%.6f\n', scale_x, scale_y, scale_z);
        
    catch ME
        fprintf('Ошибка при чтении файла калибровки: %s\n', ME.message);
        fprintf('Используются значения по умолчанию.\n');
    end
end