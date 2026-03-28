function [ax_cal, ay_cal, az_cal] = apply_accel_calibration(ax_raw, ay_raw, az_raw, bias_x, bias_y, bias_z, scale_x, scale_y, scale_z)
    % Применение калибровки к сырым данным акселерометра
    % 
    % Входные параметры:
    %   ax_raw, ay_raw, az_raw - сырые данные акселерометра (в g)
    %   bias_x, bias_y, bias_z - коэффициенты смещения
    %   scale_x, scale_y, scale_z - масштабные коэффициенты
    %
    % Выходные параметры:
    %   ax_cal, ay_cal, az_cal - калиброванные данные акселерометра (в g)
    
    % Применение формулы калибровки: A_calibrated = (A_raw - bias) * scale
    ax_cal = (ax_raw - bias_x) .* scale_x;
    ay_cal = (ay_raw - bias_y) .* scale_y;
    az_cal = (az_raw - bias_z) .* scale_z;
end