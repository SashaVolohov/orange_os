#include "ata.h"
#include "core.h"
#include "pci.h"

uint16_t bmide_base = 0;

int get_ata_disk_id_on_bus(int drive_port) {
    if (drive_port == 0xA0) {
        return 0;
    } else if (drive_port == 0xB0) {
        return 1;
    }
    return -1;
}

int ide_detect_device_type(uint8_t slave) {
    outb(0x1F6, 0xA0 | (slave ? 0x10 : 0x00));
    for (int i = 0; i < 4; i++) inb(0x1F7);

    uint8_t cyl_low  = inb(0x1F4);
    uint8_t cyl_high = inb(0x1F5);

    if (cyl_low == 0x00 && cyl_high == 0x00) {
        return ATA_DEVICE;
    }
    if (cyl_low == 0x14 && cyl_high == 0xEB) {
        return ATAPI_DEVICE;
    }

    return NO_DEVICE;
}

int init_ata_dma() {
    for (uint64_t i = 0; i < total_devices; i++) {
        if (pci_devices[i].class_code == 0x01 && pci_devices[i].subclass_code == 0x01) {

            uint8_t bus = pci_devices[i].bus;
            uint8_t dev = pci_devices[i].device;
            uint8_t func = pci_devices[i].function;

            uint32_t cmd_status = pci_read_config_32(bus, dev, func, 0x04);
            cmd_status |= (1 << 2); 
            pci_write_config_32(bus, dev, func, 0x04, cmd_status);

            uint32_t bar4 = pci_read_config_32(bus, dev, func, 0x20);
            if (bar4 & 1) {
                bmide_base = bar4 & 0xFFFC;

                int prim_master = ide_detect_device_type(0);
                int prim_slave  = ide_detect_device_type(1);

                uint32_t udma_reg = pci_read_config_32(bus, dev, func, 0x48);

                uint8_t udma_enable_mask = 0;

                if (prim_master == ATA_DEVICE)  udma_enable_mask |= (1 << 0);
                if (prim_slave  == ATA_DEVICE)  udma_enable_mask |= (1 << 1);
                

                udma_reg &= ~0x0F;
                udma_reg |= udma_enable_mask;

                udma_reg &= ~(0x0F0F0000); 
                if (prim_master == ATA_DEVICE) udma_reg |= (0x02 << 16);
                if (prim_slave  == ATA_DEVICE) udma_reg |= (0x02 << 20);

                pci_write_config_32(bus, dev, func, 0x48, udma_reg);

                return 1;
            }
        }
    }
    return 0;
}

void ata_io_wait(void) {
    uint8_t dummy;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(dummy) : "d"((uint16_t)ATA_REG_STATUS));
    __asm__ __volatile__ ("inb %1, %0" : "=a"(dummy) : "d"((uint16_t)ATA_REG_STATUS));
    __asm__ __volatile__ ("inb %1, %0" : "=a"(dummy) : "d"((uint16_t)ATA_REG_STATUS));
    __asm__ __volatile__ ("inb %1, %0" : "=a"(dummy) : "d"((uint16_t)ATA_REG_STATUS));
}

void ata_set_udma3(uint8_t drive) {
    while (inb(0x1F7) & 0x80);
    
    outb(0x1F6, drive);
    ata_io_wait();
    
    outb(0x1F1, 0x03);
    outb(0x1F2, 0x42);
    
    outb(0x1F7, 0xEF);
    
    while (inb(0x1F7) & 0x80);

    uint8_t status = inb(0x1F7);
    if (status & 0x01) {
        inb(0x1F1);
    }

    outb(0x1F6, 0xA0); 
    ata_io_wait();

    outb(0x1F1, 0x00);
    outb(0x1F2, 0x00);
}

