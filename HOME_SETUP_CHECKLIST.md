# Home Setup Checklist — 3 Arduinos

## Prep (once)
- [ ] Confirm all three UNO R4 WiFi boards are on hand, plus a USB-C cable.
- [ ] On your Mac, confirm `arduino-cli` works: `arduino-cli version`.
- [ ] Copy the secrets template and fill it in:
  ```bash
  cp Secrets.h.example Secrets.h
  ```
  Then edit `Secrets.h` and set `WIFI_SSID`, `WIFI_PASS`, `OTA_PASSWORD`, and (optionally) `OTA_HOSTNAME` to real values. `Secrets.h` is gitignored — your credentials never get committed.
- [ ] Compile once to verify the build is clean:
  ```bash
  arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --build-path /tmp/arduinoota-test-build .
  ```

## Board #1 (the one already running OTA)
- [ ] OTA upload the new sketch (export `OTA_PASSWORD` first to match the value in `Secrets.h`):
  ```bash
  export OTA_PASSWORD='your-ota-password-here'
  curl --user "arduino:$OTA_PASSWORD" --data-binary @/tmp/arduinoota-test-build/ArduinoOTA_Test.ino.bin http://192.168.0.102:65280/sketch
  ```
  (Replace IP if Serial showed a different one. Close Serial Monitor before uploading.)
- [ ] Plug USB, open Serial at `115200`. Confirm:
  - `OTA ready.`
  - `Node id: <number>`
  - `Sync UDP port: 4210`
  - An IP address.
- [ ] Note **Board #1's IP** here: `__________________`

## Board #2 (USB first time)
- [ ] Unplug Board #1, plug Board #2 via USB.
- [ ] Pick the new serial port: `arduino-cli board list`
- [ ] Upload over USB:
  ```bash
  arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi --port /dev/cu.usbmodemXXXX --input-dir /tmp/arduinoota-test-build .
  ```
- [ ] Open Serial Monitor at `115200`. Confirm `OTA ready.` and capture the IP.
- [ ] Note **Board #2's IP** here: `__________________`

## Board #3 (USB first time)
- [ ] Unplug Board #2, plug Board #3 via USB.
- [ ] Repeat the same `arduino-cli upload` command (new port name).
- [ ] Confirm Serial shows `OTA ready.`
- [ ] Note **Board #3's IP** here: `__________________`

## Verify the cluster
- [ ] Plug all three boards into power (USB or wall adapters). Wait ~5 seconds for WiFi join.
- [ ] On your Mac, watch broadcasts: `nc -ul 4210`
  - You should see one `INST1 HB <id> <seq> <uptime>` line per board per second — three different `<id>`s.
  - `Ctrl+C` to stop.
- [ ] Open `led_message_controller.html` in a browser.
- [ ] Enter Board #1's IP, hit Tab (so the peers panel starts polling).
  - "This board" line shows Board #1's id.
  - Peers list shows two other ids with green dots.
- [ ] Type `HELO` → click **Send To Matrix**.
  - All three matrices should show `HELO` within a frame.
- [ ] Change the IP field to Board #2, send a different message.
  - All three matrices update again. Confirms fan-out works from any board.
- [ ] Power-cycle Board #3.
  - Within ~3 seconds the peer list drops to one entry.
  - When it rejoins, the dot turns green again.

## OTA from now on
- [ ] Once all three IPs are known, edit the `BOARDS=(...)` list at the top of [ota-all.sh](ota-all.sh) so it matches.
- [ ] From then on, push to all three boards in parallel with one command:
  ```bash
  ./ota-all.sh
  ```
  It compiles, then flashes every board in `BOARDS` simultaneously and prints `OK` / `FAIL` per IP. Per-board failure logs go to `/tmp/ota-<ip>.log`.
- [ ] To flash just one board (e.g. after a code experiment), still use:
  ```bash
  arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --build-path /tmp/arduinoota-test-build .
  curl --user "arduino:$OTA_PASSWORD" --data-binary @/tmp/arduinoota-test-build/ArduinoOTA_Test.ino.bin http://<BOARD_IP>:65280/sketch
  ```

## If something goes wrong
- **No IP on Serial** → WiFi DHCP didn't lease. Reset the board; check SSID/password.
- **`nc -ul 4210` shows nothing** → boards aren't on the same subnet, or another listener is bound to 4210. Run `lsof -nP -iUDP:4210` to check.
- **Peers panel shows 0** → only one board is on the network, or one board's WiFi dropped. Check Serial on the missing one.
- **OTA `curl` hangs** → wrong IP, or Serial Monitor is open and holding the port. Close Serial and retry.
