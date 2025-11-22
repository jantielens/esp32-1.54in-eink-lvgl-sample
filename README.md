# ESP32 E-Paper Display with LVGL Sample

A complete sample project demonstrating how to use the **WeAct Studio E-Paper Module 1.54"** with **ESP32** and **LVGL** (Light and Versatile Graphics Library). This project shows best practices for integrating e-ink displays with modern UI frameworks on embedded devices.

## 🖼️ What This Sample Demonstrates

<table>
<tr>
<td width="50%">

- **E-ink Display Integration**: Using GxEPD2 library with 1.54" 200x200 monochrome e-paper
- **LVGL UI Framework**: Modern graphics library with efficient rendering for constrained devices
- **Smart Refresh Strategy**: Balancing partial updates for speed with periodic full refreshes to prevent ghosting
- **Memory-Efficient Design**: Optimized buffer management for ESP32's limited RAM
- **Low Power Techniques**: Manual refresh control instead of continuous polling
- **Production-Ready Code**: Well-commented, easy to understand and extend

</td>
<td width="50%">

![Demo Animation](docs/assets/demo.gif)

</td>
</tr>
</table>

## 📦 Hardware Requirements

### Essential Components

| Component | Specification | Notes |
|-----------|--------------|-------|
| **ESP32 Dev Board** | ESP32 Dev Module or compatible | Any ESP32 with SPI support |
| **E-Paper Display** | WeAct Studio 1.54" (GDEH0154D67) | 200×200, SSD1681 controller |
| **Wiring** | Jumper wires or direct connection | Keep SPI wires short (<15cm) |
| **USB Cable** | USB-A to Micro-USB or USB-C | For programming and power |

Compatible displays: 1.54", 2.13", 2.9", 3.7", 4.2" (see [pinout docs](docs/pinout.md))

## 🔌 Wiring Guide

Connect the e-paper display to ESP32 using these pins:

| Display Pin | ESP32 GPIO | Function |
|-------------|------------|----------|
| VCC | 3.3V | Power ⚠️ **3.3V only!** |
| GND | GND | Ground |
| SDA (MOSI) | GPIO 23 | SPI Data |
| SCL (SCK) | GPIO 18 | SPI Clock |
| CS | GPIO 5 | Chip Select |
| D/C | GPIO 16 | Data/Command |
| RES | GPIO 17 | Reset |
| BUSY | GPIO 4 | Busy Signal |

**⚠️ CRITICAL**: Use 3.3V for VCC, **NOT 5V**! 5V will damage the e-paper display.

📘 See [docs/pinout.md](docs/pinout.md) for detailed wiring diagrams, pin explanations, and troubleshooting.

## 🚀 Quick Start

### Prerequisites

- Linux or WSL2 environment (Windows users: see [WSL setup guide](docs/wsl-development.md))
- USB connection to ESP32
- Bash shell

### 1. Clone and Setup

```bash
git clone https://github.com/jantielens/ideal-robot.git
cd ideal-robot
./setup.sh
```

The setup script will:
- Download and install `arduino-cli` locally
- Install ESP32 board support
- Install required libraries (GxEPD2, LVGL, Adafruit GFX)

### 2. Build and Upload

```bash
# Compile the firmware
./build.sh

# Upload to ESP32 (auto-detects serial port)
./upload.sh

# View serial output
./monitor.sh
```

### 3. What You'll See

The display will show:
- "Hello LVGL!" title
- ESP32 chip information (model, CPU frequency)
- An incrementing counter that updates every 2.5 seconds
- A progress bar that fills from 0-10 and repeats

Serial output will show:
```
=== ESP32 E-INK + LVGL Demo ===
Chip Model: ESP32-D0WD-V3
CPU Frequency: 240 MHz
Flash Size: 4 MB

[E-INK] Initializing display...
[LVGL] Initializing...
[UI] Creating interface...
[READY] System initialized

[UPDATE] Refresh #1 | Free Heap: 245628 bytes
[UPDATE] Refresh #2 | Free Heap: 245628 bytes
...
```

## 📁 Project Structure

