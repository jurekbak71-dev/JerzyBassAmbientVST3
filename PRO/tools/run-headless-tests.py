"""Run CTest under xvfb-run without a window manager (Linux development only).

JUCE uses existing WM/EWMH atoms for native dialog peers. A bare Xvfb has no WM
that creates these atoms. Keep one X11 connection open while CTest runs, so that
actual dialog peers can be exercised instead of skipping their GUI tests.
Usage: xvfb-run -a python PRO/tools/run-headless-tests.py build-pro
"""
import ctypes
import ctypes.util
import subprocess
import sys

x11 = ctypes.CDLL(ctypes.util.find_library("X11"))
x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
x11.XInternAtom.restype = ctypes.c_ulong
x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
display = x11.XOpenDisplay(None)
if not display:
    raise SystemExit("No X display; run this helper through xvfb-run.")
try:
    for atom in ("WM_PROTOCOLS", "WM_TAKE_FOCUS", "WM_DELETE_WINDOW", "WM_CHANGE_STATE", "WM_STATE",
                 "_NET_WM_PING", "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_NORMAL", "_NET_WM_WINDOW_TYPE_DIALOG",
                 "_NET_WM_STATE", "_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_MAXIMIZED_HORZ", "_NET_WM_STATE_MAXIMIZED_VERT"):
        x11.XInternAtom(display, atom.encode(), False)
    raise SystemExit(subprocess.call(["ctest", "--test-dir", sys.argv[1], "--output-on-failure"]))
finally:
    x11.XCloseDisplay(display)
