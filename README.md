# Microcontroller-SmartHome
Mini Project For Microcontroller
Here’s a clear, copy-ready English explanation of your sketch. You can paste this into a text file as project documentation.

# Overview

This ESP32 sketch reads temperature/humidity from a DHT11 sensor, shows live values and time on a 16×2 I²C LCD, logs min/max **average** temperatures into EEPROM, sends data to Blynk (mobile dashboard) and to Google Sheets (via Apps Script), and displays a soil-moisture–style reading from an analog pin. Time comes from an RTC DS1307.

# Hardware & Libraries

* **ESP32**
* **DHT11** on digital pin 13
* **16×2 I²C LCD** at I²C address `0x27`
* **RTC DS1307** (I²C)
* **Analog sensor** on pin 34 (used as “soil humidity” example)
* **Libraries:** `Wire`, `EEPROM`, `WiFi`, `HTTPClient`, `BlynkSimpleEsp32`, `DHT`, `LiquidCrystal_I2C`, `RTClib`

# Cloud & Credentials

* **Blynk**: Uses `BLYNK_TEMPLATE_ID`, `BLYNK_TEMPLATE_NAME`, and `BLYNK_AUTH_TOKEN` to send virtual-pin data (V1, V2, V4).
* **Wi-Fi**: `ssid` and `pass` connect the ESP32 to the network.
* **Google Sheets**: `scriptURL` is a deployed Apps Script web app URL; the sketch calls it with `?temp=…&hum=…`.

# Global State

* `temp`, `hum`: latest DHT readings.
* `tempSum`, `tempCount`: accumulate readings to compute an average.
* `avgMinTemp`, `avgMaxTemp`: running **min/max of the average temperature**; persisted in EEPROM.
* Instances: `DHT dht(...)`, `BlynkTimer timer`, `RTC_DS1307 rtc`, `LiquidCrystal_I2C lcd(...)`.

# Helper Functions

## `checkWiFiConnection()`

If Wi-Fi drops, prints a message, disconnects, and calls `WiFi.reconnect()`.

## `clearEEPROM()`

Zeroes the first 16 bytes of EEPROM and commits. (Handy to reset min/max storage.)

## `sendToExcel()`

Reads DHT, and if Wi-Fi is connected, issues a simple HTTP GET to your Apps Script URL with `temp` and `hum` query parameters—this is how rows end up in Google Sheets.

## `sendSensor()`

* Loops 5 times:

  * Reads DHT; aborts on `NaN`.
  * Accumulates `tempSum` and increments `tempCount`.
  * When `tempCount >= 50`, computes `avgTemp`:

    * Updates `avgMinTemp` / `avgMaxTemp` if `avgTemp` breaks records.
    * Stores them in EEPROM at addresses 0 (min) and 4 (max), then commits.
    * Resets the accumulator.
  * Calls `display_sensor(...)` to refresh LCD and Blynk.

**Note:** The “records” are for the **average of batches of readings**, not single samples.

## `display_sensor(float &temp, float &hum, float &avgMinTemp, float &avgMaxTemp)`

* Clears the LCD and prints:

  * Line 1: `T:<temp>C H:<hum>%`
  * Line 2: `AN:<avgMinTemp> AX:<avgMaxTemp>` (AN = average min, AX = average max)
* Sends current `temp` to **Blynk V1** and `hum` to **Blynk V2**.
* Waits 1 second (`delay(1000)`).

## `humidity_in_missouri()`

* Reads analog value on pin 34.
* Converts 0–4095 to a percentage and inverts it: `100 - raw*100/4095` (a simple “wetness” guess).
* Sends to **Blynk V4**.

## `sendTime()`

* Loops 5 times:

  * Gets current `DateTime now = rtc.now()`.
  * Clears LCD and prints:

    * Line 1: `Date: YYYY/MM/DD` (uses `printTwoDigits` for padding)
    * Line 2: `Time: HH:MM:SS`
  * Waits 1 second.

## `printTwoDigits(int number)`

* Prints a leading zero if `< 10`, then prints the number (for neat time/date formatting).

# `setup()`

1. Initialize EEPROM (16 bytes) and serial.
2. Start I²C (`Wire`), initialize LCD + backlight, start DHT.
3. Connect to Wi-Fi; during connection prints dots on LCD.
4. Confirm LCD shows “WiFi Connected!” briefly.
5. Initialize RTC:

   * If not found: prints error and calls `abort()`.
   * If not running: sets RTC time to the sketch compile time.
6. Load `avgMinTemp` and `avgMaxTemp` from EEPROM (addresses 0 and 4). If invalid/zero, set safe defaults (100 and −100).
7. Start Blynk with `auth`, `ssid`, `pass`.
8. Set up `BlynkTimer` intervals:

   * Every **1,000 ms**: `humidity_in_missouri`
   * Every **5,000 ms**: `sendTime`
   * Every **5,000 ms**: `sendSensor`
   * Every **5,000 ms**: `sendToExcel`

# `loop()`

* Keeps the system alive:

  * `checkWiFiConnection()`
  * `Blynk.run()` (handles cloud communication)
  * `timer.run()` (executes scheduled tasks)

# Virtual Pin Map (Blynk)

* **V1**: Temperature (°C)
* **V2**: Humidity (%)
* **V4**: Analog “soil moisture” style percentage

# EEPROM Layout (first 8 bytes)

* **Address 0–3**: `avgMinTemp` (float)
* **Address 4–7**: `avgMaxTemp` (float)

# Notes & Tips

* **Blocking delays:** `display_sensor()` and `sendTime()` both use `delay(1000)` inside functions that are called by timers—this can block other tasks briefly. Consider non-blocking updates if you need smoother timing.
* **Averaging window:** Min/Max are based on the **average after 50 samples** (not one reading). Adjust `tempCount` threshold to change the averaging window.
* **NaN handling:** If DHT returns `NaN`, the function exits early; you’ll see “Failed to read from DHT sensor!” in Serial.
* **Analog scaling:** The soil-moisture formula is a simple linear invert; real sensors often need calibration.
* **Security:** Replace all placeholders (`BLYNK_*`, Wi-Fi, script URL) with your real values; avoid committing secrets to public repos.
* **RTC time base:** If the DS1307 loses power and stops, `setup()` will reseed it to the compile time of this firmware.

If you want, I can also generate this as a downloadable `.txt` file exactly as above—just say “save it as a text file”.
