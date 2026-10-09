#include "nvme.h"
#include "core.h"
#include "os_memory.h"
#include "pci.h"

uintptr_t nvme_base_phys = 0;
volatile nvme_regs_t* nvme_regs = NULL;

nvme_queue_t admin_queue;
nvme_queue_t io_queue;

uint16_t global_cid = 0;

int init_nvme() {
    for (uint64_t i = 0; i < total_devices; i++) {
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

                nvme_base_phys = bar0_phys;

                uint64_t phys_page = nvme_base_phys & ~0xFFF;

                for (int page = 0; page < 4; page++) {
                    uint64_t current_phys = phys_page + (page * 4096);
                    map_page(current_phys, current_phys, 0x1B); 
                }

                return 1; 
            }
        }
    }
    return 0;
}

int nvme_init_admin_queue() {
    uint64_t cap = nvme_regs->cap;
    
    uint32_t dstrd = (cap >> 32) & 0xF;
    uint32_t doorbell_stride = 4 * (1 << dstrd);

    admin_queue.sq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_regs + 0x1000);
    admin_queue.cq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_regs + 0x1000 + doorbell_stride);

    admin_queue.size = 64;
    admin_queue.sq_tail = 0;
    admin_queue.cq_head = 0;
    admin_queue.phase = 1;

    admin_queue.sq = (nvme_cmd_t*)alloc_page();
    admin_queue.cq = (nvme_cpl_t*)alloc_page();

    if (!admin_queue.sq || !admin_queue.cq) {
        return 0;
    }
    
    admin_queue.sq_phys = virtual_to_physical(pt_begin, (uintptr_t)admin_queue.sq);
    admin_queue.cq_phys = virtual_to_physical(pt_begin, (uintptr_t)admin_queue.cq);

    uint32_t aqa = ((admin_queue.size - 1) << 16) | (admin_queue.size - 1);
    nvme_regs->aqa = aqa;

    nvme_regs->asq = admin_queue.sq_phys;
    nvme_regs->acq = admin_queue.cq_phys;

    return 1;
}

int nvme_controller_reset() {
    nvme_regs = (volatile nvme_regs_t*)nvme_base_phys;

    if ((nvme_regs->cc & (1 << 0)) != 0) {
        nvme_regs->cc &= ~(1 << 0);
    }

    int timeout_ms = 1000;
    while ((nvme_regs->csts & (1 << 0)) != 0) {
        if (timeout_ms <= 0) return 0;
        sleep_ms(5);
        timeout_ms -= 5;
    }

    uint64_t cap = nvme_regs->cap;
    
    uint8_t mpsmin = (cap >> 48) & 0xF; 
    if (mpsmin != 0) { 
        return 0; // Размер страниц должен быть не 4 КБ
    }

    uint32_t cc = 0;
    cc |= (0 << 0);
    cc |= (0 << 7);
    cc |= (0 << 16);
    cc |= (0 << 20);
    
    nvme_regs->cc = cc;

    int status = nvme_init_admin_queue();
    if(status == 0) {
        return 0;
    }

    nvme_regs->intms = 0xFFFFFFFF;

    nvme_regs->cc |= (1 << 0);

    timeout_ms = 1000;
    while ((nvme_regs->csts & (1 << 0)) == 0) {
        if (timeout_ms <= 0) return 0;
        sleep_ms(5);
        timeout_ms -= 5;
    }

    return 1;
}

