from typing import Tuple
import numpy as np


def simple_psd(x: np.ndarray, fs: float = None) -> Tuple[np.ndarray, np.ndarray, str]:
    """
    Simple PSD/periodogram estimator.

    Args:
        x: Signal vector
        fs: Sampling frequency in Hz. If None, returns normalized frequency.

    Returns:
        f: Frequency axis
        pxx: Power spectral density estimate
        f_label: Label for frequency axis
    """
    x = np.asarray(x).flatten()
    x = x - np.mean(x)
    n = len(x)

    if n <= 1:
        f_label = "Frequency [Hz]" if fs and fs > 0 else "Frequency [cycles/sample]"
        return np.array([0]), np.array([0]), f_label

    w = 0.5 - 0.5 * np.cos(2 * np.pi * np.arange(n) / (n - 1))

    xw = x * w
    X = np.fft.fft(xw)
    n2 = n // 2
    X = X[: n2 + 1]

    wnorm = np.sum(w**2)

    if fs and fs > 0:
        pxx = (np.abs(X) ** 2) / (wnorm * fs)
        f = np.arange(n2 + 1) * (fs / n)
        f_label = "Frequency [Hz]"
    else:
        pxx = (np.abs(X) ** 2) / wnorm
        f = np.arange(n2 + 1) / n
        f_label = "Frequency [cycles/sample]"

    return f, pxx, f_label
