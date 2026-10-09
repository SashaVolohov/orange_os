#include <stdint.h>

typedef struct {
    uint32_t clb;       // 0x00: Command List Base address (Физический, выровнен по 1024 байта)
    uint32_t clbu;      // 0x04: Command List Base upper 32-bits (для 64-битных систем)
    uint32_t fb;        // 0x08: FIS Base address (Физический, выровнен по 256 байт)
    uint32_t fbu;       // 0x0C: FIS Base upper 32-bits
    uint32_t is;        // 0x10: Interrupt Status
    uint32_t ie;        // 0x14: Interrupt Enable
    uint32_t cmd;       // 0x18: Command and Status
    uint32_t rsv0;      // 0x1C: Зарезервировано
    uint32_t tfd;       // 0x20: Task File Data (Статус ATA: BSY, DRQ и т.д.)
    uint32_t sig;       // 0x24: Signature (Определяет тип устройства: SATA, ATAPI...)
    uint32_t ssts;      // 0x28: SATA Status (Подключение, скорость)
    uint32_t sctl;      // 0x2C: SATA Control
    uint32_t serr;      // 0x30: SATA Error
    uint32_t sact;      // 0x34: SATA Active (для NCQ)
    uint32_t ci;        // 0x38: Command Issue (Запуск команды, биты 0-31)
    uint32_t sntf;      // 0x3C: SATA Notification
    uint32_t fbs;       // 0x40: FIS-based Switching Control
    uint32_t rsv1[11];  // 0x44 - 0x6F: Зарезервировано
    uint32_t vendor[4]; // 0x70 - 0x7F: Специфичные для вендора регистры
} volatile ahci_port_t;

// Глобальные регистры AHCI контроллера (HBA Memory Space)
typedef struct {
    uint32_t cap;       // 0x00: Host Capabilities
    uint32_t ghc;       // 0x04: Global Host Control (Здесь включается AHCI и делается Reset)
    uint32_t is;        // 0x08: Interrupt Status (Глобальные прерывания)
    uint32_t pi;        // 0x0C: Ports Implemented (Битовая маска доступных портов)
    uint32_t vs;        // 0x10: Version
    uint32_t ccc_ctl;   // 0x14: Command Completion Coalescing Control
    uint32_t ccc_pts;   // 0x18: Command Completion Coalescing Ports
    uint32_t em_loc;    // 0x1C: Enclosure Management Location
    uint32_t em_ctl;    // 0x20: Enclosure Management Control
    uint32_t cap2;      // 0x24: Host Capabilities Extended
    uint32_t bohc;      // 0x28: BIOS/OS Handoff Control and Status
    uint8_t  rsv[116];  // 0x2C - 0x9F: Зарезервировано
    uint8_t  vendor[96];// 0xA0 - 0xFF: Специфичные для вендора регистры
    ahci_port_t ports[32]; // 0x100: Массив регистров портов
} volatile ahci_abar_t;

typedef struct {
    uint8_t cfl:5;      // Command FIS Length (длина FIS в двойных словах dwords, для RegH2D обычно 5)
    uint8_t a:1;        // ATAPI (0 для SATA HDD, 1 для оптических приводов)
    uint8_t w:1;        // Write (0 — чтение из диска в ОЗУ, 1 — запись из ОЗУ на диск)
    uint8_t p:1;        // Prefetchable

    uint8_t b:1;        // Busy Clear Trigger
    uint8_t c:1;        // Clear Busy on R_OK
    uint8_t r:1;        // Reset
    uint8_t pmp:4;      // Port Multiplier Port
    uint8_t prdtl_low:1;// Часть зарезервирована

    uint16_t prdtl;     // PRDT Length (сколько регионов памяти/буферов ОЗУ привязано к этой команде)
    
    volatile uint32_t prdbc; // PRD Byte Count (контроллер сам запишет сюда число переданных байт)
    
    uint32_t ctba;      // Command Table Base Address (ФИЗИЧЕСКИЙ адрес самой таблицы команды, выровнен по 128 байт)
    uint32_t ctbau;     // Command Table Base Address Upper (верхние 32 бита для 64-битных систем)
    
    uint32_t rsv1[4];   // Зарезервировано
} __attribute__((packed)) ahci_cmd_header_t;

