# LogiLinux Command-Line Tools

Command-line utilities for interfacing with Logitech Creator devices from bash and other shell environments.

## Overview

The `tools/` directory contains standalone executables that provide easy access to LogiLinux functionality from the command line. All tools support:

- `--help` for usage information
- JSON output for scripting
- Pipe-friendly operation
- Exit codes for success/failure

## Installation

After building the project, tools are located in `build/tools/`. Install them system-wide:

```bash
sudo make install
```

This installs all tools to `/usr/local/bin/`.

## Available Tools

### Device Discovery

#### `logilinux-devices`

List all connected Logitech devices.

**Usage:**
```bash
logilinux-devices [--json] [--type TYPE]
```

**Examples:**
```bash
# List all devices
logilinux-devices

# JSON output
logilinux-devices --json

# Filter by type
logilinux-devices --type dialpad
logilinux-devices --type keypad

# Pretty JSON with jq
logilinux-devices --json | jq .
```

**Output (JSON):**
```json
{
  "count": 2,
  "devices": [
    {
      "name": "MX Dialpad",
      "type": "dialpad",
      "vendor_id": "0x046d",
      "product_id": "0xbc00",
      "path": "/dev/input/event5",
      "capabilities": ["rotation", "buttons", "high-res-scroll"]
    }
  ]
}
```

---

### Dialpad Tools

#### `dialpad-monitor`

Monitor MX Dialpad rotation and button events.

**Usage:**
```bash
dialpad-monitor [OPTIONS]
```

**Options:**
- `--json` - Output events in JSON format (one per line)
- `--rotation-only` - Only output rotation events
- `--buttons-only` - Only output button events
- `--grab` - Grab device exclusively
- `--device PATH` - Use specific device path

**Examples:**
```bash
# Monitor all events
dialpad-monitor

# JSON output for scripting
dialpad-monitor --json

# Only rotation events
dialpad-monitor --rotation-only

# Grab device exclusively (disable default behavior)
dialpad-monitor --grab

# Process events in bash
dialpad-monitor --json --rotation-only | while read event; do
    delta=$(echo $event | jq -r .delta)
    echo "Rotated by: $delta"
done
```

**Output (JSON):**
```json
{"type":"rotation","delta":1,"delta_high_res":120,"timestamp":1234567}
{"type":"button","action":"press","button":"TOP_LEFT","code":275,"timestamp":1234568}
{"type":"button","action":"release","button":"TOP_LEFT","code":275,"timestamp":1234569}
```

#### `dialpad-grab`

Grab or release dialpad device exclusively.

**Usage:**
```bash
dialpad-grab [--device PATH] <grab|release>
```

**Examples:**
```bash
# Disable default dialpad behavior
sudo dialpad-grab grab

# Re-enable default behavior
sudo dialpad-grab release
```

**Note:** When grabbed, the device's default system behavior is disabled. The tool keeps running to maintain the grab - press Ctrl+C to release.

---

### MX Keypad Tools

#### `keypad-monitor`

Monitor MX Keypad button events.

**Usage:**
```bash
keypad-monitor [OPTIONS]
```

**Options:**
- `--json` - Output events in JSON format
- `--grid-only` - Only grid buttons (0-8)
- `--nav-only` - Only navigation buttons (P1/P2)
- `--device PATH` - Use specific device path

**Examples:**
```bash
# Monitor all button events
keypad-monitor

# JSON output
keypad-monitor --json

# Only grid buttons
keypad-monitor --grid-only

# Process button presses in bash
keypad-monitor --json | while read event; do
    button=$(echo $event | jq -r .button)
    action=$(echo $event | jq -r .action)
    
    if [ "$action" = "press" ]; then
        echo "Button $button was pressed!"
    fi
done
```

**Output (JSON):**
```json
{"type":"button","action":"press","button":"GRID_0","code":0,"timestamp":1234567}
{"type":"button","action":"release","button":"GRID_0","code":0,"timestamp":1234568}
{"type":"button","action":"press","button":"P1_LEFT","code":161,"timestamp":1234569}
```

#### `keypad-set-image`

Set JPEG image on LCD button(s).

**Usage:**
```bash
keypad-set-image [OPTIONS] <button> <image.jpg>
echo <jpeg_data> | keypad-set-image [OPTIONS] <button> -
```

**Options:**
- `--all` - Set image on all buttons (0-8)
- `--device PATH` - Use specific device path

**Examples:**
```bash
# Set button 0
sudo keypad-set-image 0 logo.jpg

# Set button 5 by name
sudo keypad-set-image GRID_5 icon.jpg

# Set all buttons
sudo keypad-set-image --all background.jpg

# Read from stdin
cat image.jpg | sudo keypad-set-image 3 -

# Generate and set image on-the-fly
convert input.png -resize 118x118 -quality 85 - | sudo keypad-set-image 0 -
```

**Note:** Images should be 118x118 pixels for best results. Requires sudo or hidraw permissions.

#### `keypad-set-color`

Set solid color on LCD button(s).

**Usage:**
```bash
keypad-set-color [OPTIONS] <button> <color>
```

**Options:**
- `--all` - Set color on all buttons
- `--device PATH` - Use specific device path

**Color Formats:**
- Named: `red`, `green`, `blue`, `yellow`, `cyan`, `magenta`, `white`, `black`, `orange`, `purple`, `pink`, `lime`
- Hex: `#RRGGBB` or `RRGGBB`
- RGB: `r,g,b` (0-255 each)

**Examples:**
```bash
# Named color
sudo keypad-set-color 0 red

# Hex color
sudo keypad-set-color 5 #FF8000

# RGB color
sudo keypad-set-color 3 255,128,0

# Set all buttons
sudo keypad-set-color --all blue

# Hex without #
sudo keypad-set-color 4 00FF00
```