int nvme_submit_admin_cmd(nvme_cmd_t* cmd, nvme_cpl_t* out_cpl) {

    uint16_t tail = admin_queue.sq_tail;
    cmd->cid = tail;

    admin_queue.sq[tail] = *cmd;

    tail++;
    if (tail >= admin_queue.size) tail = 0;
    admin_queue.sq_tail = tail;

    *admin_queue.sq_doorbell = tail;

    volatile uint32_t dummy = nvme_regs->vs; 
    (void)dummy;

    uint16_t head = admin_queue.cq_head;
    volatile nvme_cpl_t* cpl = &admin_queue.cq[head];

    for (int i = 0; i < 50000; i++) {
        if ((cpl->status & 1) == admin_queue.phase) {
            goto command_completed; 
        }
        asm volatile("pause");
    }

    int timeout_ms = 1000; 
    while ((cpl->status & 1) != admin_queue.phase) {
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
    if (head >= admin_queue.size) {
        head = 0;
        admin_queue.phase ^= 1;
    }
    admin_queue.cq_head = head;

    *admin_queue.cq_doorbell = head;

    volatile uint32_t dummy2 = nvme_regs->vs; 
    (void)dummy2;

    return (status_code == 0);
}

void nvme_identify_disk() {

    uint8_t* identify_buffer = (uint8_t*)alloc_page();
    if (!identify_buffer) return;

    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)identify_buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x06;
    cmd.nsid = 0;
    cmd.prp1 = phys_buffer;
    
    cmd.cdw10 = 1; 

    nvme_cpl_t cpl = {0};
    if (nvme_submit_admin_cmd(&cmd, &cpl)) {

        char model[41];
        for (int i = 0; i < 40; i++) {
            model[i] = identify_buffer[24 + i]; 
        }
        model[40] = '\0'; 

        tprintf(10, 10, 0x1F, "NVMe Device Found: %s\n", model);

    } else {
        // ERROR там
    }

    free_page((uintptr_t)identify_buffer);

}

void nvme_identify_namespace() {
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
    if (nvme_submit_admin_cmd(&cmd, &cpl)) {
        uint64_t total_sectors = *(uint64_t*)(&ns_buffer[0]);

        uint8_t flbas = ns_buffer[26];
        uint8_t lba_format_index = flbas & 0x0F;

        uint32_t format_offset = 128 + (lba_format_index * 4);
        uint8_t lbads = ns_buffer[format_offset + 2];

        uint32_t sector_size = 1 << lbads;

        uint64_t disk_size_mb = (total_sectors * sector_size) / (1024 * 1024);

        tprintf(10, 11, 0x1F, "NVMe Size: %d MB (%d sectors x %d bytes)\n", disk_size_mb, total_sectors, sector_size);

    } else {

        // Добавить лог там что всё
    }

    free_page((uintptr_t)ns_buffer);

}

int nvme_create_io_queues() {
    uint64_t cap = nvme_regs->cap;
    uint32_t dstrd = (cap >> 32) & 0xF;
    uint32_t doorbell_stride = 4 * (1 << dstrd);

    uint16_t sq_size = 64;
    uint16_t cq_size = 256;

    io_queue.sq_tail = 0;
    io_queue.cq_head = 0;
    io_queue.phase = 1;

    io_queue.sq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_regs + 0x1008);
    io_queue.cq_doorbell = (volatile uint32_t*)((uintptr_t)nvme_regs + 0x100C);

    io_queue.sq = (nvme_cmd_t*)alloc_page();
    io_queue.cq = (nvme_cpl_t*)alloc_page();
    
    if (!io_queue.sq || !io_queue.cq) return 0;

    for (int i = 0; i < 4096; i++) {
        ((uint8_t*)io_queue.sq)[i] = 0;
        ((uint8_t*)io_queue.cq)[i] = 0;
    }
    for (int i = 0; i < 4096; i += 64) {
        asm volatile("clflush (%0)" :: "r"((uint8_t*)io_queue.sq + i) : "memory");
        asm volatile("clflush (%0)" :: "r"((uint8_t*)io_queue.cq + i) : "memory");
    }
    asm volatile("sfence" ::: "memory");

    io_queue.sq_phys = virtual_to_physical(pt_begin, (uintptr_t)io_queue.sq);
    io_queue.cq_phys = virtual_to_physical(pt_begin, (uintptr_t)io_queue.cq);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x05;                             
    cmd.prp1 = io_queue.cq_phys;                
    cmd.cdw10 = ((cq_size - 1) << 16) | 1; 
    cmd.cdw11 = 1;                              

    nvme_cpl_t cpl = {0};
    if (!nvme_submit_admin_cmd(&cmd, &cpl)) return 0;

    cmd = (nvme_cmd_t){0};
    cmd.opc = 0x01;                             
    cmd.prp1 = io_queue.sq_phys;                
    cmd.cdw10 = ((sq_size - 1) << 16) | 1; 
    cmd.cdw11 = (1 << 16) | 1;                  

    if (!nvme_submit_admin_cmd(&cmd, &cpl)) return 0;

    return 1;
}

