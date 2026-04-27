# Arduino OTA Upload Notes

This folder contains a test sketch for Arduino UNO R4 WiFi OTA uploads.

## What We Debugged

The first WiFi test printed:

```text
Connected!
IP: 0.0.0.0
```

That meant the board had associated with the WiFi access point, but DHCP had not produced a usable IP address yet. The sketch now waits until `WiFi.localIP()` is no longer `0.0.0.0` before reporting success or starting OTA.

The working serial output looks like:

```text
Starting WiFi test
WiFi firmware: 0.6.0
.
Connected with IP address.
OTA ready.
WiFi status: 3
SSID: Lucy
RSSI: -38 dBm
IP: 192.168.0.102
```

`WiFi status: 3` means `WL_CONNECTED`. `OTA ready` means the board is listening for OTA uploads on TCP port `65280`.

## Important Workflow

Use USB for:

- Serial Monitor
- Recovery if OTA fails
- Uploading a sketch that does not contain OTA support

Use WiFi OTA for:

- Uploading a compiled `.bin` file to the board over the local network

Close Serial Monitor before uploading.

Every sketch uploaded over OTA must keep OTA support in it, including:

```cpp
ArduinoOTA.begin(WiFi.localIP(), "ArduinoOTA_Test", "password", InternalStorage);
```

and this in `loop()`:

```cpp
ArduinoOTA.poll();
```

If you upload a sketch without `ArduinoOTA.poll()`, OTA will stop working and you will need USB again.

## Arduino IDE Issue

The Arduino IDE OTA upload path crashed with:

```text
Failed uploading: uploading error: signal: segmentation fault
```

The sketch compiled successfully, so this was not a sketch compile problem. The board's OTA listener was reachable, but the Arduino IDE / `arduinoOTA` upload tool path crashed.

To support IDE network uploads, this file was copied into the UNO R4 board package:

```text
/Users/tate.carson/Library/Arduino15/packages/arduino/hardware/renesas_uno/1.5.3/platform.local.txt
```

Source file:

```text
/Users/tate.carson/Documents/Arduino/libraries/ArduinoOTA/extras/renesas/platform.local.txt
```

Even after that, the IDE upload path still segfaulted, so the working method is direct upload with `curl`.

## Upload Without Arduino IDE

First, confirm the board is running the OTA sketch:

1. Plug in USB.
2. Open Serial Monitor at `115200`.
3. Press reset if needed.
4. Confirm the board prints `OTA ready` and an IP address.
5. Close Serial Monitor before uploading.

Then compile:

```bash
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --build-path /tmp/arduinoota-test-build .
```

Then upload with `curl`, replacing the IP address if Serial Monitor shows a different one:

```bash
curl --user arduino:password --data-binary @/tmp/arduinoota-test-build/ArduinoOTA_Test.ino.bin http://192.168.0.102:65280/sketch
```

A successful upload returns:

```text
OK
```

The board may reset or pause briefly while applying the update.

## Change The LED Matrix Message Over WiFi

The sketch also starts a small web server on port `80`. Once Serial Monitor shows `OTA ready`, you can change the LED matrix readout from a browser:

```text
http://192.168.0.102/msg?text=TATE
```

Replace `192.168.0.102` with the IP address printed by the board.

You can also send the message from Terminal:

```bash
curl "http://192.168.0.102/msg?text=HI"
```

Or open the local controller page in this folder:

```text
led_message_controller.html
```

Enter the board IP address and a short message, then click "Send To Matrix". The page sends the message to the Arduino with:

```text
http://BOARD_IP/msg?text=TEXT
```

The matrix is only 12 columns wide, so the sketch uses a compact font and accepts up to 4 letters or numbers. Lowercase letters are converted to uppercase.

## Password

The OTA username is:

```text
arduino
```

The OTA password currently used by the sketch is:

```text
password
```

That comes from this line in the sketch:

```cpp
ArduinoOTA.begin(WiFi.localIP(), "ArduinoOTA_Test", "password", InternalStorage);
```

For a real project, change `"password"` to a better password and upload once over USB or over the working `curl` OTA method.

## Multi-Board UDP Sync

When two or more boards run this sketch on the same WiFi, they coordinate over UDP broadcast on port `4210`. No configuration: every board listens, every board broadcasts.

What syncs:

- **Message** — submitting the form on any board updates all of them.
- **Heartbeat** — each board sends one packet per second (presence + clock).
- **Events** — `broadcastEvent("FLASH")` from any board calls `onSyncEvent()` on the others. The hook is in the sketch, ready for installation cues.
- **Peers** — the web page shows other live boards by node id and IP.

Packet format is plain text, one line:

```text
INST1 <type> <originId> <seq> <payload>
```

Types are `MSG`, `EVT`, `HB`. Receivers dedupe by `(originId, seq)` and never rebroadcast, so there are no echo storms.

Snoop traffic from a laptop on the same LAN:

```bash
nc -ul 4210
```

You should see one `INST1 HB ...` line per board per second.

## Troubleshooting

If `curl` cannot connect:

- Confirm the board is powered.
- Confirm Serial Monitor shows `OTA ready`.
- Confirm the IP address has not changed.
- Make sure the computer and Arduino are on the same WiFi network.
- Close Serial Monitor before OTA upload.

If OTA stops working:

- Reconnect USB.
- Select the USB serial port.
- Upload a sketch that includes OTA support.
- Open Serial Monitor and verify `OTA ready`.

If Arduino IDE says no monitor is available for a network port:

- That is expected.
- Network/OTA ports are for upload only.
- Use the USB port for Serial Monitor.
