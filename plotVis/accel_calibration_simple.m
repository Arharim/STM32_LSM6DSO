% Simple 6-axis accelerometer calibration script
% Processes calibration data and outputs bias and scale coefficients

clear all;
close all;
clc;

fprintf('=== 6-Axis Accelerometer Calibration ===\n\n');

% Define file paths
files = {
    '../logs/accel_X_down_calibration_data.csv',  % Position 1: X down (+1g)
    '../logs/accel_X_up_calibration_data.csv',    % Position 2: X up (-1g)
    '../logs/accel_Y_down_calibration_data.csv',  % Position 3: Y down (+1g)
    '../logs/accel_Y_up_calibration_data.csv',    % Position 4: Y up (-1g)
    '../logs/accel_Z_down_calibration_data.csv',  % Position 5: Z down (+1g)
    '../logs/accel_Z_up_calibration_data.csv'     % Position 6: Z up (-1g)
};

% Time durations for each measurement
durations = [47.03, 67.60, 66.53, 69.85, 47.48, 51.12];
position_names = {'X_down', 'X_up', 'Y_down', 'Y_up', 'Z_down', 'Z_up'};

% Initialize storage for averaged values
avg_values = zeros(6, 3); % 6 positions, 3 axes

% Process each calibration file
for i = 1:length(files)
    fprintf('Reading %s (%.1fs)... ', position_names{i}, durations(i));
    
    % Read CSV file (skip header)
    try
        data = csvread(files{i}, 1, 0);
        % Extract accelerometer data (columns 5, 6, 7 are ax_g, ay_g, az_g)
        accel_data = data(:, 5:7);
    catch
        error('Error reading file: %s', files{i});
    end
    
    % Calculate average
    avg_values(i, :) = mean(accel_data, 1);
    fprintf('%d samples, avg=[%.4f, %.4f, %.4f]g\n', ...
            size(accel_data, 1), avg_values(i, 1), avg_values(i, 2), avg_values(i, 3));
end

fprintf('\n=== Calibration Calculation ===\n');

% Extract measurements for each axis
X_down = avg_values(1, 1);  % X reading when X axis points down
X_up   = avg_values(2, 1);  % X reading when X axis points up
Y_down = avg_values(3, 2);  % Y reading when Y axis points down  
Y_up   = avg_values(4, 2);  % Y reading when Y axis points up
Z_down = avg_values(5, 3);  % Z reading when Z axis points down
Z_up   = avg_values(6, 3);  % Z reading when Z axis points up

fprintf('Raw measurements:\n');
fprintf('  X: down=%.6f, up=%.6f\n', X_down, X_up);
fprintf('  Y: down=%.6f, up=%.6f\n', Y_down, Y_up);
fprintf('  Z: down=%.6f, up=%.6f\n', Z_down, Z_up);

% Calculate bias (offset) - should be 0 in ideal case
bias_x = (X_down + X_up) / 2;
bias_y = (Y_down + Y_up) / 2;
bias_z = (Z_down + Z_up) / 2;

% Calculate measured sensitivity - should be 1.0 in ideal case
sens_x = (X_down - X_up) / 2;
sens_y = (Y_down - Y_up) / 2;
sens_z = (Z_down - Z_up) / 2;

% Calculate scale correction factors
scale_x = 1.0 / sens_x;
scale_y = 1.0 / sens_y;
scale_z = 1.0 / sens_z;

fprintf('\n=== CALIBRATION RESULTS ===\n');
fprintf('Bias (offset):\n');
fprintf('  X: %+.6f g\n', bias_x);
fprintf('  Y: %+.6f g\n', bias_y);
fprintf('  Z: %+.6f g\n', bias_z);

fprintf('\nScale (correction factor):\n');
fprintf('  X: %.6f\n', scale_x);
fprintf('  Y: %.6f\n', scale_y);
fprintf('  Z: %.6f\n', scale_z);

fprintf('\nMeasured sensitivity:\n');
fprintf('  X: %.6f g/g (error: %+.2f%%)\n', sens_x, (sens_x-1)*100);
fprintf('  Y: %.6f g/g (error: %+.2f%%)\n', sens_y, (sens_y-1)*100);
fprintf('  Z: %.6f g/g (error: %+.2f%%)\n', sens_z, (sens_z-1)*100);

fprintf('\n=== CALIBRATION FORMULA ===\n');
fprintf('Apply to raw readings:\n');
fprintf('  accel_x_corrected = (accel_x_raw - %.6f) * %.6f\n', bias_x, scale_x);
fprintf('  accel_y_corrected = (accel_y_raw - %.6f) * %.6f\n', bias_y, scale_y);
fprintf('  accel_z_corrected = (accel_z_raw - %.6f) * %.6f\n', bias_z, scale_z);

fprintf('\n=== C CODE CONSTANTS ===\n');
fprintf('#define ACCEL_BIAS_X    (%.6ff)\n', bias_x);
fprintf('#define ACCEL_BIAS_Y    (%.6ff)\n', bias_y);
fprintf('#define ACCEL_BIAS_Z    (%.6ff)\n', bias_z);
fprintf('#define ACCEL_SCALE_X   (%.6ff)\n', scale_x);
fprintf('#define ACCEL_SCALE_Y   (%.6ff)\n', scale_y);
fprintf('#define ACCEL_SCALE_Z   (%.6ff)\n', scale_z);

fprintf('\n=== VERIFICATION ===\n');
fprintf('Calibrated values for each position:\n');
expected = [1 0 0; -1 0 0; 0 1 0; 0 -1 0; 0 0 1; 0 0 -1];
total_error = 0;

for i = 1:6
    % Apply calibration
    cal_x = (avg_values(i,1) - bias_x) * scale_x;
    cal_y = (avg_values(i,2) - bias_y) * scale_y;
    cal_z = (avg_values(i,3) - bias_z) * scale_z;
    
    % Calculate error
    err_x = cal_x - expected(i,1);
    err_y = cal_y - expected(i,2);
    err_z = cal_z - expected(i,3);
    rms_err = sqrt(err_x^2 + err_y^2 + err_z^2);
    total_error = total_error + rms_err;
    
    fprintf('  %s: [%+.3f, %+.3f, %+.3f] (error: %.4fg)\n', ...
            position_names{i}, cal_x, cal_y, cal_z, rms_err);
end

avg_error = total_error / 6;
fprintf('\nAverage RMS error: %.4f g\n', avg_error);

if avg_error < 0.01
    fprintf('✓ Calibration quality: EXCELLENT\n');
elseif avg_error < 0.02
    fprintf('✓ Calibration quality: GOOD\n');
elseif avg_error < 0.05
    fprintf('⚠ Calibration quality: ACCEPTABLE\n');
else
    fprintf('✗ Calibration quality: POOR\n');
end

fprintf('\nCalibration completed!\n');