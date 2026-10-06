#!/usr/bin/env bash
#
# benchmark_firewall_read.sh
#
# Copyright (C) 2026, Charles Chiou
#

set -euo pipefail

CLIENT_BIN="./build/netmon-ai-client"
if [[ ! -x "${CLIENT_BIN}" ]]; then
    if [[ -x "./netmon-ai-client" ]]; then
        CLIENT_BIN="./netmon-ai-client"
    else
        echo "Compiling netmon-ai-client..."
        gcc -std=c99 -Wall -Wextra -pedantic -O2 client/netmon-ai-client.c -o build/netmon-ai-client
        CLIENT_BIN="./build/netmon-ai-client"
    fi
fi

echo "================================================================================"
echo " NetMon AI Firewall Read Latency & Pipelining Benchmark"
echo "================================================================================"

# Verify daemon status
STATUS_OUT=$("${CLIENT_BIN}" status 2>&1 || true)
echo "Client Session: ${STATUS_OUT}"

if ! echo "${STATUS_OUT}" | grep -q "state=GRANTED"; then
    echo "ERROR: Client session is not in GRANTED state."
    echo "Run '${CLIENT_BIN} request R' and grant on netmon console before running benchmark."
    exit 1
fi

echo ""
echo "--- Benchmark 1: Pipelined 21-Rule Read (Threshold: <= 3.0s) ---"
START_TIME=$(date +%s%N)

# Pipeline all 21 security policies
"${CLIENT_BIN}" do \
    "show secure-policy 1" \
    "show secure-policy 2" \
    "show secure-policy 3" \
    "show secure-policy 4" \
    "show secure-policy 5" \
    "show secure-policy 6" \
    "show secure-policy 7" \
    "show secure-policy 8" \
    "show secure-policy 9" \
    "show secure-policy 10" \
    "show secure-policy 11" \
    "show secure-policy 12" \
    "show secure-policy 13" \
    "show secure-policy 14" \
    "show secure-policy 15" \
    "show secure-policy 16" \
    "show secure-policy 17" \
    "show secure-policy 18" \
    "show secure-policy 19" \
    "show secure-policy 20" \
    "show secure-policy 21" > /dev/null

END_TIME=$(date +%s%N)
ELAPSED_NS=$((END_TIME - START_TIME))
ELAPSED_MS=$((ELAPSED_NS / 1000000))
AVG_MS_PER_RULE=$((ELAPSED_MS / 21))

echo "Elapsed Time: ${ELAPSED_MS} ms total (Average: ${AVG_MS_PER_RULE} ms/rule)"

if [[ ${ELAPSED_MS} -gt 3000 ]]; then
    echo "VERDICT: FAILED (Exceeded 3000ms threshold)"
    exit 1
else
    echo "VERDICT: PASSED (Well within <= 3000ms threshold)"
fi

echo ""
echo "--- Benchmark 2: Complete Firewall Diagnostic Inventory (Threshold: <= 5.0s) ---"
START_TIME_FULL=$(date +%s%N)

"${CLIENT_BIN}" do \
    "show version" \
    "show cpu status" \
    "show mem status" \
    "show conn status" \
    "show ip route-settings" \
    "show zone" \
    "show ip virtual-server" > /dev/null

END_TIME_FULL=$(date +%s%N)
ELAPSED_NS_FULL=$((END_TIME_FULL - START_TIME_FULL))
ELAPSED_MS_FULL=$((ELAPSED_NS_FULL / 1000000))

echo "Elapsed Time: ${ELAPSED_MS_FULL} ms total for 7 system domains"

if [[ ${ELAPSED_MS_FULL} -gt 5000 ]]; then
    echo "VERDICT: FAILED (Exceeded 5000ms threshold)"
    exit 1
else
    echo "VERDICT: PASSED (Well within <= 5000ms threshold)"
fi

echo ""
echo "================================================================================"
echo " All Firewall Read Performance Benchmarks Passed Successfully!"
echo "================================================================================"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# tab-width: 4
# indent-tabs-mode: nil
# End:
