#include <stdint.h>

typedef struct {
    uint64_t cap;
    uint32_t vs;
    uint32_t intms;
    uint32_t intmc;
    uint32_t cc;
    uint32_t rsvd1;
    uint32_t csts;
    uint32_t nssr;
    uint32_t aqa;
    uint64_t asq;
    uint64_t acq;
    uint32_t cmbloc;
    uint32_t cmbsz;
} __attribute__((packed)) nvme_regs_t;

typedef struct {
    uint8_t  opc;
    uint8_t  flags;
    uint16_t cid;
    uint32_t nsid;
    uint64_t rsvd1;
    uint64_t mptr;
    uint64_t prp1;
    uint64_t prp2;
    
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed)) nvme_cmd_t;

typedef struct {
    uint32_t cdw0;
    uint32_t rsvd;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t cid;
    uint16_t status;
} __attribute__((packed)) nvme_cpl_t;

typedef struct {
    nvme_cmd_t* sq;
    nvme_cpl_t* cq;
    uint64_t    sq_phys;
    uint64_t    cq_phys;
    uint16_t    size;
    uint16_t    sq_tail;
    uint16_t    cq_head;
    uint8_t     phase;
    
    volatile uint32_t* sq_doorbell;
    volatile uint32_t* cq_doorbell;
} nvme_queue_t;

int init_nvme();
int nvme_controller_reset();
void nvme_identify_disk();
void nvme_identify_namespace();
int nvme_create_io_queues();
int nvme_read_sectors(uint64_t lba, uint16_t count, void* buffer);
int nvme_write_sectors(uint64_t lba, uint16_t count, const void* buffer);
void nvme_dump_state();