typedef struct {
    uint32_t dba;       // Data Base Address (ФИЗИЧЕСКИЙ адрес буфера в ОЗУ, куда/откуда копировать данные)
    uint32_t dbau;      // Data Base Address Upper (верхние 32 бита)
    uint32_t rsv0;      // Зарезервировано
    uint32_t dbc:22;    // Byte Count (размер буфера в байтах минус 1. Например, для 512 байт пишем 511)
    uint32_t rsv1:9;    // Зарезервировано
    uint32_t i:1;       // Interrupt on Completion (1 — сгенерировать прерывание, когда этот регион заполнится)
} __attribute__((packed)) ahci_prdt_entry_t;

// Вся таблица команды целиком
typedef struct {
    uint8_t cfis[64];   // Command FIS (сюда мы запишем FIS-пакет типа Register Host to Device)
    uint8_t acmd[16];   // ATAPI Command (зарезервировано для ATAPI)
    uint8_t rsv[48];    // Зарезервировано
    ahci_prdt_entry_t prdt[1]; // Массив буферов (размер зависит от prdtl в заголовке, для тестов хватит 1)
} __attribute__((packed)) ahci_cmd_table_t;

typedef struct {
    uint8_t fis_type;
    
    uint8_t pmport:4;
    uint8_t rsv0:3;
    uint8_t c:1;
    
    uint8_t command;
    uint8_t featurel;
    
    // LBA адрес диска (разделен на байты)
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    
    uint32_t rsv1;
} __attribute__((packed)) fis_reg_h2d_t;

typedef struct {
    uint16_t config;             // Слово 0
    uint16_t reserved1[82];      // Слова 1-82
    uint16_t command_sets;       // Слово 83 (Бит 10 указывает на поддержку LBA48)
    uint16_t reserved2[16];      // Слова 84-99
    uint64_t lba48_sectors;      // Слова 100-103: ОБЩЕЕ КОЛИЧЕСТВО СЕКТОРОВ ДЛЯ LBA48 (64 бита)
    uint16_t reserved3[152];     // Остальные слова до 256
} __attribute__((packed)) ahci_identify_t;

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t device_type      : 5; // Тип устройства (0x05 для CD/DVD-ROM)
    uint8_t peripheral_qual  : 3; // Квалификатор периферии

    uint8_t rmb              : 1; // Removable Medium (1 = диск можно извлечь)
    uint8_t device_type_mod  : 7; // Модификатор типа устройства

    uint8_t version;              // Версия стандарта SCSI
    
    uint8_t response_format  : 4; // Формат ответа (обычно 2 для SCSI-2/SPC)
    uint8_t hi_sup           : 1;
    uint8_t norm_aca         : 1;
    uint8_t reserved1        : 2;

    uint8_t additional_length;    // Сколько еще байт данных доступно (обычно >= 31)
    
    uint8_t sccs             : 1;
    uint8_t acc              : 1;
    uint8_t tpgs             : 2;
    uint8_t pc3              : 1;
    uint8_t reserved2        : 2;
    uint8_t protect          : 1;

    uint8_t bque             : 1;
    uint8_t enc_serv         : 1;
    uint8_t vs               : 1;
    uint8_t multi_p          : 1;
    uint8_t m_chnl           : 1;
    uint8_t reserved3        : 1;
    uint8_t addr16           : 1;
    uint8_t reserved4        : 1;

    uint8_t vs2              : 1;
    uint8_t cmd_que          : 1;
    uint8_t reserved5        : 6;

    // --- Важные текстовые поля ---
    char vendor_id[8];            // Производитель (например, "VBOX    ")
    char product_id[16];          // Модель привода (например, "CD-ROM          ")
    char product_rev[4];          // Версия прошивки (например, "1.0 ")
    
    // Остальные байты до 96 зарезервированы или содержат специфичные данные vendor-специфичные данные
    uint8_t vendor_specific[20];
    uint8_t reserved6[40];
} satapi_inquiry_data_t;

typedef struct __attribute__((packed)) {
    uint32_t lba_max_big;
    uint32_t bytes_in_one_sector_big;
} satapi_capacity_data_t;

#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_PACKET              0xA0
#define ATA_CMD_IDENTIFY        0xEC

int init_ahci();

int ahci_read(int port, uint64_t lba, uint16_t count, uint64_t target_phys_buffer);
int ahci_write(int port, uint64_t lba, uint16_t count, uint64_t source_phys_buffer);
uint64_t ahci_get_sectors(int port);
int get_ahch_port(int port);
satapi_inquiry_data_t* ahch_atapi_get_info(int port);
int ahci_atapi_get_sectors(int port);
int ahci_wait_port(int port);
int ahci_atapi_test_ready(int port);
int ahci_atapi_request_sense(int port);