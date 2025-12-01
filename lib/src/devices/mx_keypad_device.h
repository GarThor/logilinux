#ifndef LOGILINUX_MX_KEYPAD_DEVICE_H
#define LOGILINUX_MX_KEYPAD_DEVICE_H

#include "logilinux/device.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace LogiLinux {

class MXKeypadDevice : public Device {
public:
  explicit MXKeypadDevice(const DeviceInfo &info);
  ~MXKeypadDevice() override;

  const DeviceInfo &getInfo() const override { return info_; }
  DeviceType getType() const override { return info_.type; }
  bool hasCapability(DeviceCapability cap) const override;

  void setEventCallback(EventCallback callback) override;
  void startMonitoring() override;
  void stopMonitoring() override;
  bool isMonitoring() const override;

  bool grabExclusive(bool grab) override;

  // MX Keypad specific API
  bool setKeyImage(int keyIndex, const std::vector<uint8_t> &jpegData);
  bool setKeyColor(int keyIndex, uint8_t r, uint8_t g, uint8_t b);
  bool initialize();
  bool hasLCD() const;

  // Full screen image (434x434 covering all 9 keys with gaps)
  bool setScreenImage(const std::vector<uint8_t> &jpegData);
  
  // Raw image placement at arbitrary coordinates
  bool setRawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   const std::vector<uint8_t> &jpegData);

  // Screen dimensions
  static constexpr uint16_t SCREEN_WIDTH = 434;   // 118*3 + 40*2
  static constexpr uint16_t SCREEN_HEIGHT = 434;
  static constexpr uint16_t KEY_SIZE = 118;
  static constexpr uint16_t GAP_SIZE = 40;

  // GIF support for individual keys
  bool setKeyGif(int keyIndex, const std::vector<uint8_t> &gifData,
                 bool loop = true);
  bool setKeyGifFromFile(int keyIndex, const std::string &gifPath,
                         bool loop = true);
  void stopKeyAnimation(int keyIndex);
  void stopAllAnimations();

  // Full-screen GIF (434x434, much faster than 9 individual key GIFs)
  bool setScreenGif(const std::vector<uint8_t> &gifData, bool loop = true);
  bool setScreenGifFromFile(const std::string &gifPath, bool loop = true);
  void stopScreenAnimation();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  DeviceInfo info_;
  std::vector<DeviceCapability> capabilities_;
  EventCallback event_callback_;
};

} // namespace LogiLinux

#endif // LOGILINUX_MX_KEYPAD_DEVICE_H
