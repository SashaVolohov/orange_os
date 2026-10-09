#include <stdint.h>

typedef struct {
    uint16_t    vendor_id;
    uint16_t    device_id;
    uint8_t     class_code;
    uint8_t     bus;
    uint8_t     device;
    uint8_t     function;
    uint8_t     subclass_code;
    uint8_t     prog_if;
    uint8_t     reserved[6];
} __attribute__((packed)) pci_device_t;

#define PCI_CONFIG_BAR0        0x10
#define PCI_CONFIG_BAR1        0x14
#define PCI_CONFIG_COMMAND     0x04

#define BAR_TYPE_MMIO          0x00
#define BAR_MEM_64BIT          0x04

extern pci_device_t* pci_devices;
extern uint64_t total_devices;

uint32_t pci_read_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);