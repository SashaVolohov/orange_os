typedef unsigned long long efi_status_t;
typedef void* efi_handle_t;

struct efi_simple_text_output_protocol;

typedef efi_status_t (__attribute__((ms_abi)) *efi_text_string_t) (
    struct efi_simple_text_output_protocol *this,
    const unsigned short *string
);

typedef struct efi_simple_text_output_protocol {
    void* reset;
    efi_text_string_t output_string;
} efi_simple_text_output_protocol_t;

typedef struct {
    unsigned long long signature;
    unsigned int revision;
    unsigned int header_size;
    unsigned int crc32;
    unsigned int reserved;
} efi_table_header_t;

typedef struct {
    unsigned int   data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char  data4[8];
} efi_guid_t;

typedef struct {
    efi_table_header_t hdr;
    void* shortcuts[36];
    
    efi_status_t (__attribute__((ms_abi)) *locate_protocol) (
        efi_guid_t *protocol,
        void *registration,
        void **interface
    );
} efi_boot_services_t;

typedef struct {
    efi_table_header_t hdr;
    const unsigned short* firmware_vendor;
    unsigned int firmware_revision;
    efi_handle_t console_in_handle;
    void* con_in;
    efi_handle_t console_out_handle;
    efi_simple_text_output_protocol_t *con_out;
    void* std_err;
    void* runtime_services;
    efi_boot_services_t *boot_services; 
} efi_system_table_t;

typedef struct {
    unsigned int version;
    unsigned int horizontal_resolution;
    unsigned int vertical_resolution;
    int pixel_format;
    unsigned int pixel_information;
    unsigned int pixels_per_scan_line;
} efi_graphics_output_mode_information_t;

typedef struct {
    unsigned int max_mode;
    unsigned int mode;
    efi_graphics_output_mode_information_t *info;
    unsigned long long size_of_info;
    unsigned long long frame_buffer_base;
    unsigned long long frame_buffer_size;
} efi_graphics_output_protocol_mode_t;

typedef struct {
    void* query_mode;
    void* set_mode;
    void* blt;
    efi_graphics_output_protocol_mode_t *mode;
} efi_graphics_output_protocol_t;

void print_hex_digit(efi_simple_text_output_protocol_t* con_out, unsigned char val) {
    unsigned short str[2];
    if (val < 10) {
        str[0] = (unsigned short)('0' + val);
    } else {
        str[0] = (unsigned short)('A' + (val - 10));
    }
    str[1] = 0;
    con_out->output_string(con_out, str);
}

void print_status(efi_simple_text_output_protocol_t* con_out, efi_status_t status) {
    con_out->output_string(con_out, L"Status: 0x");
    for (int i = 15; i >= 0; i--) {
        unsigned char digit = (status >> (i * 4)) & 0xF;
        print_hex_digit(con_out, digit);
    }
    con_out->output_string(con_out, L"\r\n");
}

__attribute__((ms_abi))
efi_status_t efi_main(efi_handle_t image_handle, efi_system_table_t *system_table) {
    (void)image_handle;

    system_table->con_out->output_string(system_table->con_out, L"Searching for GOP...\r\n");
    efi_guid_t gop_guid = { 
        0x7126830c, 
        0x3830, 
        0x41a5, 
        { 0xa6, 0x21, 0x22, 0x5b, 0x96, 0xa1, 0x91, 0xb0 } 
    };
    
    efi_graphics_output_protocol_t *gop = 0;

    efi_status_t status = system_table->boot_services->locate_protocol(&gop_guid, 0, (void**)&gop);

    print_status(system_table->con_out, status);

    if (status == 0 && gop != 0 && gop->mode != 0) {
        system_table->con_out->output_string(system_table->con_out, L"GOP Found! Filling screen...\r\n");

        unsigned int* fb = (unsigned int*)gop->mode->frame_buffer_base;
        unsigned int pitch = gop->mode->info->pixels_per_scan_line;
        unsigned int height = gop->mode->info->vertical_resolution;

        for (unsigned int i = 0; i < pitch * height; i++) {
            fb[i] = 0x002B2B2B; 
        }
        
        for(unsigned int y=0; y<100; y++) {
            for(unsigned int x=0; x<100; x++) {
                fb[y*pitch + x] = 0x00FF6600;
            }
        }
    } else {
        system_table->con_out->output_string(system_table->con_out, L"GOP initialization failed.\r\n");
    }

    while(1) { __asm__ __volatile__("hlt"); }
    return 0;
}