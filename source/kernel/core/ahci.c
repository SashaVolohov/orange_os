#include "ahci.h"
#include "core.h"
#include "os_memory.h"
#include "pci.h"

uintptr_t ahci_base_phys = 0;
uint8_t sata_ports[32];
uint8_t sata_devices_count = 0;

__attribute__((aligned(1024))) uint8_t ahci_static_clb[1024];
__attribute__((aligned(256)))  uint8_t ahci_static_fb[256];
__attribute__((aligned(128)))  uint8_t ahci_static_ctba[512];

int get_ahch_port(int port) {
    if(port >= 32) return 0;
    return sata_ports[port];
}

void ahci_init_port(ahci_port_t* port) {
    port->cmd &= ~0x0001;
    port->cmd &= ~0x0010;

    port->is = 0xFFFF;
    port->serr = 0xFFFFFFFF;
    
    while (port->cmd & (0x8000 | 0x4000)); 

    uint64_t phys_page = alloc_page();
    
    port->clb = phys_page; 
    port->clbu = 0;
    
    port->fb = phys_page + 1024;
    port->fbu = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)phys_page;
    memset(cmd_list, 0, 1024);

    uint64_t tables_phys_page1 = alloc_page();
    uint64_t tables_phys_page2 = alloc_page();

    for (int i = 0; i < 32; i++) {
        uint32_t current_table_phys = 0;
        if (i < 16) {
            current_table_phys = tables_phys_page1 + (i * 256);
        } else {
            current_table_phys = tables_phys_page2 + ((i - 16) * 256);
        }

        cmd_list[i].ctba = current_table_phys;
        cmd_list[i].ctbau = 0;

        ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)current_table_phys;
        memset(cmd_table, 0, sizeof(ahci_cmd_table_t));
    }

    port->cmd |= 0x0010;
    port->cmd |= 0x0001;
}

void ahci_init_port_atapi(ahci_port_t* port) {
    port->cmd &= ~0x0001;
    port->cmd &= ~0x0010;
    
    while(port->cmd & 0x4000);
    while(port->cmd & 0x0020);

    port->is = 0xFFFF;
    port->serr = 0xFFFFFFFF;

    uint64_t phys_page = alloc_page();

    port->clb = (uintptr_t)ahci_static_clb;
    port->fb  = (uintptr_t)ahci_static_fb;
    
    port->clb = phys_page; 
    port->clbu = 0;
    
    port->fb = phys_page + 1024;
    port->fbu = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)phys_page;
    memset(cmd_list, 0, 1024);

    uint64_t tables_phys_page1 = alloc_page();
    uint64_t tables_phys_page2 = alloc_page();

    for (int i = 0; i < 32; i++) {
        uint32_t current_table_phys = 0;
        if (i < 16) {
            current_table_phys = tables_phys_page1 + (i * 256);
        } else {
            current_table_phys = tables_phys_page2 + ((i - 16) * 256);
        }

        cmd_list[i].ctba = current_table_phys;
        cmd_list[i].ctbau = 0;

        ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)current_table_phys;
        memset(cmd_table, 0, sizeof(ahci_cmd_table_t));
    }

    port->clb = (uintptr_t)ahci_static_clb;
    port->fb  = (uintptr_t)ahci_static_fb;

    cmd_list[0].ctba = (uintptr_t)ahci_static_ctba;

    port->cmd |= 0x0010;
    sleep_ms(10);

    port->is = port->is;
    port->serr = port->serr;

    port->cmd |= 0x0001;

}

