function [f, pxx, fLabel] = simple_psd(x, Fs)
  % SIMPLE_PSD  Minimal PSD/periodogram-like estimator compatible with
  % Matlab/Octave without extra toolboxes.
  %   [f, pxx, fLabel] = simple_psd(x, Fs)
  %   - x: signal vector
  %   - Fs: sampling frequency [Hz]; if not known, set to NaN or <= 0
  %
  % Returns:
  %   f      - frequency axis
  %   pxx    - power spectral density estimate
  %   fLabel - label for x-axis (Hz or cycles/sample)

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
