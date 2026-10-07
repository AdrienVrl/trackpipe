#!/usr/bin/env bash
# Phase 0 checks, run inside the dev container: verify-env
# Prints OK / WARN / FAIL per item; never stops at the first failure.

ok()   { printf '  \e[32mOK\e[0m    %s\n' "$*"; }
warn() { printf '  \e[33mWARN\e[0m  %s\n' "$*"; }
fail() { printf '  \e[31mFAIL\e[0m  %s\n' "$*"; }

echo "Toolchain"
for t in gcc g++ clang++ clang-tidy cmake ninja gdb; do
    if command -v "$t" >/dev/null; then ok "$t: $("$t" --version | head -n1)"; else fail "$t missing"; fi
done

echo "GPU"
if nvidia-smi --query-gpu=name,driver_version --format=csv,noheader >/tmp/gpu 2>/dev/null; then
    ok "GPU: $(cat /tmp/gpu)"
else
    fail "nvidia-smi failed (container started without --gpus all, or toolkit not installed in WSL)"
fi
if command -v nvcc >/dev/null; then ok "nvcc: $(nvcc --version | tail -n1)"; else fail "nvcc not on PATH"; fi
TRTEXEC=$(command -v trtexec || find / -name trtexec -type f 2>/dev/null | head -n1)
if [ -n "$TRTEXEC" ]; then ok "trtexec: $TRTEXEC"; else fail "trtexec not found"; fi

echo "Display (WSLg)"
if [ -n "${DISPLAY:-}" ] && [ -S /tmp/.X11-unix/X0 ]; then ok "DISPLAY=$DISPLAY, X11 socket present"
else warn "no X11 socket or DISPLAY; OpenCV windows will not open"; fi

echo "Devices"
if [ -e /dev/ttyACM0 ]; then
    if [ -w /dev/ttyACM0 ]; then ok "/dev/ttyACM0 writable"; else fail "/dev/ttyACM0 present but not writable (dialout group?)"; fi
else warn "/dev/ttyACM0 absent (attach the Nucleo with usbipd, then add --device and rebuild)"; fi
if [ -e /dev/video0 ]; then ok "/dev/video0 present"; else warn "/dev/video0 absent (use the file or UDP source)"; fi

echo "Real-time"
if [ "$(ulimit -r)" -gt 0 ] 2>/dev/null; then ok "rtprio limit: $(ulimit -r)"; else warn "rtprio limit is 0 (SCHED_FIFO will fail)"; fi