void enable_ahci(uint64_t abar_phys_addr) {
    ahci_abar_t* abar = (ahci_abar_t*)abar_phys_addr;

    abar->ghc |= (1 << 31);
    abar->ghc |= 1;
    while (abar->ghc & 1);
    abar->ghc |= (1 << 31);
    abar->ghc |= (1 << 1);

    for (int i = 0; i < 32; i++) {

        sata_ports[i] = 0;

        if (abar->pi & (1 << i)) {
            uint32_t status = abar->ports[i].ssts;
            uint8_t ipm = (status >> 8) & 0x0F;
            uint8_t det = status & 0x0F;

            if (det == 3 && ipm == 1) {
                if (abar->ports[i].sig == 0x00000101) {
                    ahci_init_port(&abar->ports[i]);

                    sata_ports[i] = 1;
                    sata_devices_count++;
                } else if(abar->ports[i].sig == 0xEB140101) {
                    ahci_init_port_atapi(&abar->ports[i]);

                    satapi_inquiry_data_t* satapi_info = ahch_atapi_get_info(i);

                    if(satapi_info->device_type == 0x05) { // CD-ROM/DVD-ROM
                        sata_ports[i] = 2;
                        sata_devices_count++;

                        abar->ports[i].sctl = (abar->ports[i].sctl & ~0xF) | 1;
                        sleep_ms(20);

                        abar->ports[i].sctl = (abar->ports[i].sctl & ~0xF);
                        sleep_ms(50);

                        uint32_t timeout = 1000;
                        while (((abar->ports[i].ssts & 0xF) != 3) && --timeout) {
                            sleep_ms(1);
                        }

                        volatile uint32_t dummy = abar->ports[i].is;
                        abar->ports[i].is = dummy;

                        volatile uint32_t dummy_serr = abar->ports[i].serr; 
                        abar->ports[i].serr = dummy_serr;

                        int status = ahci_wait_port(i);
                        if(status == 0) {
                            continue;
                        }

                        /*status = ahci_atapi_test_ready(i);
                        if(status == 0) {
                            tprintf(0, 24, 0x70, "Port not ready.");
                            while(1) {

                            }
                        }*/

                        /*int count_of_sectors = ahci_atapi_get_sectors(i);

                        int drive_size_in_mb = (count_of_sectors * 2048) / 1024 / 1024;*/
                    }

                }
            }
        }
    }
}

int init_ahci() {

    for (uint64_t i = 0; i < total_devices; i++) {

        if (pci_devices[i].class_code == 0x01 && pci_devices[i].subclass_code == 0x06 && pci_devices[i].prog_if == 0x01) {

            uint8_t bus = pci_devices[i].bus;
            uint8_t dev = pci_devices[i].device;
            uint8_t func = pci_devices[i].function;

            uint32_t cmd_status = pci_read_config_32(bus, dev, func, 0x04);
            
            cmd_status |= (1 << 2);
            cmd_status |= (1 << 1); 
            pci_write_config_32(bus, dev, func, 0x04, cmd_status);

            uint32_t bar5 = pci_read_config_32(bus, dev, func, 0x24);
            
            if ((bar5 & 1) == 0) {
                ahci_base_phys = bar5 & 0xFFFFFFF0;

                if ((bar5 & 0x4) != 0) {
                    uint64_t bar4 = pci_read_config_32(bus, dev, func, 0x20);
                    ahci_base_phys |= (bar4 << 32);
                }

                uint64_t phys_page = (uint64_t)ahci_base_phys & ~0xFFF;
                map_page(phys_page, phys_page, 0x1B);

                enable_ahci(ahci_base_phys);

                return 1;
            }
        }
    }

    for(int i = 0; i < 32; i++) {
        sata_ports[i] = 0;
    }

    return 0;
}

int ahci_read_lba48(ahci_port_t* port, uint64_t lba, uint16_t count, uint64_t target_phys_buffer) {
    port->is = 0xFFFF;

    int slot = 0; 
    
    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = sizeof(fis_reg_h2d_t) / 4; 
    cmd_header->w = 0;
    cmd_header->prdtl = 1;

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = virtual_to_physical(pt_begin, target_phys_buffer);
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = (count * 512) - 1;  
    cmd_table->prdt[0].i = 1;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_CMD_READ_DMA_EXT;
    fis->device = 1 << 6;

    fis->lba0 = (uint8_t)(lba >> 0);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);

    fis->countl = (uint8_t)(count >> 0);
    fis->counth = (uint8_t)(count >> 8);

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) {
        return -1;
    }

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) {
            break;
        }
        
        if (port->tfd & 0x01) {
            return -2;
        }
    }

    if (port->tfd & 0x01) {
        return -2;
    }

    return 0;
}

int ahci_read(int port, uint64_t lba, uint16_t count, uint64_t target_phys_buffer) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahci_read_lba48(&abar->ports[port], lba, count, target_phys_buffer);
}

