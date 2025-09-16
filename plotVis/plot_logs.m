clear all; close all; clc;

% CONFIGURATION
% Sampling frequency [Hz]. Set to a positive value if known.
FS = NaN;
WINDOW_SEC = 2;

PLOT_TS       = true;   % Time series for gyro/accel with smoothing
PLOT_MAG      = true;   % Magnitudes |g| and |a|
PLOT_2D_ACCEL = true;   % 2D projection ax-ay
PLOT_2D_GYRO  = false;  % 2D projections for gyro (optional)
PLOT_3D_ACCEL = false;  % 3D scatter for accelerometer (colored by time/index)
PLOT_3D_GYRO  = false;  % 3D scatter for gyroscope (colored by time/index)
PLOT_HIST     = true;   % Histograms for each axis
PLOT_PSD      = true;   % PSD/periodogram for each axis

thisFile  = mfilename('fullpath');
scriptDir = fileparts(thisFile);
logdir    = fullfile(scriptDir, '..', 'logs');

if ~exist(logdir, 'dir')
  error("Папка логов не найдена: %s", logdir);
endif

entries = dir(logdir);
isFile  = ~[entries.isdir];
names   = {entries(isFile).name};
csvMask = cellfun(@(s) length(s) >= 4 && any(strcmpi(s(end-3:end), {'.csv'})), names);
csvNames = names(csvMask);

