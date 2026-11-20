#include "nvme.h"
#include "core.h"
#include "os_memory.h"
#include "pci.h"

nvme_controller_t nvme_controllers[NVME_MAX_CONTROLLERS];
int nvme_controller_count = 0;

int64_t nvme_get_sectors_count(int id) {
    if(id < 0 || id >= nvme_controller_count) return 0;
    return nvme_controllers[id].total_sectors;
}

int init_nvme() {

    int nvme_found = 0;

    for (uint64_t i = 0; i < total_devices; i++) {

        if(nvme_controller_count >= NVME_MAX_CONTROLLERS) {
            return 1;
        }

        if (pci_devices[i].class_code == 0x01 && 
            pci_devices[i].subclass_code == 0x08 && 
            pci_devices[i].prog_if == 0x02) {

            uint8_t bus  = pci_devices[i].bus;
            uint8_t dev  = pci_devices[i].device;
            uint8_t func = pci_devices[i].function;

            uint32_t cmd_status = pci_read_config_32(bus, dev, func, 0x04);
            cmd_status |= (1 << 2) | (1 << 1); 
            pci_write_config_32(bus, dev, func, 0x04, cmd_status);

            uint32_t bar0_low = pci_read_config_32(bus, dev, func, 0x10);
            
            if ((bar0_low & 1) == 0) {
                uint64_t bar0_phys = bar0_low & 0xFFFFFFF0;

                if ((bar0_low & (1 << 2)) != 0) {
                    uint64_t bar0_high = pci_read_config_32(bus, dev, func, 0x14);
                    bar0_phys |= (bar0_high << 32);
                }

                nvme_controllers[nvme_controller_count].base_phys = bar0_phys;

                uint64_t phys_page = nvme_controllers[nvme_controller_count].base_phys & ~0xFFF;

                for (int page = 0; page < 4; page++) {
                    uint64_t current_phys = phys_page + (page * 4096);
                    map_page(current_phys, current_phys, 0x1B); 
                }

                nvme_controller_count++;

                nvme_found = 1;
            }
        }
    }
    return nvme_found;
}

int nvme_init_admin_queue(int id) {
    uint64_t cap = nvme_controllers[id].regs->cap;
    
    uint32_t dstrd = (cap >> 32) & 0xF;
    uint32_t doorbell_stride = 4 * (1 << dstrd);

    nvme_controllers[id].admin_queue.sq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_controllers[id].regs + 0x1000);
    nvme_controllers[id].admin_queue.cq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_controllers[id].regs + 0x1000 + doorbell_stride);

    nvme_controllers[id].admin_queue.size = 64;
    nvme_controllers[id].admin_queue.sq_tail = 0;
    nvme_controllers[id].admin_queue.cq_head = 0;
    nvme_controllers[id].admin_queue.phase = 1;

    nvme_controllers[id].admin_queue.sq = (nvme_cmd_t*)alloc_page();
    nvme_controllers[id].admin_queue.cq = (nvme_cpl_t*)alloc_page();

    if (!nvme_controllers[id].admin_queue.sq || !nvme_controllers[id].admin_queue.cq) {
        return 0;
    }
    
    nvme_controllers[id].admin_queue.sq_phys = virtual_to_physical(pt_begin, (uintptr_t)nvme_controllers[id].admin_queue.sq);
    nvme_controllers[id].admin_queue.cq_phys = virtual_to_physical(pt_begin, (uintptr_t)nvme_controllers[id].admin_queue.cq);

    uint32_t aqa = ((nvme_controllers[id].admin_queue.size - 1) << 16) | (nvme_controllers[id].admin_queue.size - 1);
    nvme_controllers[id].regs->aqa = aqa;

    nvme_controllers[id].regs->asq = nvme_controllers[id].admin_queue.sq_phys;
    nvme_controllers[id].regs->acq = nvme_controllers[id].admin_queue.cq_phys;

    return 1;
}

int nvme_controller_reset(int id) {
    nvme_controllers[id].regs =
        (volatile nvme_regs_t*)nvme_controllers[id].base_phys;

    volatile nvme_regs_t* regs = nvme_controllers[id].regs;

    regs->cc &= ~NVME_CC_EN;

    int timeout_ms = 1000;

    while (regs->csts & NVME_CSTS_RDY) {
        if (regs->csts & NVME_CSTS_CFS)
            return 0;

        if (timeout_ms <= 0)
            return 0;

        sleep_ms(5);
        timeout_ms -= 5;
    }

    uint64_t cap = regs->cap;
    uint8_t mpsmin = (cap >> 48) & 0xF;

    if (mpsmin != 0)
        return 0;

    if (nvme_init_admin_queue(id) == 0)
        return 0;

    regs->intms = 0xFFFFFFFF;

    uint32_t cc = 0;
    cc |= NVME_CC_EN;
    cc |= (6U << 16);
    cc |= (4U << 20);

    regs->cc = cc;

    timeout_ms = 1000;

    while (!(regs->csts & NVME_CSTS_RDY)) {
        uint32_t csts = regs->csts;

        if (csts & NVME_CSTS_CFS)
            return 0;

        if (timeout_ms <= 0)
            return 0;

        sleep_ms(5);
        timeout_ms -= 5;
    }

    return 1;
}

int nvme_submit_admin_cmd(nvme_cmd_t* cmd, nvme_cpl_t* out_cpl, int id) {

    uint16_t tail = nvme_controllers[id].admin_queue.sq_tail;
    cmd->cid = tail;

    nvme_controllers[id].admin_queue.sq[tail] = *cmd;

    tail++;
    if (tail >= nvme_controllers[id].admin_queue.size) tail = 0;
    nvme_controllers[id].admin_queue.sq_tail = tail;

    *nvme_controllers[id].admin_queue.sq_doorbell = tail;

    volatile uint32_t dummy = nvme_controllers[id].regs->vs; 
    (void)dummy;

    uint16_t head = nvme_controllers[id].admin_queue.cq_head;
    volatile nvme_cpl_t* cpl = &nvme_controllers[id].admin_queue.cq[head];

    for (int i = 0; i < 50000; i++) {
        if ((cpl->status & 1) == nvme_controllers[id].admin_queue.phase) {
            goto command_completed; 
        }
        asm volatile("pause");
    }

    int timeout_ms = 1000; 
    while ((cpl->status & 1) != nvme_controllers[id].admin_queue.phase) {
        if (timeout_ms <= 0) {
            return 0; 
        }
        sleep_ms(1);
        timeout_ms--;
    }

command_completed:

    if (out_cpl) *out_cpl = *cpl;

    uint16_t status_code = (cpl->status >> 1) & 0x7FFF;

    head++;
    if (head >= nvme_controllers[id].admin_queue.size) {
        head = 0;
        nvme_controllers[id].admin_queue.phase ^= 1;
    }
    nvme_controllers[id].admin_queue.cq_head = head;

    *nvme_controllers[id].admin_queue.cq_doorbell = head;

    volatile uint32_t dummy2 = nvme_controllers[id].regs->vs; 
    (void)dummy2;

    return (status_code == 0);
}

void nvme_identify_disk(int id) {

    uint8_t* identify_buffer = (uint8_t*)alloc_page();
    if (!identify_buffer) return;

    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)identify_buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x06;
    cmd.nsid = 0;
    cmd.prp1 = phys_buffer;
    
    cmd.cdw10 = 1; 

    nvme_cpl_t cpl = {0};
    if (nvme_submit_admin_cmd(&cmd, &cpl, id)) {

        for (int i = 0; i < 40; i++) {
            nvme_controllers[id].model[i] = identify_buffer[24 + i]; 
        }
        nvme_controllers[id].model[40] = '\0'; 

    }

    free_page((uintptr_t)identify_buffer);

}

