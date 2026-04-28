#!/usr/bin/env bash
# OTA-flash every board listed in BOARDS in parallel.
# Edit the IPs below after the home setup so they match your three Arduinos.
set -e

BOARDS=(
  192.168.0.102
  192.168.0.103
  192.168.0.104
)

FQBN="arduino:renesas_uno:unor4wifi"
BUILD_DIR="/tmp/arduinoota-test-build"
BIN="$BUILD_DIR/ArduinoOTA_Test.ino.bin"
USER_PASS="arduino:password"

cd "$(dirname "$0")"

echo "Compiling..."
arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD_DIR" .

if [[ ! -f "$BIN" ]]; then
  echo "Build artifact not found at $BIN" >&2
  exit 1
fi

echo "Uploading to ${#BOARDS[@]} boards in parallel..."
pids=()
for ip in "${BOARDS[@]}"; do
  (
    if curl -sS --fail --user "$USER_PASS" \
        --data-binary @"$BIN" \
        "http://$ip:65280/sketch" > "/tmp/ota-$ip.log" 2>&1; then
      echo "  OK   $ip"
    else
      echo "  FAIL $ip (see /tmp/ota-$ip.log)"
    fi
  ) &
  pids+=($!)
done

for pid in "${pids[@]}"; do
  wait "$pid" || true
done

echo "Done."