int ata_dma_read_sector(uint64_t lba, uint32_t count, uint64_t target_addr, uint8_t slave) {
    if (lba > 0xFFFFFFFFFFFFULL || count == 0) return -1;

    uint64_t prdt_page = alloc_page();
    if (!prdt_page) return -2;
    prd_t* prdt = (prd_t*)prdt_page;

    uint32_t total_bytes = count * 512;
    uint32_t bytes_remaining = total_bytes;
    uint32_t prd_count = 0;

    uint64_t allocated_pages[512]; 

    while (bytes_remaining > 0) {
        if (prd_count >= 512) {
            free_page(prdt_page);
            return -5; 
        }

        uint64_t data_page = alloc_page();
        if (!data_page) {
            for (uint32_t i = 0; i < prd_count; i++) free_page(allocated_pages[i]);
            free_page(prdt_page);
            return -2;
        }
        allocated_pages[prd_count] = data_page;

        uint32_t chunk_size = (bytes_remaining > 4096) ? 4096 : bytes_remaining;

        prdt[prd_count].buffer_phys_addr = (uint32_t)data_page;
        prdt[prd_count].byte_count = (uint16_t)chunk_size;
        prdt[prd_count].reserved = 0;
        prdt[prd_count].last_prd = 0;

        bytes_remaining -= chunk_size;
        prd_count++;
    }

    prdt[prd_count - 1].last_prd = 0x80;

    outl(bmide_base + 0x04, (uint32_t)prdt_page); 
    
    outb(bmide_base + 0x00, 0x08); 
    
    outb(bmide_base + 0x02, 0x06);
    
    if (lba >= 0x0FFFFFFF || count > 256) {
        
        uint8_t drive_select = slave ? 0x50 : 0x40;
        outb(0x1F6, drive_select);
        
        for(int i = 0; i < 4; i++) inb(bmide_base + 0x02);

        outb(0x1F2, (uint8_t)(count >> 8));
        outb(0x1F2, (uint8_t)count);
        
        outb(0x1F3, (uint8_t)(lba >> 24));
        outb(0x1F3, (uint8_t)lba);
        
        outb(0x1F4, (uint8_t)(lba >> 32));
        outb(0x1F4, (uint8_t)(lba >> 8));
        
        outb(0x1F5, (uint8_t)(lba >> 40));
        outb(0x1F5, (uint8_t)(lba >> 16));

        ata_irq14_fired = 0;
        
        outb(0x1F7, ATA_CMD_READ_DMA_EXT);
    } 
    else {
        
        outb(0x1F6, 0xE0 | (slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F));
        
        for(int i = 0; i < 4; i++) inb(bmide_base + 0x02);

        outb(0x1F2, (uint8_t)count);
        outb(0x1F3, (uint8_t)lba);
        outb(0x1F4, (uint8_t)(lba >> 8));
        outb(0x1F5, (uint8_t)(lba >> 16));

        ata_irq14_fired = 0;

        outb(0x1F7, 0xC8);
    }

    outb(bmide_base + 0x00, 0x08 | 0x01);
    
    int status_code = 0; 

    uint64_t start_ticks = timer_ticks;
    uint64_t timeout_ticks = ms_to_ticks(2000);
    
    while (timer_ticks - start_ticks <= timeout_ticks) {
        uint8_t ata_status = inb(0x1F7);
        if (ata_status & (ATA_SR_ERR | ATA_SR_DF)) {
            status_code = -3; 
            break;
        }
        
        uint8_t bmide_status = inb(bmide_base + 0x02);
        if ((bmide_status & 0x04) && !(bmide_status & 0x01) && !(ata_status & ATA_SR_BSY)) {
            outb(bmide_base + 0x02, 0x04);
            break; 
        }
        
        if(!ata_irq14_fired) {
            asm volatile("hlt");
        }
    }
    
    outb(bmide_base + 0x00, 0x00);
    
    if (timer_ticks - start_ticks > timeout_ticks) status_code = -4; // Таймаут
    
    if (status_code == 0) {
        uint8_t* dest = (uint8_t*)target_addr;
        bytes_remaining = total_bytes;

        for (uint32_t i = 0; i < prd_count; i++) {
            uint8_t* src_page = (uint8_t*)allocated_pages[i];
            uint32_t chunk_size = (bytes_remaining > 4096) ? 4096 : bytes_remaining;

            uintptr_t start = (uintptr_t)src_page;
            uintptr_t end = start + chunk_size;
            for (uintptr_t ptr = start; ptr < end; ptr += 64) {
                asm volatile("clflush (%0)" : : "r"(ptr) : "memory");
            }
            asm volatile("mfence" : : : "memory");

            memcpy(dest, src_page, chunk_size);
            dest += chunk_size;
            bytes_remaining -= chunk_size;
        }
    }

    for (uint32_t i = 0; i < prd_count; i++) {
        free_page(allocated_pages[i]);
    }
    free_page(prdt_page);

    return status_code;
}

