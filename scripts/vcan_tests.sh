#!/bin/sh
# Sets up (best-effort) a vcan0 interface and runs the given SocketCanStream test
# binary against it. Requires CAP_NET_ADMIN for the `ip link` step to succeed; the
# vcan kernel module itself normally needs to be loaded on the host first (a plain
# container can't load a brand-new kernel module) -- see README.md.
set -e

if command -v modprobe >/dev/null 2>&1; then
    modprobe vcan 2>/dev/null || true
fi

if ! ip link show vcan0 >/dev/null 2>&1; then
    ip link add dev vcan0 type vcan
    ip link set up vcan0
fi

exec "$1"
