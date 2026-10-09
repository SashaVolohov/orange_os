#include "ata.h"
#include "ahci.h"
#include "disk.h"

Drive drives[64];
int total_drives = 0;

void registerDisk(uint64_t type, uint8_t port, uint8_t port2, uint64_t sector_count) {
    drives[total_drives].type = type;
    drives[total_drives].port = port;
    drives[total_drives].port2 = port2;
    drives[total_drives].sector_count = sector_count;

    total_drives++;
}

Drive* get_disk(int drive) {
    if(drive >= 64) return 0;
    return &drives[drive];
}

int get_drives_count() {
    return total_drives;
}

void registerATADevices() {

    const int drive_ports[] = {0xA0, 0xB0};
    const int num_drives = sizeof(drive_ports) / sizeof(drive_ports[0]);

    for (int i = 0; i < num_drives; i++) {
        DriveInfo info = ata_detect_drive(drive_ports[i]);
        if (info.type == 1) {
            registerDisk(0, drive_ports[i], 0, info.sector_count);
        }
    }

}

void registerSATADevices() {

    for(int i = 0; i < 32; i++) {
        int sata_port = get_ahch_port(i);
        if(sata_port == 1) {
            registerDisk(1, i, 0, ahci_get_sectors(i));
        }
    }

}

int disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr) {

    if(disk_id >= 32) return -1;

    if(drives[disk_id].type == 0) { // IDE
        return ata_dma_read_sector(lba, count, target_addr, get_ata_disk_id_on_bus(drives[disk_id].port));
    } else if(drives[disk_id].type == 1) { // SATA
        return ahci_read(drives[disk_id].port, lba, count, target_addr);
    }

}

int disk_write(int disk_id, uint64_t lba, uint32_t count, uint64_t source_addr) {

    if(disk_id >= 32) return -1;

    if(drives[disk_id].type == 0) { // IDE
        return ata_dma_write_sector(lba, count, source_addr, get_ata_disk_id_on_bus(drives[disk_id].port));
    } else if(drives[disk_id].type == 1) { // SATA
        return ahci_write(drives[disk_id].port, lba, count, source_addr);
    }

}