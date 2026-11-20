#include <stdint.h>

#include "core.h"
#include "pci.h"

uint32_t pci_read_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC));
    outl(0xCF8, address);
    return inl(0xCFC);
}

void pci_write_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC));
    outl(0xCF8, address);
    outl(0xCFC, value);
}

uint16_t pci_read_config_16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t dword = pci_read_config_32(bus, dev, func, offset & 0xFC);
    
    uint8_t shift = (offset & 2) * 8;
    return (uint16_t)((dword >> shift) & 0xFFFF);
}

void pci_write_config_16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t value) {
    uint8_t aligned_offset = offset & 0xFC;
    uint8_t shift = (offset & 2) * 8;

    uint32_t dword = pci_read_config_32(bus, dev, func, aligned_offset);

    dword &= ~(0xFFFF << shift);
    dword |= ((uint32_t)value << shift);
    
    pci_write_config_32(bus, dev, func, aligned_offset, dword);
}