```
ideal-robot/
├── src/
│   ├── app/
│   │   ├── app.ino           # Main application (UI creation, update logic)
│   │   ├── eink_display.h    # Display driver interface
│   │   └── eink_display.ino  # E-ink + LVGL integration layer
│   └── version.h              # Firmware version tracking
├── docs/
│   ├── pinout.md             # Wiring guide and pin explanations
│   ├── extending.md          # Tips for customizing this sample
│   ├── scripts.md            # Build script documentation
│   ├── library-management.md # Managing Arduino libraries
│   └── wsl-development.md    # Windows/WSL setup guide
├── lv_conf.h                 # LVGL configuration (optimized for e-ink)
├── arduino-libraries.txt     # Library dependencies
├── build.sh                  # Compile firmware
├── upload.sh                 # Upload to ESP32
├── monitor.sh                # Serial console
└── README.md                 # This file
```

## 🎨 How It Works

### E-ink Display Basics

E-ink displays are fundamentally different from LCD/OLED:

- **Bistable**: Pixels retain state without power (perfect for battery operation)
- **Slow Refresh**: Full updates take 2-5 seconds
- **Partial Updates**: Faster (0.5-2s) but can cause ghosting over time
- **Monochrome**: This sample uses black and white (1-bit color)

### Refresh Strategy

The code uses a hybrid approach:

```cpp
// Update every 5 seconds
if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
  update_counter++;
  
  // Partial refresh for speed (4 times)
  // Full refresh every 5th update to clear ghosting
  eink_force_full_refresh();
  
  lv_refr_now(eink_get_display());
}
```

This balances:
- **Speed**: Partial updates are 4× faster
- **Quality**: Periodic full refreshes prevent ghosting artifacts
- **Power**: Only refresh when needed (not continuously)

### LVGL Integration

LVGL provides a modern UI framework with:

- **I1 Color Format**: 1-bit-per-pixel for monochrome displays
- **Partial Rendering**: Only redraw changed areas
- **Memory Efficiency**: 50-line buffer (10KB) instead of full framebuffer (5KB saved)
- **Manual Refresh**: We control when displays updates (not automatic timer)

### Memory Management

ESP32 has limited RAM (~320KB). This sample:

- Uses **10KB LVGL buffer** (50 lines × 200 pixels ÷ 8 bits)
- Uses **64KB LVGL heap** for UI objects
- Leaves **~240KB free** for your code
- Stack and global variables use remaining DRAM

## 🛠️ Available Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| `setup.sh` | Install toolchain and libraries | Run once during setup |
| `build.sh` | Compile the firmware | `./build.sh` |
| `upload.sh` | Upload to ESP32 | `./upload.sh [port]` |
| `upload-erase.sh` | Erase flash memory | `./upload-erase.sh [port]` |
| `monitor.sh` | View serial output | `./monitor.sh [port] [baud]` |
| `clean.sh` | Remove build artifacts | `./clean.sh` |
| `library.sh` | Manage libraries | `./library.sh [command]` |

📘 See [docs/scripts.md](docs/scripts.md) for detailed script documentation.

## 🔧 Customization Guide

### Change Update Interval

In `src/app/app.ino`:

```cpp
const unsigned long UPDATE_INTERVAL = 2500; // milliseconds (2.5 seconds)
```

### Adjust Refresh Strategy

In `src/app/eink_display.h`:

```cpp
#define MAX_PARTIAL_UPDATES 50  // Full refresh every N flush calls
```

**Note**: This counts e-ink flush operations, not user update cycles. With multiple widgets (e.g., counter + progress bar), each update may trigger 2 flushes, so 50 flushes = ~25 update cycles.

Lower number = less ghosting but slower overall. Higher = faster but more ghosting. Modern displays can often handle 100+ partial updates before visible ghosting appears.

### Change Display Size

1. Update pin definitions if needed (some displays use different pins)
2. Change display constructor in `eink_display.ino`:

```cpp
// For 2.13" display:
GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
  GxEPD2_213_BN(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY)
);
```

3. Update `SCREEN_WIDTH` and `SCREEN_HEIGHT` in `eink_display.h`

### Create Custom UI

LVGL makes it easy to create interfaces. Example adding a button:

```cpp
// In create_ui() function
lv_obj_t *btn = lv_button_create(scr);
lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);

lv_obj_t *btn_label = lv_label_create(btn);
lv_label_set_text(btn_label, "Click Me");
lv_obj_center(btn_label);
```