**Note:** Requires ImageMagick's `convert` command and hidraw permissions.

#### `keypad-set-gif`

Set animated GIF on LCD button(s).

**Usage:**
```bash
keypad-set-gif [OPTIONS] <button> <animation.gif>
```

**Options:**
- `--all` - Set GIF on all buttons
- `--no-loop` - Play animation once (don't loop)
- `--device PATH` - Use specific device path

**Examples:**
```bash
# Animate button 0
sudo keypad-set-gif 0 spinner.gif

# Animate button 5 by name
sudo keypad-set-gif GRID_5 loading.gif

# Animate all buttons
sudo keypad-set-gif --all background.gif

# Play once without looping
sudo keypad-set-gif --no-loop 3 intro.gif
```

**Note:** GIF is scaled to 118x118 pixels. Animation runs until Ctrl+C. Requires giflib and libjpeg support.

---

## Bash Integration Examples

### Volume Control with Dialpad

```bash
#!/bin/bash
# Control system volume with MX Dialpad

dialpad-monitor --json --rotation-only | while read event; do
    delta=$(echo $event | jq -r .delta)
    
    if [ "$delta" -gt 0 ]; then
        pactl set-sink-volume @DEFAULT_SINK@ +5%
    else
        pactl set-sink-volume @DEFAULT_SINK@ -5%
    fi
done
```

### Button-Triggered Actions

```bash
#!/bin/bash
# Run commands based on MX Keypad button presses

keypad-monitor --json --grid-only | while read event; do
    action=$(echo $event | jq -r .action)
    button=$(echo $event | jq -r .code)
    
    if [ "$action" = "press" ]; then
        case $button in
            0) firefox & ;;
            1) code & ;;
            2) gnome-terminal & ;;
            # ... more buttons
        esac
    fi
done
```

### Dynamic Button Colors

```bash
#!/bin/bash
# Change button colors based on system state

while true; do
    load=$(uptime | awk '{print $(NF-2)}' | sed 's/,//')
    
    if (( $(echo "$load < 1.0" | bc -l) )); then
        sudo keypad-set-color 0 green
    elif (( $(echo "$load < 2.0" | bc -l) )); then
        sudo keypad-set-color 0 yellow
    else
        sudo keypad-set-color 0 red
    fi
    
    sleep 5
done
```

### Device Auto-Detection

```bash
#!/bin/bash
# Check if devices are connected

if logilinux-devices --type dialpad --json | jq -e '.count > 0' > /dev/null; then
    echo "Dialpad connected!"
    # Do something with dialpad
fi

if logilinux-devices --type keypad --json | jq -e '.count > 0' > /dev/null; then
    echo "Keypad connected!"
    # Do something with keypad
fi
```

### Button Image Gallery

```bash
#!/bin/bash
# Set different images on each button

for i in {0..8}; do
    sudo keypad-set-image $i "images/icon_$i.jpg"
done
```

---

## Permissions

Most tools require access to `/dev/input/event*` or `/dev/hidraw*` devices.

### Option 1: Add user to input group (for monitoring)

```bash
sudo usermod -a -G input $USER
# Log out and back in
```

### Option 2: Use sudo (for all operations)

```bash
sudo dialpad-monitor
sudo keypad-set-image 0 logo.jpg
```

### Option 3: Create udev rules

Create `/etc/udev/rules.d/99-logitech.rules`:

```
# MX Dialpad
SUBSYSTEM=="input", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="bc00", MODE="0666"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="bc00", MODE="0666"

# MX Keypad
SUBSYSTEM=="input", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c354", MODE="0666"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c354", MODE="0666"
```

Then reload:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## Dependencies

### Core Tools
- `logilinux-devices` - None
- `dialpad-monitor` - None
- `dialpad-grab` - None
- `keypad-monitor` - None

### LCD Tools
- `keypad-set-image` - None (reads JPEG directly)
- `keypad-set-color` - **ImageMagick** (`convert` command)
- `keypad-set-gif` - **giflib** and **libjpeg** (compile-time)

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install imagemagick
```

**Fedora:**
```bash
sudo dnf install ImageMagick
```

**Arch:**
```bash
sudo pacman -S imagemagick
```

---

## Exit Codes

All tools follow standard Unix exit code conventions:

- `0` - Success
- `1` - Error (device not found, permission denied, invalid arguments, etc.)

This allows for easy error checking in scripts:

```bash
if logilinux-devices --type dialpad --json > /dev/null; then
    echo "Dialpad found!"
else
    echo "No dialpad connected"
fi
```

---

## Troubleshooting

### "No device found"
- Check device is connected: `lsusb | grep -i logitech`
- Check permissions: Try with `sudo`
- Verify device is detected: `logilinux-devices`

### "Permission denied"
- Add user to input group or use sudo
- Check udev rules are configured
- Verify device files exist: `ls -l /dev/input/event*`

### "Failed to set image"
- Verify file is valid JPEG: `file image.jpg`
- Check file size isn't too large (< 50KB recommended)
- For colors: Install ImageMagick

### "GIF support not available"
- Rebuild library with giflib and libjpeg installed
- Check: `ldconfig -p | grep gif`

---

## Contributing

When adding new tools:

1. Create tool source in `tools/`
2. Add to `tools/CMakeLists.txt`
3. Include `--help` option with examples
4. Support JSON output where appropriate
5. Document in this README
6. Follow naming convention: `<device>-<action>`

---

## See Also

- [Main README](../README.md) - Library overview
- [Library API](../lib/README.md) - C++ API documentation
- [Examples](../examples/) - Example programs