int ahci_write_lba48(ahci_port_t* port, uint64_t lba, uint16_t count, uint64_t source_phys_buffer) {
    port->is = 0xFFFF;

    int slot = 0; 
    
    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = sizeof(fis_reg_h2d_t) / 4; 
    cmd_header->w = 1;
    cmd_header->prdtl = 1;
    

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = virtual_to_physical(pt_begin, source_phys_buffer);
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = (count * 512) - 1;
    cmd_table->prdt[0].i = 1;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_CMD_WRITE_DMA_EXT;
    fis->device = 1 << 6;

    fis->lba0 = (uint8_t)(lba >> 0);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);

    fis->countl = (uint8_t)(count >> 0);
    fis->counth = (uint8_t)(count >> 8);

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) {
        return -1;
    }

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) {
            break;
        }
        if (port->tfd & 0x01) {
            return -2;
        }
    }

    if (port->tfd & 0x01) {
        return -2;
    }

    return 0;
}

int ahci_write(int port, uint64_t lba, uint16_t count, uint64_t source_phys_buffer) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahci_write_lba48(&abar->ports[port], lba, count, source_phys_buffer);
}

uint64_t ahci_get_sector_count(ahci_port_t* port) {
    uint64_t phys_buffer = alloc_page();
    
    port->is = 0xFFFF;
    int slot = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = sizeof(fis_reg_h2d_t) / 4;
    cmd_header->w = 0;
    cmd_header->prdtl = 1;

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = phys_buffer;
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = 512 - 1;
    cmd_table->prdt[0].i = 1;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_CMD_IDENTIFY;
    fis->device = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) return 0;

    port->ci = (1 << slot);

    while (1) {

        if ((port->ci & (1 << slot)) == 0) break;
        if (port->tfd & 0x01) {
            return 0; 
        }
    }

    ahci_identify_t* identify = (ahci_identify_t*)phys_buffer;
    
    uint64_t total_sectors = 0;

    if (identify->command_sets & (1 << 10)) {
        total_sectors = identify->lba48_sectors;
    } else {
        uint32_t* lba28_sectors = (uint32_t*)&((uint16_t*)identify)[60];
        total_sectors = *lba28_sectors;
    }

    free_page(phys_buffer);

    return total_sectors;
}

uint64_t ahci_get_sectors(int port) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahci_get_sector_count(&abar->ports[port]);
}

satapi_inquiry_data_t* ahch_atapi_get_info_port(ahci_port_t* port) {
    uint64_t phys_buffer = alloc_page();
    
    port->is = 0xFFFF;
    int slot = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = 5;
    cmd_header->w = 0;
    cmd_header->prdtl = 1;
    cmd_header->a = 1; // ATAPI

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = phys_buffer;
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = 512 - 1;
    cmd_table->prdt[0].i = 1;

    cmd_table->acmd[0] = 0x12; // SCSI INQUIRY
    cmd_table->acmd[1] = 0x00;
    cmd_table->acmd[2] = 0x00;
    cmd_table->acmd[3] = 0x00;
    cmd_table->acmd[4] = 0x60;
    cmd_table->acmd[5] = 0x00;
    
    cmd_table->acmd[6] = 0x00;
    cmd_table->acmd[7] = 0x00;
    cmd_table->acmd[8] = 0x00;
    cmd_table->acmd[9] = 0x00;
    cmd_table->acmd[10] = 0x00;
    cmd_table->acmd[11] = 0x00;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_PACKET;
    fis->featurel = 0x01;
    fis->featureh = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) return 0;

    port->ci = (1 << slot);

    while (1) {

        if (port->is) {
            port->is = port->is;
        }

        if ((port->ci & (1 << slot)) == 0) break;
        if (port->tfd & 0x01) {
            return 0; 
        }
    }

    port->cmd &= ~0x0001;
    port->cmd &= ~0x0010;
    while(port->cmd & 0x4000);
    while(port->cmd & 0x0020);

    port->cmd &= ~0x0004;
    sleep_ms(20);

    port->cmd |= 0x0004;
    port->cmd |= 0x0002;
    sleep_ms(50);

    port->is = 0xFFFF;
    port->serr = 0xFFFFFFFF;
    volatile uint32_t* hba_is = (volatile uint32_t*)((uintptr_t)ahci_base_phys + 0x08);
    *hba_is = 0xFFFFFFFF;

    port->cmd |= 0x0010;
    sleep_ms(10);
    port->cmd |= 0x0001;
    sleep_ms(10);

    uint32_t wait_sata = 1000;
    while (((port->ssts & 0xF) != 3) && --wait_sata) {
        sleep_ms(1);
    }

    satapi_inquiry_data_t* identify = (satapi_inquiry_data_t*)phys_buffer;

    return identify;
}

