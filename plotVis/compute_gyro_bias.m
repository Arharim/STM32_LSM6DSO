function [bias_x, bias_y, bias_z] = compute_gyro_bias(csv_path)

  if nargin < 1 || isempty(csv_path)
    script_dir = fileparts(mfilename('fullpath'));
    csv_path = fullfile(script_dir, '..', 'logs', 'gyro_calibration_data.csv');
  end

  if ~exist(csv_path, 'file')
    error('file not found', csv_path);
  end

  fid = fopen(csv_path, 'r');
  if fid < 0
    error('Could not open the file: %s', csv_path);
  end

  unwind_protect
    header = fgetl(fid); %#ok<NASGU>
    C = textscan(fid, '%f%f%f%f%f%f%f', 'Delimiter', ',', ...
                 'CollectOutput', true);
    data = C{1}; % N x 7: [Index gx gy gz ax ay az]
  unwind_protect_cleanup
    fclose(fid);
  end_unwind_protect

  if size(data, 2) < 4
    error('Expected >= 4 columns (Index,gx_dps,gy_dps,gz_dps,...) в %s', csv_path);
  end

  gx = data(:, 2); % gx_dps
  gy = data(:, 3); % gy_dps
  gz = data(:, 4); % gz_dps

  gx = gx(!isnan(gx));
  gy = gy(!isnan(gy));
  gz = gz(!isnan(gz));

  bias_x = mean(gx);
  bias_y = mean(gy);
  bias_z = mean(gz);

  fprintf('bias_x = %.6f dps\n', bias_x);
  fprintf('bias_y = %.6f dps\n', bias_y);
  fprintf('bias_z = %.6f dps\n', bias_z);
end

function [gx_corr, gy_corr, gz_corr] = apply_gyro_bias(gx, gy, gz, bias_x, bias_y, bias_z)
  gx_corr = gx - bias_x;
  gy_corr = gy - bias_y;
  gz_corr = gz - bias_z;
end
