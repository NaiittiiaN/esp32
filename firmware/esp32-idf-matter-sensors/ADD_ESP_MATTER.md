# Adding ESP-Matter Component

## Version Information

**Current ESP-Matter Release**: v1.5 (branch: `release/v1.5`)

**Available in ESP Component Registry**: v1.4.2~1 (latest stable)

**Note**: The standalone component in the registry (v1.4.2~1) is the most recent 
version available. For v1.5 features, use the full esp-matter repository approach below.

## Option 1: Using ESP-IDF Component Registry (Recommended for v1.4.2)

Run this command in the project directory:

```bash
cd firmware/esp32-idf-matter-sensors
idf.py add-dependency "espressif/esp_matter^1.4.2~1"
```

This will download and install esp_matter v1.4.2~1 to `components/`.

## Option 2: Full ESP-Matter Repository (For v1.5)

For Matter v1.5, clone the full esp-matter repository:

```bash
# From project root
cd firmware/esp32-idf-matter-sensors

# Clone esp-matter release/v1.5 branch
git clone --branch release/v1.5 --depth 1 \
    https://github.com/espressif/esp-matter.git \
    components/esp_matter_temp

# Initialize connectedhomeip submodule (required for full Matter stack)
cd components/esp_matter_temp
git submodule update --init --depth 1
cd ./connectedhomeip/connectedhomeip
./scripts/checkout_submodules.py --platform esp32 linux --shallow
cd ../../..

# The component is at components/esp_matter_temp/components/esp_matter
# Move it to the correct location
mv components/esp_matter_temp/components/esp_matter components/
rm -rf components/esp_matter_temp
```

## After Adding Component

1. **Clean and rebuild**:
   ```bash
   idf.py fullclean
   idf.py set-target esp32
   idf.py build
   ```

2. **Flash**:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Dependencies

The ESP-Matter component requires:
- ESP-IDF v5.4.1 or later
- connectedhomeip submodule

## Configuration

After adding the component, you can configure Matter via `idf.py menuconfig`:

```
→ Component config → ESP Matter
```

Key settings:
- Enable/disable test credentials
- Device VID/PID
- Setup passcode
- Discriminator

## Expected Output

After adding esp_matter, the build output should show:
- `matter_bridge.cpp` will detect `#include "esp_matter.h"`
- Status changes from "stub" to "esp-matter"
- Real Matter endpoints become available