satapi_inquiry_data_t* ahch_atapi_get_info(int port) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahch_atapi_get_info_port(&abar->ports[port]);
}

int ahci_atapi_clear_unit_attention_dma(ahci_port_t* port) {
    uint64_t phys_buffer = alloc_page();
    if (!phys_buffer) return 0;
    
    port->is = 0xFFFF;
    port->serr = 0xFFFFFFFF;
    int slot = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = 5;
    cmd_header->w = 0;
    cmd_header->prdtl = 1;
    cmd_header->a = 1;

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = phys_buffer;
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = 512 - 1;
    cmd_table->prdt[0].i = 1;

    cmd_table->acmd[0] = 0x03; // REQUEST SENSE
    cmd_table->acmd[1] = 0x00;
    cmd_table->acmd[2] = 0x00;
    cmd_table->acmd[3] = 0x00;
    cmd_table->acmd[4] = 0x60;
    cmd_table->acmd[5] = 0x00;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_PACKET;
    fis->featurel = 0x01; // DMA
    fis->featureh = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) { free_page(phys_buffer); return 0; }

    port->ci = (1 << slot);

    while (1) {
        if (port->is) port->is = port->is;
        if (port->serr) port->serr = port->serr;
        
        volatile uint32_t* hba_is = (volatile uint32_t*)((uintptr_t)ahci_base_phys + 0x08);
        if (*hba_is) *hba_is = *hba_is;

        if ((port->ci & (1 << slot)) == 0) break;
        
        if (port->tfd & 0x01) {
            while (port->ci & (1 << slot));
            break; 
        }
    }

    port->cmd &= ~0x0001;
    
    uint32_t wait_cr = 100000;
    while ((port->cmd & 0x4000) && --wait_cr);

    port->is = 0xFFFF;
    port->serr = 0xFFFFFFFF;
    
    volatile uint32_t* hba_is = (volatile uint32_t*)((uintptr_t)ahci_base_phys + 0x08);
    *hba_is = 0xFFFFFFFF;

    port->cmd |= 0x0001;
    
    sleep_ms(20);

    free_page(phys_buffer);
    return 1;
}

int ahci_atapi_get_sectors_port(ahci_port_t* port) {

    uint64_t phys_buffer = alloc_page();
    
    port->is = 0xFFFF;
    int slot = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = 5;
    cmd_header->w = 0;
    cmd_header->prdtl = 1;
    cmd_header->a = 1; // ATAPI

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = phys_buffer;
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = 512 - 1;
    cmd_table->prdt[0].i = 1;

    cmd_table->acmd[0] = 0x25; // SCSI INQUIRY
    cmd_table->acmd[1] = 0x00;
    cmd_table->acmd[2] = 0x00;
    cmd_table->acmd[3] = 0x00;
    cmd_table->acmd[4] = 0x00;
    cmd_table->acmd[5] = 0x00;
    
    cmd_table->acmd[6] = 0x00;
    cmd_table->acmd[7] = 0x00;
    cmd_table->acmd[8] = 0x00;
    cmd_table->acmd[9] = 0x00;
    cmd_table->acmd[10] = 0x00;
    cmd_table->acmd[11] = 0x00;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_PACKET;
    fis->featurel = 0x01;
    fis->featureh = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) return 0;

    port->ci = (1 << slot);

    while (1) {

        if (port->is) {
            port->is = port->is;
        }

        if ((port->ci & (1 << slot)) == 0) break;
        if (port->tfd & 0x01) {
            return 0; 
        }
    }

    satapi_capacity_data_t* capacity = (satapi_capacity_data_t*)phys_buffer;

    int capacity_sectors = swap_uint32(capacity->lba_max_big) + 1;

    free_page(phys_buffer);
    return capacity_sectors;
}

int ahci_atapi_get_sectors(int port) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahci_atapi_get_sectors_port(&abar->ports[port]);
}