📘 See [docs/extending.md](docs/extending.md) for more examples and patterns.

## 💡 Tips for Extension

### Add Sensors

```cpp
#include <Adafruit_BME280.h>  // Example: temperature sensor

Adafruit_BME280 bme;

void setup() {
  // ... existing setup ...
  bme.begin(0x76);
}

void loop() {
  if (update time) {
    float temp = bme.readTemperature();
    String temp_text = String(temp, 1) + " C";
    lv_label_set_text(label_temperature, temp_text.c_str());
    // ... refresh display ...
  }
}
```

### Power Optimization

For battery operation, add deep sleep between updates:

```cpp
#include <esp_sleep.h>

void loop() {
  // Update display
  lv_refr_now(eink_get_display());
  
  // Sleep for 60 seconds
  esp_sleep_enable_timer_wakeup(60 * 1000000ULL);  // microseconds
  esp_deep_sleep_start();
}
```

This can reduce average current from 5mA to <50µA!

### WiFi Integration

```cpp
#include <WiFi.h>

void setup() {
  // ... existing setup ...
  WiFi.begin("SSID", "password");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  // Fetch data from API, update display
}
```

📘 More examples in [docs/extending.md](docs/extending.md)

## 🧪 Testing & CI/CD

GitHub Actions automatically validates builds on every push:

- Compiles firmware for ESP32
- Caches arduino-cli and libraries
- Uploads build artifacts (.bin and .elf files)

See `.github/workflows/build.yml` for pipeline configuration.

## 🐛 Troubleshooting

### Display Shows Nothing

1. **Check power**: Verify VCC is 3.3V (measure with multimeter)
2. **Check wiring**: All 8 connections must be correct
3. **Check BUSY pin**: Should go LOW after initialization
4. **Try full reset**: Run `./upload-erase.sh` then `./upload.sh`

### Ghosting/Artifacts

1. **Reduce partial updates**: Lower `MAX_PARTIAL_UPDATES` to 3 or 2
2. **Check power**: Weak power causes refresh issues
3. **Check ground**: Ensure solid GND connection

### Upload Fails: Permission Denied

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Logout and login, or restart WSL
wsl --terminate Ubuntu  # (in PowerShell)
```

### Display Updates Too Slow

1. **E-ink is inherently slow**: 2-5s full refresh is normal
2. **Use partial updates**: Already enabled in this sample
3. **Reduce update frequency**: Increase `UPDATE_INTERVAL`
4. **Consider faster display tech**: OLED/TFT for interactive UIs

📘 More troubleshooting in [docs/pinout.md](docs/pinout.md#troubleshooting)

## 📚 Documentation

- **[Pinout Guide](docs/pinout.md)** - Wiring, pin functions, compatible displays
- **[Extending Guide](docs/extending.md)** - Customization tips and examples
- **[Script Reference](docs/scripts.md)** - Detailed build script usage
- **[Library Management](docs/library-management.md)** - Managing dependencies
- **[WSL Setup](docs/wsl-development.md)** - Windows development guide

## 🔗 Additional Resources

### Libraries Used

- [GxEPD2](https://github.com/ZinggJM/GxEPD2) - E-paper display driver
- [LVGL](https://lvgl.io/) - Graphics library
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) - Core graphics primitives

### Learning Resources

- [LVGL Documentation](https://docs.lvgl.io/)
- [LVGL Examples](https://github.com/lvgl/lvgl/tree/master/examples)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [WeAct E-paper Examples](https://github.com/WeActStudio/WeActStudio.EpaperModule)

### Community

- [LVGL Forum](https://forum.lvgl.io/)
- [ESP32 Forum](https://www.esp32.com/)
- [Arduino Forum](https://forum.arduino.cc/)

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **ZinggJM** for the excellent GxEPD2 library
- **LVGL Team** for the amazing graphics framework
- **WeAct Studio** for quality e-paper modules and documentation
- **Espressif** for ESP32 and development tools

---

**Ready to build your own e-ink display project? Start customizing!** 🚀

For questions or issues, please open an issue on [GitHub](https://github.com/jantielens/ideal-robot/issues).