int nvme_submit_io_cmd(nvme_cmd_t* cmd, nvme_cpl_t* out_cpl) {
    uint16_t tail = io_queue.sq_tail;
    cmd->cid = tail;

    io_queue.sq[tail] = *cmd;

    asm volatile("clflush (%0)" :: "r"(&io_queue.sq[tail]) : "memory");
    asm volatile("sfence" ::: "memory");

    tail++;
    if (tail >= 64) tail = 0;
    io_queue.sq_tail = tail;

    *io_queue.sq_doorbell = (uint32_t)tail;

    volatile uint32_t dummy = nvme_regs->vs; 
    (void)dummy;

    uint16_t head = io_queue.cq_head;
    volatile nvme_cpl_t* cpl = &io_queue.cq[head];

    for (int i = 0; i < 50000; i++) {
        if ((cpl->status & 1) == io_queue.phase) {
            goto command_completed; 
        }
        asm volatile("pause");
    }

    int print_counter = 0;

    int timeout_ms = 1000; 
    while (cpl->status == 0) {
        if (timeout_ms <= 0) {
            tprintf(18, 0, 0x1F, "[Timeout Dump] Final cpl->status: 0x%x\n", cpl->status);
            return 0;
        }

        if (print_counter++ % 100 == 0) {
            tprintf(19, 0, 0x1F, "Wait read... Raw status field: 0x%x\n", cpl->status);
        }

        sleep_ms(1);
        timeout_ms--;
    }

command_completed:

    if (out_cpl) *out_cpl = *cpl;
    uint16_t status_code = (cpl->status >> 1) & 0x7FFF;

    head++;
    if (head >= 256) {
        head = 0;
        io_queue.phase ^= 1;
    }
    io_queue.cq_head = head;
    *io_queue.cq_doorbell = (uint32_t)head;

    volatile uint32_t dummy2 = nvme_regs->vs; 
    (void)dummy2;

    return (status_code == 0);
}

int nvme_read_sectors(uint64_t lba, uint16_t count, void* buffer) {
    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x02;
    cmd.nsid = 1;
    cmd.prp1 = phys_buffer;

    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)((lba >> 32) & 0xFFFFFFFF);

    cmd.cdw12 = (uint32_t)(count - 1); 

    nvme_cpl_t cpl = {0};
    return nvme_submit_io_cmd(&cmd, &cpl);
}

int nvme_write_sectors(uint64_t lba, uint16_t count, const void* buffer) {
    uint64_t phys_buffer = virtual_to_physical(pt_begin, (uintptr_t)(void*)buffer);

    nvme_cmd_t cmd = {0};
    cmd.opc = 0x01;
    cmd.nsid = 1;
    cmd.prp1 = phys_buffer;

    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)((lba >> 32) & 0xFFFFFFFF);

    cmd.cdw12 = (uint32_t)(count - 1); 

    nvme_cpl_t cpl = {0};
    
    return nvme_submit_io_cmd(&cmd, &cpl); 
}

void nvme_dump_state() {

    static_assert(sizeof(nvme_cmd_t) == 64, "NVMe CMD size must be 64 bytes");

    tprintf(0, 19, 0x1F, "\n--- NVMe Debug Dump ---\n");
    tprintf(0, 20, 0x1F, "CSTS (Status Register): 0x%x\n", nvme_regs->csts);
    tprintf(0, 21, 0x1F, "CC   (Configuration):   0x%x\n", nvme_regs->cc);
    
    tprintf(0, 22, 0x1F, "Software: SQ_Tail=%d, CQ_Head=%d, Phase=%d\n", 
             io_queue.sq_tail, io_queue.cq_head, io_queue.phase);
             
    uint16_t head = io_queue.cq_head;
    tprintf(0, 23, 0x1F, "Current CQ Entry Status Raw: 0x%x\n", io_queue.cq[head].status);
}