int ahci_wait_port_ready(ahci_port_t* port) {
    int timeout_ms = 5000;
    while ((port->tfd & (0x80 | 0x08)) && timeout_ms > 0) {
        sleep_ms(1);
        timeout_ms--;
    }
    return (timeout_ms > 0);
}

int ahci_wait_port(int port) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahci_wait_port_ready(&abar->ports[port]);
}

int ahci_atapi_test_unit_ready_dma(ahci_port_t* port) {
    uint64_t phys_buffer = alloc_page();
    if (!phys_buffer) return 0;

    port->is = 0xFFFF;
    int slot = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = 5;
    cmd_header->w = 0;
    cmd_header->prdtl = 1;
    cmd_header->a = 1;

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = phys_buffer;
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = 512 - 1;
    cmd_table->prdt[0].i = 1;

    cmd_table->acmd[0] = 0x00;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_PACKET;
    fis->featurel = 0x01;
    fis->featureh = 0;
    fis->lba0 = 0;
    fis->lba1 = 0;
    fis->lba2 = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) { free_page(phys_buffer); return 0; }

    port->ci = (1 << slot);

    while (1) {
        if (port->is) {
            port->is = port->is;
        }
        
        if (port->serr) {
            port->serr = port->serr;
        }

        volatile uint32_t* hba_is = (volatile uint32_t*)((uintptr_t)ahci_base_phys + 0x08);
        if (*hba_is) {
            *hba_is = *hba_is;
        }

        if ((port->ci & (1 << slot)) == 0) break;

        if ((port->tfd & 0x01) && ((port->tfd >> 8) & 0xFF) != 0x60) {
            free_page(phys_buffer);
            return 0; 
        }
    }

    int success = 1;
    while (1) {
        if (port->is) { port->is = port->is; }
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->tfd & 0x01) {
            success = 0;
            break;
        }
    }

    free_page(phys_buffer);
    return success; 
}

int ahci_atapi_test_ready(int port) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;

    int ready = 0;
    
    for (int retry = 0; retry < 50; retry++) {
        int status = ahci_atapi_test_unit_ready_dma(&abar->ports[port]);
        
        if (status == 1) {
            ready = 1;
            break;
        }
        
        int err_code = -status;
        
        tprintf(20, 22, 0x1F, "TUR Err: %x, Retry: %d", err_code, retry);

        sleep_ms(100); 

        ahci_atapi_request_sense(port);
    }

    if (!ready) {
        return 0; 
    } else {
        return 1;
    }
}

int ahci_atapi_request_sense_dma(ahci_port_t* port) {
    uint64_t phys_buffer = alloc_page();
    if (!phys_buffer) return 0;

    port->is = 0xFFFF;
    int slot = 0;

    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)(uintptr_t)port->clb;
    ahci_cmd_header_t* cmd_header = &cmd_list[slot];

    cmd_header->cfl = 5;
    cmd_header->w = 0;
    cmd_header->prdtl = 1;
    cmd_header->a = 1;

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)(uintptr_t)cmd_header->ctba;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));

    cmd_table->prdt[0].dba = phys_buffer;
    cmd_table->prdt[0].dbau = 0;
    cmd_table->prdt[0].dbc = 512 - 1;
    cmd_table->prdt[0].i = 1;

    cmd_table->acmd[0] = 0x03;
    cmd_table->acmd[1] = 0x00;
    cmd_table->acmd[2] = 0x00;
    cmd_table->acmd[3] = 0x00;
    cmd_table->acmd[4] = 0x60;
    cmd_table->acmd[5] = 0x00;

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&cmd_table->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = ATA_PACKET;
    fis->featurel = 0x01;
    fis->featureh = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (0x80 | 0x08)) && --timeout);
    if (timeout == 0) { free_page(phys_buffer); return 0; }

    port->ci = (1 << slot);

    while (1) {
        if (port->is) { port->is = port->is; }
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->tfd & 0x01) {
            tprintf(15, 11, 0x1F, "RS Hard Err! TFD: %x", port->tfd);
            free_page(phys_buffer);
            return 0; 
        }
    }

    free_page(phys_buffer);
    return 1; 
}

int ahci_atapi_request_sense(int port) {
    ahci_abar_t* abar = (ahci_abar_t*)ahci_base_phys;
    return ahci_atapi_request_sense_dma(&abar->ports[port]);
}