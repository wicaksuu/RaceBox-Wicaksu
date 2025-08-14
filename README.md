# ESP32-Cheap Yellow Display (ESP32-2432S028R) Pinout

## 1. Display (SPI — HSPI)

- **TFT_MISO (MISO)**: GPIO 12
- **TFT_MOSI (MOSI)**: GPIO 13
- **TFT_SCLK (SCLK)**: GPIO 14
- **TFT_CS (CS)**: GPIO 15
- **TFT_DC (DC)**: GPIO 2
- **TFT_RST (RST)**: –1 (unused)
- **Backlight**: GPIO 21

## 2. Touchscreen (SPI — VSPI)

- **XPT2046_IRQ (IRQ)**: GPIO 36
- **XPT2046_MOSI (MOSI)**: GPIO 32
- **XPT2046_MISO (MISO)**: GPIO 39
- **XPT2046_CLK (CLK)**: GPIO 25
- **XPT2046_CS (CS)**: GPIO 33

## 3. RGB LED (Active-Low)

- **Red LED**: GPIO 4
- **Green LED**: GPIO 16
- **Blue LED**: GPIO 17  
  _(Catatan: LED aktif saat pin diset LOW)_

## 4. microSD Card (SPI — VSPI)

- **MISO**: GPIO 19
- **MOSI**: GPIO 23
- **SCK**: GPIO 18
- **CS**: GPIO 5

## 5. LDR (Sensor Cahaya)

- **LDR**: GPIO 34

## 6. Speaker

- **Speaker (JST 2-pin)**: GPIO 26

## 7. Buttons

- **BOOT Button**: GPIO 0
- **RST (Reset)**: on-board Reset button

## 8. Extended GPIO (P3 & CN1)

- **P3**: GND, GPIO 35, GPIO 22, GPIO 21 (Backlight)
- **CN1**: GND, GPIO 22, GPIO 27, 3V3  
  _(Catatan: gunakan custom I2C pins karena SDA default di GPIO 21 digunakan untuk backlight)_

## 9. TX/RX Serial (P1 Connector)

- **TX**: GPIO 1
- **RX**: GPIO 3

## 10. GPS

- **TX**: GPIO 27
- **RX**: GPIO 22