void nvme_identify_namespace(int id) {
    uint8_t* ns_buffer = (uint8_t*)alloc_page(); 
    if (!ns_buffer) return;
    
    for(int i = 0; i < 4096; i++) ns_buffer[i] = 0;

    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)ns_buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x06;
    cmd.nsid = 1;
    cmd.prp1 = phys_buffer;
    cmd.cdw10 = 0;

    nvme_cpl_t cpl = {0};
    if (nvme_submit_admin_cmd(&cmd, &cpl, id)) {
        uint64_t total_sectors = *(uint64_t*)(&ns_buffer[0]);

        uint8_t flbas = ns_buffer[26];
        uint8_t lba_format_index = flbas & 0x0F;

        uint32_t format_offset = 128 + (lba_format_index * 4);
        uint8_t lbads = ns_buffer[format_offset + 2];

        uint32_t sector_size = 1 << lbads;

        uint64_t disk_size_mb = (total_sectors * sector_size) / (1024 * 1024);
        nvme_controllers[id].disk_size_mb = disk_size_mb;
        nvme_controllers[id].total_sectors = total_sectors;

    } else {

        // Добавить лог там что всё
    }

    free_page((uintptr_t)ns_buffer);

}

int nvme_create_io_queues(int id) {
    uint64_t cap = nvme_controllers[id].regs->cap;
    uint32_t dstrd = (cap >> 32) & 0xF;
    uint32_t doorbell_stride = 4 * (1 << dstrd);

    uint16_t sq_size = 64;
    uint16_t cq_size = 64;
    nvme_controllers[id].io_queue.size = sq_size;

    nvme_controllers[id].io_queue.sq_tail = 0;
    nvme_controllers[id].io_queue.cq_head = 0;
    nvme_controllers[id].io_queue.phase = 1;

    uint32_t io_qid = 1;
    nvme_controllers[id].io_queue.sq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_controllers[id].regs + 0x1000 + (2 * io_qid * doorbell_stride));
    nvme_controllers[id].io_queue.cq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_controllers[id].regs + 0x1000 + ((2 * io_qid + 1) * doorbell_stride));

    nvme_controllers[id].io_queue.sq = (nvme_cmd_t*)alloc_page();
    nvme_controllers[id].io_queue.cq = (nvme_cpl_t*)alloc_page();
    
    if (!nvme_controllers[id].io_queue.sq || !nvme_controllers[id].io_queue.cq) return 0;

    for (int i = 0; i < 4096; i++) {
        ((uint8_t*)nvme_controllers[id].io_queue.sq)[i] = 0;
        ((uint8_t*)nvme_controllers[id].io_queue.cq)[i] = 0;
    }
    for (int i = 0; i < 4096; i += 64) {
        asm volatile("clflush (%0)" :: "r"((uint8_t*)nvme_controllers[id].io_queue.sq + i) : "memory");
        asm volatile("clflush (%0)" :: "r"((uint8_t*)nvme_controllers[id].io_queue.cq + i) : "memory");
    }
    asm volatile("sfence" ::: "memory");

    nvme_controllers[id].io_queue.sq_phys = virtual_to_physical(pt_begin, (uintptr_t)nvme_controllers[id].io_queue.sq);
    nvme_controllers[id].io_queue.cq_phys = virtual_to_physical(pt_begin, (uintptr_t)nvme_controllers[id].io_queue.cq);

    *(volatile uint32_t*)((uintptr_t)nvme_controllers[id].regs + 0x10) = 0xFFFFFFFF;
    asm volatile("mfence" ::: "memory");

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x05;                             
    cmd.prp1 = nvme_controllers[id].io_queue.cq_phys;                
    cmd.cdw10 = ((cq_size - 1) << 16) | 1; 
    cmd.cdw11 = 1;                              

    nvme_cpl_t cpl = {0};
    if (!nvme_submit_admin_cmd(&cmd, &cpl, id)) return 0;

    cmd = (nvme_cmd_t){0};
    cmd.opc = 0x01;                             
    cmd.prp1 = nvme_controllers[id].io_queue.sq_phys;                
    cmd.cdw10 = ((sq_size - 1) << 16) | 1; 
    cmd.cdw11 = (1 << 16) | 1;

    if (!nvme_submit_admin_cmd(&cmd, &cpl, id)) return 0;

    return 1;
}

