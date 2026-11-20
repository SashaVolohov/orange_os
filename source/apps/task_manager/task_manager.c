#include "core.h"
#include "task_manager.h"

#include "Am79C973.h"

#include "https.h"

int hover_process = -1;
int selected_process = -1;
static uint32_t history_head = 0;
int last_cmos_times = 0;

int drawn_rows = 0;

static uint64_t last_sample_tick = 0;

static uint32_t mem_history[GRAPH_HISTORY_LEN] = {0};

static void update_memory_history() {
    if (cmos_ticks - last_sample_tick >= 100) {
        last_sample_tick = cmos_ticks;
        
        mem_history[history_head] = get_used_memory_mb_no_popcnt();
        history_head = (history_head + 1) % GRAPH_HISTORY_LEN;
    }
}

button_t end_process_button = { .rel_x = 305,  .rel_y = 250,  .width = 90, .height = 25, .text = "End process", .is_pressed = 0 };

void draw_task_manager_content(const window_t* win, int bx, int by, int bw, int bh) {

    update_memory_history();
    
    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    if(dhcp_supplied_address(&g_am79c973_netif)) {
        uint32_t ip = ip4_addr_get_u32(netif_ip4_addr(&g_am79c973_netif));

        win_printf(win, 5, 125, 0x00, 0x00, 0x00, 0xFF, "Network: %d packets received, %d sent, IP: %u.%u.%u.%u", internet_packages_received, internet_packages_sended, ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
    } else {
        win_printf(win, 5, 125, 0x00, 0x00, 0x00, 0xFF, "Network: %d packets received, %d sent. DHCP is not working", internet_packages_received, internet_packages_sended);
    }

    pcb_t* tasks = process_table_ptr; 
    
    drawn_rows = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_t* task = &tasks[i];

        if (task->state == TASK_STATE_EMPTY || task->state == TASK_STATE_ZOMBIE) {
            continue;
        }

        uint8_t bg_r = 0x00, bg_g = 0x00, bg_b = 0x00, bg_a = 0x00;
        uint8_t text_color = 0x00;

        if (hover_process == drawn_rows || selected_process == drawn_rows) {
            bg_r = 0x00;
            bg_g = 0x56;
            bg_b = 0xb3;
            bg_a = 0xFF;
            text_color = 0xFF;
        }

        int y_pos = 140 + (15 * drawn_rows);

        draw_window_rect(win, 5, y_pos, win->width - 20, 15, bg_r, bg_g, bg_b, bg_a);

        const char* proc_name = task->name ? task->name : "Unknown";

        win_printf(win, 5, y_pos, text_color, text_color, text_color, 0xFF, 
                   "%s, PID: %d", proc_name, task->pid);

        drawn_rows++;
    }

    int gx = 5;
    int gy = 20;
    int gw = GRAPH_HISTORY_LEN;
    int gh = GRAPH_HEIGHT;

    uint32_t used_mb = get_used_memory_mb_no_popcnt();
    uint32_t total_mb = mb_memory;
    win_printf(win, gx, 5, 0x00, 0x00, 0x00, 255, "Memory Usage: %d MB / %d MB", used_mb, total_mb);

    draw_window_rect(win, gx, gy, gw, gh, 10, 25, 15, 220);

    for (int x = 25; x < gw; x += 25) {
        draw_window_line(win, gx + x, gy, gx + x, gy + gh, 20, 60, 30, 180);
    }
    for (int y = 25; y < gh; y += 25) {
        draw_window_line(win, gx, gy + y, gx + gw, gy + y, 20, 60, 30, 180);
    }
    int prev_x = 0;
    int prev_y = 0;

    for (int i = 0; i < gw; i++) {
        uint32_t idx = (history_head + i) % GRAPH_HISTORY_LEN;
        uint32_t val = mem_history[idx];

        int bar_h = (int)(((uint64_t)val * gh) / mb_memory);
        if (bar_h > gh) bar_h = gh;

        int cur_x = gx + i;
        int cur_y = (gy + gh) - bar_h;

        if (bar_h > 0) {
            draw_window_line(win, cur_x, cur_y, cur_x, gy + gh, 0, 220, 80, 65);
        }

        if (i > 0) {
            draw_window_line(win, prev_x, prev_y, cur_x, cur_y, 50, 255, 100, 255);
        }

        prev_x = cur_x;
        prev_y = cur_y;
    }

    draw_window_line(win, gx, gy, gx + gw, gy, 80, 140, 90, 255);
    draw_window_line(win, gx, gy + gh, gx + gw, gy + gh, 80, 140, 90, 255);
    draw_window_line(win, gx, gy, gx, gy + gh, 80, 140, 90, 255);
    draw_window_line(win, gx + gw, gy, gx + gw, gy + gh, 80, 140, 90, 255);

    draw_window_button(win, 0, 0, &end_process_button);
}

void mouse_draw_task_manager_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {

    if(is_button_clicked(&end_process_button, click_x, click_y, is_left_pressed)) {
        if(selected_process != -1) {
            int our_process = 0;
            pcb_t* tasks = process_table_ptr; 

            for (int i = 0; i < MAX_PROCESSES; i++) {

                pcb_t* task = &tasks[i];

                if (task->state == TASK_STATE_EMPTY || task->state == TASK_STATE_ZOMBIE) {
                    continue;
                }

                if(our_process == selected_process) {
                    KillProcess(task->pid);
                }

                our_process++;

            }

            selected_process = -1;

        }
    }

    hover_process = -1;
    if(click_y < 140) return;
    if(click_y >= (140 + (15*drawn_rows))) return;
    hover_process = (click_y - 140) / 15;

    if(is_left_pressed) {
        selected_process = hover_process;
    }

}

void handle_ctrl_alt_del() {

    last_cmos_times = cmos_ticks;

    createWindow(200, 200, 400, 300, "Task Manager", draw_task_manager_content, mouse_draw_task_manager_content);
}