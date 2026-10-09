#include "Am79C973.h"
#include "core.h"
#include "pci.h"

int am79c973_base = 0;

uint8_t mac[6];

uintptr_t init_am79c973() {

    for (uint64_t i = 0; i < total_devices; i++) {

        if (pci_devices[i].vendor_id == 0x1022 && pci_devices[i].device_id == 0x2000) {

            KeBugCheck(0x20);

            uint8_t bus = pci_devices[i].bus;
            uint8_t dev = pci_devices[i].device;
            uint8_t func = pci_devices[i].function;

            uint32_t cmd_status = pci_read_config_32(bus, dev, func, 0x04);
            
            cmd_status |= (1 << 2);
            cmd_status |= (1 << 1); 
            pci_write_config_32(bus, dev, func, 0x04, cmd_status);

            uint32_t bar0 = pci_read_config_32(bus, dev, func, 0x10);
            
            if ((bar0 & 1) == 1) {
                am79c973_base = (uint16_t)(bar0 & 0xFFFFFFFC);

                inw(am79c973_base + 0x14);

                outw(am79c973_base + 0x14, 0x0000);

                uint32_t mac_low = inl(am79c973_base + 0x00);
                uint16_t mac_high = inw(am79c973_base + 0x04);

                mac[0] = mac_low & 0xFF;
                mac[1] = (mac_low >> 8) & 0xFF;
                mac[2] = (mac_low >> 16) & 0xFF;
                mac[3] = (mac_low >> 24) & 0xFF;
                mac[4] = mac_high & 0xFF;
                mac[5] = (mac_high >> 8) & 0xFF;

                return (uintptr_t)&mac;
            }
        }
    }

    return 0;
}

void am79c973_write_csr(uint16_t base, uint32_t csr_num, uint32_t value) {
    outw(base + 0x12, csr_num);
    outw(base + 0x10, value);
}

uint32_t am79c973_read_csr(uint16_t base, uint32_t csr_num) {
    outw(base + 0x12, csr_num);
    return inw(base + 0x10);
}