if isempty(csvNames)
  printf("Содержимое %s:\n", logdir);
  disp(names');
  error("Нет файлов .csv в папке %s", logdir);
endif

% Загрузка смещения (калибровки) гироскопа один раз для всех графиков
bias_x = 0; bias_y = 0; bias_z = 0;
try
  [bias_x, bias_y, bias_z] = compute_gyro_bias();
  printf("Используется калибровка гироскопа: [%.6f %.6f %.6f] dps\n", bias_x, bias_y, bias_z);
catch
  warning("Не удалось загрузить калибровку гироскопа. Смещения будут нулевыми.");
end_try_catch

for k = 1:numel(csvNames)
  fname = fullfile(logdir, csvNames{k});
  printf("Загружаю %s ...\n", csvNames{k});

  data = csvread(fname, 1, 0);

  idx = data(:,1);
  gx  = data(:,2); gy = data(:,3); gz = data(:,4);
  ax  = data(:,5); ay = data(:,6); az = data(:,7);

  % Применяем калибровку гироскопа (вычитаем смещения)
  gx = gx - bias_x;
  gy = gy - bias_y;
  gz = gz - bias_z;

  if isfinite(FS) && FS > 0
    t = (idx - idx(1)) / FS;
    xLabelTS = "Time [s]";
    winN = max(1, round(WINDOW_SEC * FS));
  else
    t = idx;
    xLabelTS = "Index";
    winN = 101;
  endif

  smooth = @(x,n) filter(ones(1,n)/n, 1, x);

  gm = sqrt(gx.^2 + gy.^2 + gz.^2);
  am = sqrt(ax.^2 + ay.^2 + az.^2);

  if PLOT_TS
    figure("name", ["Gyro time series: " csvNames{k}]);
    subplot(3,1,1); hold on; plot(t, gx, "-"); plot(t, smooth(gx, winN), "-", "LineWidth", 1.2); grid on; ylabel("gx [dps]"); title(["Gyro time series: " csvNames{k}]); legend("raw","mean");
    subplot(3,1,2); hold on; plot(t, gy, "-"); plot(t, smooth(gy, winN), "-", "LineWidth", 1.2); grid on; ylabel("gy [dps]"); legend("raw","mean");
    subplot(3,1,3); hold on; plot(t, gz, "-"); plot(t, smooth(gz, winN), "-", "LineWidth", 1.2); grid on; ylabel("gz [dps]"); xlabel(xLabelTS); legend("raw","mean");

    figure("name", ["Accel time series: " csvNames{k}]);
    subplot(3,1,1); hold on; plot(t, ax, "-"); plot(t, smooth(ax, winN), "-", "LineWidth", 1.2); grid on; ylabel("ax [g]"); title(["Accel time series: " csvNames{k}]); legend("raw","mean");
    subplot(3,1,2); hold on; plot(t, ay, "-"); plot(t, smooth(ay, winN), "-", "LineWidth", 1.2); grid on; ylabel("ay [g]"); legend("raw","mean");
    subplot(3,1,3); hold on; plot(t, az, "-"); plot(t, smooth(az, winN), "-", "LineWidth", 1.2); grid on; ylabel("az [g]"); xlabel(xLabelTS); legend("raw","mean");
  endif

  if PLOT_MAG
    figure("name", ["Magnitudes: " csvNames{k}]);
    subplot(2,1,1); hold on; plot(t, gm, "-"); plot(t, smooth(gm, winN), "-", "LineWidth", 1.2); grid on; ylabel("|g| [dps]"); title(["Magnitudes: " csvNames{k}]); legend("raw","mean");
    subplot(2,1,2); hold on; plot(t, am, "-"); plot(t, smooth(am, winN), "-", "LineWidth", 1.2); grid on; ylabel("|a| [g]"); xlabel(xLabelTS); legend("raw","mean");
  endif

  if PLOT_2D_ACCEL
    figure("name", ["Accel 2D projection (ax-ay): " csvNames{k}]);
    plot(ax, ay, "-"); grid on; axis equal; xlabel("ax [g]"); ylabel("ay [g]"); title(["Accel ax-ay: " csvNames{k}]);
  endif

  if PLOT_2D_GYRO
    figure("name", ["Gyro 2D projections: " csvNames{k}]);
    subplot(1,3,1); plot(gx, gy, "-"); grid on; xlabel("gx [dps]"); ylabel("gy [dps]"); title("gx-gy");
    subplot(1,3,2); plot(gy, gz, "-"); grid on; xlabel("gy [dps]"); ylabel("gz [dps]"); title("gy-gz");
    subplot(1,3,3); plot(gz, gx, "-"); grid on; xlabel("gz [dps]"); ylabel("gx [dps]"); title("gz-gx"); axis equal;
  endif

  if PLOT_3D_ACCEL
    figure("name", ["Accelerometer 3D: " csvNames{k}]);
    scatter3(ax, ay, az, 6, t, "filled"); grid on; axis equal; xlabel("ax [g]"); ylabel("ay [g]"); zlabel("az [g]");
    title(["Accelerometer 3D trajectory: " csvNames{k}]); colorbar; colormap jet;
  endif

  if PLOT_3D_GYRO
    figure("name", ["Gyroscope 3D: " csvNames{k}]);
    scatter3(gx, gy, gz, 6, t, "filled"); grid on; axis equal; xlabel("gx [dps]"); ylabel("gy [dps]"); zlabel("gz [dps]");
    title(["Gyroscope 3D trajectory: " csvNames{k}]); colorbar; colormap jet;
  endif

  if PLOT_HIST
    figure("name", ["Gyro histograms: " csvNames{k}]);
    subplot(3,1,1);
    if exist("histogram", "file") == 2
      histogram(gx, 100);
    else
      [counts, centers] = hist(gx, 100);
      bar(centers, counts, 1.0);
    endif
    grid on; ylabel("gx"); title(["Gyro histograms: " csvNames{k}]);
    subplot(3,1,2);
    if exist("histogram", "file") == 2
      histogram(gy, 100);
    else
      [counts, centers] = hist(gy, 100);
      bar(centers, counts, 1.0);
    endif
    grid on; ylabel("gy");
    subplot(3,1,3);
    if exist("histogram", "file") == 2
      histogram(gz, 100);
    else
      [counts, centers] = hist(gz, 100);
      bar(centers, counts, 1.0);
    endif
    grid on; ylabel("gz"); xlabel("Value [dps]");

    figure("name", ["Accel histograms: " csvNames{k}]);
    subplot(3,1,1);
    if exist("histogram", "file") == 2
      histogram(ax, 100);
    else
      [counts, centers] = hist(ax, 100);
      bar(centers, counts, 1.0);
    endif
    grid on; ylabel("ax"); title(["Accel histograms: " csvNames{k}]);
    subplot(3,1,2);
    if exist("histogram", "file") == 2
      histogram(ay, 100);
    else
      [counts, centers] = hist(ay, 100);
      bar(centers, counts, 1.0);
    endif
    grid on; ylabel("ay");
    subplot(3,1,3);
    if exist("histogram", "file") == 2
      histogram(az, 100);
    else
      [counts, centers] = hist(az, 100);
      bar(centers, counts, 1.0);
    endif
    grid on; ylabel("az"); xlabel("Value [g]");
  endif

  if PLOT_PSD
    [fgx, Pgx, fLabel] = simple_psd(gx, FS);
    [fgy, Pgy, ~]      = simple_psd(gy, FS);
    [fgz, Pgz, ~]      = simple_psd(gz, FS);
    figure("name", ["Gyro PSD: " csvNames{k}]);
    subplot(3,1,1); plot(fgx, 10*log10(Pgx + eps)); grid on; ylabel("gx [dB]"); title(["Gyro PSD: " csvNames{k}]);
    subplot(3,1,2); plot(fgy, 10*log10(Pgy + eps)); grid on; ylabel("gy [dB]");
    subplot(3,1,3); plot(fgz, 10*log10(Pgz + eps)); grid on; ylabel("gz [dB]"); xlabel(fLabel);

    [fax, Pax, fLabel2] = simple_psd(ax, FS);
    [fay, Pay, ~]       = simple_psd(ay, FS);
    [faz, Paz, ~]       = simple_psd(az, FS);
    figure("name", ["Accel PSD: " csvNames{k}]);
    subplot(3,1,1); plot(fax, 10*log10(Pax + eps)); grid on; ylabel("ax [dB]"); title(["Accel PSD: " csvNames{k}]);
    subplot(3,1,2); plot(fay, 10*log10(Pay + eps)); grid on; ylabel("ay [dB]");
    subplot(3,1,3); plot(faz, 10*log10(Paz + eps)); grid on; ylabel("az [dB]"); xlabel(fLabel2);
  endif

endfor

function [f, pxx, fLabel] = simple_psd(x, Fs)
  x = x(:);
  x = x - mean(x);
  n = length(x);
  if n <= 1
    if isfinite(Fs) && Fs > 0
      fLabel = "Frequency [Hz]";
    else
      fLabel = "Frequency [cycles/sample]";
    endif
    f = 0; pxx = 0;
    return;
  endif

  if exist("hann", "file") == 2
    w = hann(n);
  else
    w = 0.5 - 0.5*cos(2*pi*((0:n-1)'/(n-1)));
  endif

  xw = x .* w;
  X = fft(xw);
  n2 = floor(n/2);
  X = X(1:n2+1);

  wnorm = sum(w.^2);
  if isfinite(Fs) && Fs > 0
    pxx = (abs(X).^2) / (wnorm * Fs);
    f = (0:n2)' * (Fs / n);
    fLabel = "Frequency [Hz]";
  else
    pxx = (abs(X).^2) / wnorm;
    f = (0:n2)' / n;
    fLabel = "Frequency [cycles/sample]";
  endif
endfunction

function plot_hist_compat(x, nbins)
  if exist("histogram", "file") == 2
    histogram(x, nbins);
  else
    x = x(:);
    if nargin < 2 || isempty(nbins)
      nbins = max(10, min(100, round(sqrt(numel(x)))));
    endif
    [counts, centers] = hist(x, nbins);
    bar(centers, counts, 1.0);
  endif
endfunction
