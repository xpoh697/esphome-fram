#include "FRAM.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace fram {

static const char *const TAG = "fram";
const uint8_t FRAM_SLAVE_ID_ = 0x7C;
const uint8_t FRAM_SLEEP_CMD = 0x86;

void FRAM::setup() {
  if (!this->isConnected()) {
    ESP_LOGE(TAG, "Device on address 0x%x not found!", this->address_);
    this->mark_failed();
  } else if (!this->getSize()) {
    ESP_LOGW(TAG, "Device on address 0x%x returned 0 size, set size in config!", this->address_);
  }
}

void FRAM::dump_config() {
  ESP_LOGCONFIG(TAG, "FRAM:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%x", this->address_);
  if (!this->isConnected()) ESP_LOGE(TAG, "  Device not found!");
  if (this->_sizeBytes) ESP_LOGCONFIG(TAG, "  Size: %uKiB", this->_sizeBytes / 1024UL);
}

bool FRAM::isConnected() {
  // Явно вызываем версию из I2CDevice
  return this->i2c::I2CDevice::write(nullptr, 0) == i2c::ERROR_OK;
}

void FRAM::write8(uint16_t memaddr, uint8_t value) { this->_writeBlock(memaddr, &value, 1); }
void FRAM::write16(uint16_t memaddr, uint16_t value) { this->_writeBlock(memaddr, (uint8_t *)&value, 2); }
void FRAM::write32(uint16_t memaddr, uint32_t value) { this->_writeBlock(memaddr, (uint8_t *)&value, 4); }
void FRAM::writeFloat(uint16_t memaddr, float value) { this->_writeBlock(memaddr, (uint8_t *)&value, 4); }
void FRAM::writeDouble(uint16_t memaddr, double value) { this->_writeBlock(memaddr, (uint8_t *)&value, 8); }

void FRAM::write(uint16_t memaddr, uint8_t *obj, uint16_t size) {
  const int blocksize = 24;
  while (size >= blocksize) {
    this->_writeBlock(memaddr, obj, blocksize);
    memaddr += blocksize; obj += blocksize; size -= blocksize;
  }
  if (size > 0) this->_writeBlock(memaddr, obj, size);
}

uint8_t FRAM::read8(uint16_t memaddr) { uint8_t v; this->_readBlock(memaddr, &v, 1); return v; }
uint16_t FRAM::read16(uint16_t memaddr) { uint16_t v; this->_readBlock(memaddr, (uint8_t *)&v, 2); return v; }
uint32_t FRAM::read32(uint16_t memaddr) { uint32_t v; this->_readBlock(memaddr, (uint8_t *)&v, 4); return v; }
float FRAM::readFloat(uint16_t memaddr) { float v; this->_readBlock(memaddr, (uint8_t *)&v, 4); return v; }
double FRAM::readDouble(uint16_t memaddr) { double v; this->_readBlock(memaddr, (uint8_t *)&v, 8); return v; }

void FRAM::read(uint16_t memaddr, uint8_t *obj, uint16_t size) {
  const uint8_t blocksize = 24;
  while (size >= blocksize) {
    this->_readBlock(memaddr, obj, blocksize);
    memaddr += blocksize; obj += blocksize; size -= blocksize;
  }
  if (size > 0) this->_readBlock(memaddr, obj, size);
}

int32_t FRAM::readUntil(uint16_t memaddr, char *buf, uint16_t buflen, char separator) {
  this->read(memaddr, (uint8_t *)buf, buflen);
  for (uint16_t i = 0; i < buflen; i++) {
    if (buf[i] == separator) { buf[i] = 0; return i; }
  }
  return -1;
}

int32_t FRAM::readLine(uint16_t memaddr, char *buf, uint16_t buflen) {
  this->read(memaddr, (uint8_t *)buf, buflen);
  for (uint16_t i = 0; i < buflen - 1; i++) {
    if (buf[i] == '\n') { buf[i + 1] = 0; return i + 1; }
  }
  return -1;
}

