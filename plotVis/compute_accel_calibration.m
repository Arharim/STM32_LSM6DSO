function [bias_x, scale_x, bias_y, scale_y, bias_z, scale_z] = compute_accel_calibration()
    % Compute accelerometer calibration coefficients from 6-axis calibration data
    % Returns bias and scale factors for X, Y, Z axes
    
    % Define file paths
    files = {
        '../logs/accel_X_down_calibration_data.csv',  % X axis down (+1g expected)
        '../logs/accel_X_up_calibration_data.csv',    % X axis up (-1g expected)
        '../logs/accel_Y_down_calibration_data.csv',  % Y axis down (+1g expected)
        '../logs/accel_Y_up_calibration_data.csv',    % Y axis up (-1g expected)
        '../logs/accel_Z_down_calibration_data.csv',  % Z axis down (+1g expected)
        '../logs/accel_Z_up_calibration_data.csv'     % Z axis up (-1g expected)
    };
    
    % Initialize arrays to store averaged values
    avg_values = zeros(6, 3); % 6 positions, 3 axes (X, Y, Z)
    
    % Process each file
    for i = 1:length(files)
        % Check if file exists
        if ~exist(files{i}, 'file')
            error('Accelerometer calibration file not found: %s', files{i});
        end
        
        % Read CSV data (skip header)
        try
            data = csvread(files{i}, 1, 0);
            % Extract accelerometer data (columns 5, 6, 7 are ax_g, ay_g, az_g)
            accel_data = data(:, 5:7);
            % Calculate average values for each axis
            avg_values(i, :) = mean(accel_data, 1);
        catch
            error('Error reading accelerometer calibration file: %s', files{i});
        end
    end
    
    % Extract measurements for each axis
    X_down = avg_values(1, 1);  % X axis reading when X is down
    X_up = avg_values(2, 1);    % X axis reading when X is up
    Y_down = avg_values(3, 2);  % Y axis reading when Y is down
    Y_up = avg_values(4, 2);    % Y axis reading when Y is up
    Z_down = avg_values(5, 3);  % Z axis reading when Z is down
    Z_up = avg_values(6, 3);    % Z axis reading when Z is up
    
    % Calculate bias (offset) for each axis
    bias_x = (X_down + X_up) / 2;
    bias_y = (Y_down + Y_up) / 2;
    bias_z = (Z_down + Z_up) / 2;
    
    % Calculate measured sensitivity
    measured_scale_x = (X_down - X_up) / 2;
    measured_scale_y = (Y_down - Y_up) / 2;
    measured_scale_z = (Z_down - Z_up) / 2;
    
    % Calculate correction scale factors
    scale_x = 1.0 / measured_scale_x;
    scale_y = 1.0 / measured_scale_y;
    scale_z = 1.0 / measured_scale_z;
end