int ata_dma_write_sector(uint64_t lba, uint32_t count, uint64_t target_addr, uint8_t slave) {
    if (lba > 0xFFFFFFFFFFFFULL || count == 0) return -1;

    uint64_t prdt_page = alloc_page();
    if (!prdt_page) return -2;
    prd_t* prdt = (prd_t*)prdt_page;

    uint32_t total_bytes = count * 512;
    uint32_t bytes_remaining = total_bytes;
    uint32_t prd_count = 0;

    uint64_t allocated_pages[512]; 

    uint8_t* src = (uint8_t*)target_addr;
    
    while (bytes_remaining > 0) {
        if (prd_count >= 512) {
            free_page(prdt_page);
            return -5;
        }

        uint64_t data_page = alloc_page();
        if (!data_page) {
            for (uint32_t i = 0; i < prd_count; i++) free_page(allocated_pages[i]);
            free_page(prdt_page);
            return -2;
        }
        allocated_pages[prd_count] = data_page;

        uint32_t chunk_size = (bytes_remaining > 4096) ? 4096 : bytes_remaining;

        memcpy((void*)data_page, src, chunk_size);
        src += chunk_size;

        uintptr_t start = (uintptr_t)data_page;
        uintptr_t end = start + chunk_size;
        for (uintptr_t ptr = start; ptr < end; ptr += 64) {
            asm volatile("clflush (%0)" : : "r"(ptr) : "memory");
        }
        asm volatile("mfence" : : : "memory");

        prdt[prd_count].buffer_phys_addr = (uint32_t)data_page;
        prdt[prd_count].byte_count = (uint16_t)chunk_size;
        prdt[prd_count].reserved = 0;
        prdt[prd_count].last_prd = 0; 

        bytes_remaining -= chunk_size;
        prd_count++;
    }

    prdt[prd_count - 1].last_prd = 0x80;

    outl(bmide_base + 0x04, (uint32_t)prdt_page); 
    outb(bmide_base + 0x00, 0x00);
    outb(bmide_base + 0x02, 0x06);
    
    while (inb(0x1F7) & ATA_SR_BSY);
    
    if (lba >= 0x0FFFFFFF || count > 256) {
        
        uint8_t drive_select = slave ? 0x50 : 0x40;
        outb(0x1F6, drive_select); 
        
        for(int i = 0; i < 4; i++) inb(bmide_base + 0x02);

        outb(0x1F2, (uint8_t)(count >> 8));
        outb(0x1F2, (uint8_t)count);
        
        outb(0x1F3, (uint8_t)(lba >> 24));
        outb(0x1F3, (uint8_t)lba);
        
        outb(0x1F4, (uint8_t)(lba >> 32));
        outb(0x1F4, (uint8_t)(lba >> 8));
        
        outb(0x1F5, (uint8_t)(lba >> 40));
        outb(0x1F5, (uint8_t)(lba >> 16));

        ata_irq14_fired = 0;
        
        outb(0x1F7, ATA_CMD_WRITE_DMA_EXT);
    } 
    else {
        outb(0x1F6, 0xE0 | (slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F));
        
        for(int i = 0; i < 4; i++) inb(bmide_base + 0x02);

        outb(0x1F2, (uint8_t)count);
        outb(0x1F3, (uint8_t)lba);
        outb(0x1F4, (uint8_t)(lba >> 8));
        outb(0x1F5, (uint8_t)(lba >> 16));

        ata_irq14_fired = 0;

        outb(0x1F7, 0xCA);
    }
    
    outb(bmide_base + 0x00, 0x01);
    
    int status_code = 0; 

    uint64_t start_ticks = timer_ticks;
    uint64_t timeout_ticks = ms_to_ticks(2000);
    
    while (timer_ticks - start_ticks <= timeout_ticks) {
        uint8_t ata_status = inb(0x1F7);
        if (ata_status & (ATA_SR_ERR | ATA_SR_DF)) {
            status_code = -3;
            break;
        }
        
        uint8_t bmide_status = inb(bmide_base + 0x02);
        if ((bmide_status & 0x04) && !(bmide_status & 0x01) && !(ata_status & ATA_SR_BSY)) {
            outb(bmide_base + 0x02, 0x04);
            break; 
        }
        
        if (!ata_irq14_fired) {
            asm volatile("hlt"); 
        }
    }
    
    outb(bmide_base + 0x00, 0x00);
    
    if (timer_ticks - start_ticks > timeout_ticks) status_code = -4;
    
    for (uint32_t i = 0; i < prd_count; i++) {
        free_page(allocated_pages[i]);
    }
    free_page(prdt_page);

    return status_code;
}