uint16_t FRAM::getManufacturerID() { return this->_getMetaData(0); }
uint16_t FRAM::getProductID() { return this->_getMetaData(1); }
uint16_t FRAM::getSize() {
  uint16_t density = this->_getMetaData(2);
  if (density > 0) {
    uint16_t size = (1UL << density);
    this->_sizeBytes = size * 1024UL;
    return size;
  }
  return 0;
}
uint32_t FRAM::getSizeBytes() { return this->_sizeBytes; }
void FRAM::setSizeBytes(uint32_t value) { this->_sizeBytes = value; }

uint32_t FRAM::clear(uint8_t value) {
  uint8_t buffer[16];
  for (uint8_t i = 0; i < 16; i++) buffer[i] = value;
  for (uint32_t addr = 0; addr < this->_sizeBytes; addr += 16) this->_writeBlock(addr, buffer, 16);
  return this->_sizeBytes;
}

void FRAM::sleep() {
  uint8_t addr = this->address_ << 1;
  this->bus_->write(FRAM_SLAVE_ID_, &addr, 1, false);
  this->bus_->write(FRAM_SLEEP_CMD >> 1, nullptr, 0, true);
}

bool FRAM::wakeup(uint32_t trec) {
  bool b = this->isConnected();
  if (trec > 0) delayMicroseconds(trec);
  return this->isConnected();
}

uint16_t FRAM::_getMetaData(uint8_t field) {
  if (field > 2) return 0;
  uint8_t addr = this->address_ << 1;
  this->bus_->write(FRAM_SLAVE_ID_, &addr, 1, false);
  uint8_t data[3] = {0,0,0};
  if (this->bus_->read(FRAM_SLAVE_ID_, data, 3) != i2c::ERROR_OK) return 0;
  if (field == 0) return (data[0] << 4) + (data[1] >> 4);
  if (field == 1) return ((data[1] & 0x0F) << 8) + data[2];
  return data[1] & 0x0F;
}

// Прямые вызовы функций устройства
void FRAM::_writeBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) {
  this->write_register16(memaddr, obj, size);
}

void FRAM::_readBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) {
  this->read_register16(memaddr, obj, size);
}

// Для версий с разным адресным пространством используем ручную сборку буфера
void FRAM32::_writeBlock(uint32_t memaddr, uint8_t *obj, uint8_t size) {
  uint8_t addr = this->address_ + (memaddr >> 16);
  uint8_t buffer[blocksize + 2]; // blocksize обычно 24
  buffer[0] = (uint8_t)(memaddr >> 8);
  buffer[1] = (uint8_t)(memaddr & 0xFF);
  std::memcpy(&buffer[2], obj, size);
  this->bus_->write(addr, buffer, size + 2);
}

void FRAM32::_readBlock(uint32_t memaddr, uint8_t *obj, uint8_t size) {
  uint8_t addr = this->address_ + (memaddr >> 16);
  uint8_t maddr[] = { (uint8_t)(memaddr >> 8), (uint8_t)(memaddr & 0xFF) };
  this->bus_->write(addr, maddr, 2, false);
  this->bus_->read(addr, obj, size);
}

void FRAM11::_writeBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) {
  uint8_t addr = this->address_ | ((memaddr & 0x0700) >> 8);
  uint8_t buffer[32];
  buffer[0] = memaddr & 0xFF;
  std::memcpy(&buffer[1], obj, size);
  this->bus_->write(addr, buffer, size + 1);
}

void FRAM11::_readBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) {
  uint8_t addr = this->address_ | ((memaddr & 0x0700) >> 8);
  uint8_t maddr = memaddr & 0xFF;
  this->bus_->write(addr, &maddr, 1, false);
  this->bus_->read(addr, obj, size);
}

void FRAM9::_writeBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) {
  uint8_t addr = this->address_ | ((memaddr & 0x0100) >> 8);
  uint8_t buffer[32];
  buffer[0] = memaddr & 0xFF;
  std::memcpy(&buffer[1], obj, size);
  this->bus_->write(addr, buffer, size + 1);
}

void FRAM9::_readBlock(uint16_t memaddr, uint8_t *obj, uint8_t size) {
  uint8_t addr = this->address_ | ((memaddr & 0x0100) >> 8);
  uint8_t maddr = memaddr & 0xFF;
  this->bus_->write(addr, &maddr, 1, false);
  this->bus_->read(addr, obj, size);
}

}  // namespace fram
}  // namespace esphome
