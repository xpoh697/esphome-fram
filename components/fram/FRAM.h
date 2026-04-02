#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace fram {

class FRAM : public Component, public i2c::I2CDevice {
 public:
  // Позволяет использовать стандартный write из I2CDevice параллельно с нашим
  using i2c::I2CDevice::write; 

  void setup() override;
  void dump_config() override;

  bool isConnected();

  void write8(uint16_t memaddr, uint8_t value);
  void write16(uint16_t memaddr, uint16_t value);
  void write32(uint16_t memaddr, uint32_t value);
  void writeFloat(uint16_t memaddr, float value);
  void writeDouble(uint16_t memaddr, double value);
  void write(uint16_t memaddr, uint8_t *obj, uint16_t size);

  uint8_t read8(uint16_t memaddr);
  uint16_t read16(uint16_t memaddr);
  uint32_t read32(uint16_t memaddr);
  float readFloat(uint16_t memaddr);
  double readDouble(uint16_t memaddr);
  void read(uint16_t memaddr, uint8_t *obj, uint16_t size);

  int32_t readUntil(uint16_t memaddr, char *buf, uint16_t buflen, char separator);
  int32_t readLine(uint16_t memaddr, char *buf, uint16_t buflen);

  uint16_t getManufacturerID();
  uint16_t getProductID();
  uint16_t getSize();
  uint32_t getSizeBytes();
  void setSizeBytes(uint32_t value);

  uint32_t clear(uint8_t value = 0);

  void sleep();
  bool wakeup(uint32_t trec = 400);

 protected:
  uint16_t _getMetaData(uint8_t field);
  virtual void _writeBlock(uint16_t memaddr, uint8_t *obj, uint8_t size);
  virtual void _readBlock(uint16_t memaddr, uint8_t *obj, uint8_t size);

  uint32_t _sizeBytes{0};
};

class FRAM32 : public FRAM {
 public:
  void write8(uint32_t memaddr, uint8_t value);
  void write16(uint32_t memaddr, uint16_t value);
  void write32(uint32_t memaddr, uint32_t value);
  void writeFloat(uint32_t memaddr, float value);
  void writeDouble(uint32_t memaddr, double value);
  void write(uint32_t memaddr, uint8_t *obj, uint16_t size);

  uint8_t read8(uint32_t memaddr);
  uint16_t read16(uint32_t memaddr);
  uint32_t read32(uint32_t memaddr);
  float readFloat(uint32_t memaddr);
  double readDouble(uint32_t memaddr);
  void read(uint32_t memaddr, uint8_t *obj, uint16_t size);

 protected:
  void _writeBlock(uint32_t memaddr, uint8_t *obj, uint8_t size);
  void _readBlock(uint32_t memaddr, uint8_t *obj, uint8_t size);
};

class FRAM11 : public FRAM {
 protected:
  void _writeBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) override;
  void _readBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) override;
};

class FRAM9 : public FRAM {
 protected:
  void _writeBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) override;
  void _readBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) override;
};

}  // namespace fram
}  // namespace esphome