int atapi_dma_read_sector(uint64_t lba, uint32_t count, uint64_t target_addr, uint8_t slave) {
    if (count == 0) return -1;

    uint32_t total_bytes = count * 2048;
    uint32_t bytes_remaining = total_bytes;
    uint32_t prd_count = 0;

    uint64_t prdt_page = alloc_page();
    if (!prdt_page) return -2;
    prd_t* prdt = (prd_t*)prdt_page;

    uint64_t allocated_pages[512]; 

    while (bytes_remaining > 0) {
        if (prd_count >= 512) {
            free_page(prdt_page);
            return -5; 
        }

        uint64_t data_page = alloc_page();
        if (!data_page) {
            for (uint32_t i = 0; i < prd_count; i++) free_page(allocated_pages[i]);
            free_page(prdt_page);
            return -2;
        }
        allocated_pages[prd_count] = data_page;

        uint32_t chunk_size = (bytes_remaining > 4096) ? 4096 : bytes_remaining;

        prdt[prd_count].buffer_phys_addr = (uint32_t)data_page;
        prdt[prd_count].byte_count = (uint16_t)chunk_size;
        prdt[prd_count].reserved = 0;
        prdt[prd_count].last_prd = 0;

        bytes_remaining -= chunk_size;
        prd_count++;
    }
    prdt[prd_count - 1].last_prd = 0x80;

    outl(bmide_base + 0x04, (uint32_t)prdt_page); 
    outb(bmide_base + 0x00, 0x08);
    outb(bmide_base + 0x02, 0x06);

    // Выбор устройства (0xA0 маска)
    outb(0x1F6, 0xA0 | (slave ? 0x10 : 0x00));
    for(int i = 0; i < 4; i++) inb(bmide_base + 0x02);

    outb(0x1F1, 0x01);
    outb(0x1F2, 0x00);
    outb(0x1F3, 0x00);

    outb(0x1F4, 0xFE); 
    outb(0x1F5, 0xFF); 

    outb(0x1F7, 0xA0); 

    uint8_t status = 0;
    uint64_t wait_start = timer_ticks;
    while (timer_ticks - wait_start <= ms_to_ticks(500)) {
        status = inb(0x3F6);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
    }
    
    if ((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ) || (status & ATA_SR_ERR)) {
        for (uint32_t i = 0; i < prd_count; i++) free_page(allocated_pages[i]);
        free_page(prdt_page);
        return -3;
    }

    uint8_t scsi_packet[12] = {0};
    scsi_packet[0] = 0x28; // READ(10)
    scsi_packet[2] = (uint8_t)(lba >> 24);
    scsi_packet[3] = (uint8_t)(lba >> 16);
    scsi_packet[4] = (uint8_t)(lba >> 8);
    scsi_packet[5] = (uint8_t)(lba);
    scsi_packet[7] = (uint8_t)(count >> 8);
    scsi_packet[8] = (uint8_t)(count);

    outb(bmide_base + 0x00, 0x08 | 0x01); 

    uint16_t* packet_words = (uint16_t*)scsi_packet;
    for (int i = 0; i < 6; i++) {
        outw(0x1F0, packet_words[i]);
    }

    ata_irq14_fired = 0;

    int status_code = 0; 
    uint64_t start_ticks = timer_ticks;
    uint64_t timeout_ticks = ms_to_ticks(5000);
    
    while (timer_ticks - start_ticks <= timeout_ticks) {
        uint8_t ata_status = inb(0x1F7); 
        if (ata_status & (ATA_SR_ERR | ATA_SR_DF)) {
            status_code = -3; 
            break;
        }
        
        uint8_t bmide_status = inb(bmide_base + 0x02);
        if ((bmide_status & 0x04) && !(bmide_status & 0x01) && !(ata_status & ATA_SR_BSY)) {
            outb(bmide_base + 0x02, 0x04);
            break; 
        }
        
        if(!ata_irq14_fired) {
            asm volatile("hlt");
        }
    }
    
    outb(bmide_base + 0x00, 0x00);
    
    if (timer_ticks - start_ticks > timeout_ticks) status_code = -4;
    
    if (status_code == 0) {
        uint8_t* dest = (uint8_t*)target_addr;
        uint32_t bytes_remaining = total_bytes;

        for (uint32_t i = 0; i < prd_count; i++) {
            uint8_t* src_page = (uint8_t*)allocated_pages[i];
            uint32_t chunk_size = (bytes_remaining > 4096) ? 4096 : bytes_remaining;

            uintptr_t start = (uintptr_t)src_page;
            uintptr_t end = start + chunk_size;
            for (uintptr_t ptr = start; ptr < end; ptr += 64) {
                asm volatile("clflush (%0)" : : "r"(ptr) : "memory");
            }
            asm volatile("mfence" : : : "memory");

            memcpy(dest, src_page, chunk_size);
            dest += chunk_size;
            bytes_remaining -= chunk_size;
        }
    }

    for (uint32_t i = 0; i < prd_count; i++) {
        free_page(allocated_pages[i]);
    }
    free_page(prdt_page);

    return status_code;
}