int nvme_submit_io_cmd(nvme_cmd_t* cmd, nvme_cpl_t* out_cpl, int id) {
    uint16_t tail = nvme_controllers[id].io_queue.sq_tail; 
    cmd->cid = tail; 
    cmd->prp2 = 0; 

    nvme_controllers[id].io_queue.sq[tail] = *cmd;

    asm volatile("clflush %0" : : "m"(nvme_controllers[id].io_queue.sq[tail]) : "memory");
    asm volatile("sfence" ::: "memory");

    tail++;
    if (tail >= 64) tail = 0;
    nvme_controllers[id].io_queue.sq_tail = tail;

    *(volatile uint32_t*)(nvme_controllers[id].io_queue.sq_doorbell) = (uint32_t)tail;
    asm volatile("mfence" ::: "memory");

    volatile uint32_t dummy = nvme_controllers[id].regs->vs; 
    (void)dummy;

    uint16_t head = nvme_controllers[id].io_queue.cq_head;
    volatile nvme_cpl_t* cpl = &nvme_controllers[id].io_queue.cq[head];

    volatile int loops = 0;
    int timeout_ms = 2000;

    while (1) {
        asm volatile("clflush %0" : : "m"(*cpl) : "memory");
        asm volatile("mfence" ::: "memory");

        if (loops % 1000 == 0) {
            if (lapic_base_addr != 0) {
                *(volatile uint32_t*)((uintptr_t)lapic_base_addr + 0xB0) = 0;
            }
        }

        if ((cpl->status & 1) == nvme_controllers[id].io_queue.phase) {
            goto command_completed;
        }

        if (loops++ > 50000) { 
            loops = 0;
            sleep_ms(1);
            timeout_ms--;
            
            if (timeout_ms <= 0) {
                return 0;
            }
        }
        asm volatile("pause");
    }

command_completed:
    if (out_cpl) *out_cpl = *cpl;
    
    uint16_t status_code = (cpl->status >> 1) & 0x7FFF;

    head++;
    if (head >= 64) { 
        head = 0;
        nvme_controllers[id].io_queue.phase ^= 1; 
    }
    nvme_controllers[id].io_queue.cq_head = head;
    
    *(volatile uint32_t*)(nvme_controllers[id].io_queue.cq_doorbell) = (uint32_t)head;
    asm volatile("sfence" ::: "memory");

    if (lapic_base_addr != 0) {
        *(volatile uint32_t*)((uintptr_t)lapic_base_addr + 0xB0) = 0;
    }
    asm volatile("mfence" ::: "memory");

    volatile uint32_t dummy2 = nvme_controllers[id].regs->vs;
    (void)dummy2;

    return (status_code == 0);
}

int nvme_read_sectors(uint64_t lba, uint16_t count, void* buffer, int id) {
    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x02;
    cmd.nsid = 1;
    cmd.prp1 = phys_buffer;

    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)((lba >> 32) & 0xFFFFFFFF);

    cmd.cdw12 = (uint32_t)(count - 1); 

    nvme_cpl_t cpl = {0};
    return nvme_submit_io_cmd(&cmd, &cpl, id);
}

int nvme_write_sectors(uint64_t lba, uint16_t count, const void* buffer, int id) {
    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)(void*)buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x01;
    cmd.nsid = 1;
    cmd.prp1 = phys_buffer;

    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)((lba >> 32) & 0xFFFFFFFF);

    cmd.cdw12 = (uint32_t)(count - 1); 

    nvme_cpl_t cpl = {0};
    
    return nvme_submit_io_cmd(&cmd, &cpl, id); 
}