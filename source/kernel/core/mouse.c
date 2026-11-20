#include "core.h"
#include "calculator.h"
#include "notepad.h"
#include "settings.h"
#include "explorer.h"
#include "terminal.h"
#include "browser.h"

extern int is_start_pressed;
extern int is_start_menu_open;
extern int active_menu_item;

static window_t* dragging_window = NULL;

extern uint8_t os_state;
extern int window_in_focus;

void window_click_handler(void* data) {
    task_event_mouse_click_t* task_event_mouse_click = (task_event_mouse_click_t*)data;
    task_event_mouse_click->window->on_mouse(task_event_mouse_click->window, task_event_mouse_click->mouse_x, task_event_mouse_click->mouse_y, 1);
    kfree(task_event_mouse_click);
    return;
}

void handle_mouse_click(int mx, int my, int is_left_button_pressed) {

    if(os_state == 1) {

        int menu_w = 160;
        int menu_h = 340;
        int menu_x = 5;
        int menu_y = TASKBAR_Y - menu_h;

        if (mx >= 5 && mx <= 75 && my >= (TASKBAR_Y + 5) && my <= (TASKBAR_Y + 35)) {
            if (is_left_button_pressed) {
                is_start_menu_open = 1;
                is_start_pressed = 1;
            } else if (!is_left_button_pressed) {
                is_start_pressed = 0;
            }
        } else {
            if (!is_left_button_pressed) is_start_pressed = 0;
        }

        if (is_start_menu_open) {
            if (mx >= menu_x && mx <= (menu_x + menu_w) &&
                my >= menu_y && my <= (menu_y + menu_h)) {
                
                int relative_y = my - (menu_y + 4);
                int item_index = relative_y / 36;

                if (item_index >= 0 && item_index < 9) {
                    active_menu_item = item_index;

                    if (is_left_button_pressed) {
                        is_start_menu_open = 0;
                        
                        if(active_menu_item == 0) {
                            openFileExplorer();
                        } else if(active_menu_item == 1) {
                            launch_browser();
                        } else if(active_menu_item == 2) {
                            openCalc();
                        } else if (active_menu_item == 3) {
                            openNotepad();
                        } else if (active_menu_item == 4) {
                            openDeviceManager();
                        } else if(active_menu_item == 5) {
                            open_terminal_app();
                        } else if(active_menu_item == 6) {
                            openSettings();
                        } else if (active_menu_item == 7) {
                            openAbout();
                        } else if (active_menu_item == 8) {
                            acpi_os_shutdown();
                        }
                    }
                } else {
                    active_menu_item = -1;
                }
            } else {
                active_menu_item = -1;

                if (is_left_button_pressed && my < TASKBAR_Y) {
                    is_start_menu_open = 0;
                }
            }

        }

        if(my >= TASKBAR_Y) {
            UpdateTaskbar(mx, my, is_left_button_pressed);
        }

        if(my >= TASKBAR_Y) return;

    }
        
    if (is_left_button_pressed) {

        if (dragging_window != NULL) {

            int new_x = mx - dragging_window->drag_offset_x;
            int new_y = my - dragging_window->drag_offset_y;

            if (new_y < 0) new_y = 0;
            if (os_state == 1 && new_y > TASKBAR_Y - HEADER_HEIGHT - 4) {
                new_y = TASKBAR_Y - HEADER_HEIGHT - 4;
            }
            if(os_state == 2 && new_y > SCREEN_HEIGHT - HEADER_HEIGHT - 4) {
                new_y = SCREEN_HEIGHT - HEADER_HEIGHT - 4;
            }

            if (new_x < -dragging_window->width + 40) {
                new_x = -dragging_window->width + 40;
            }
            if (new_x > SCREEN_WIDTH - 40) {
                new_x = SCREEN_WIDTH - 40;
            }

            dragging_window->x = new_x;
            dragging_window->y = new_y;
            return;
        }

        window_t* active_window = window_focus_handle(mx, my);
        
        if (active_window != NULL) {

            int header_min_y = active_window->y + 2;
            int header_max_y = active_window->y + 2 + 22;
            
            int btn_left_x = active_window->x + active_window->width - 22;

            int btn_min_x = active_window->x + active_window->width - 22;
            int btn_max_x = btn_min_x + 16;
            int btn_min_y = active_window->y + 5;
            int btn_max_y = btn_min_y + 16;

            if (mx >= btn_min_x && mx < btn_max_x && my >= btn_min_y && my < btn_max_y) { // Close button
                active_window->is_free = 0;
                dragging_window = NULL;

                return;
            }

            if (mx >= btn_min_x-20 && mx < btn_max_x-20 && my >= btn_min_y && my < btn_max_y) { // Collapse button
                active_window->is_visible = 0;
                dragging_window = NULL;

                return;
            }

            if (my >= header_min_y && my < header_max_y && mx < btn_left_x) {
                active_window->is_dragging = 1;
                
                active_window->drag_offset_x = mx - active_window->x;
                active_window->drag_offset_y = my - active_window->y;
                
                dragging_window = active_window;

            }

            int body_x = active_window->x + 2;
            int body_y = active_window->y + 2 + 22;
            int body_w = active_window->width - 4;
            int body_h = active_window->height - 4 - 22;

            if (mx >= body_x && mx < (body_x + body_w) && my >= body_y && my < (body_y + body_h)) {
                if (active_window->on_mouse != NULL) {
                    int local_x = mx - body_x;
                    int local_y = my - body_y;

                    task_event_mouse_click_t* ev = kmalloc(sizeof(task_event_mouse_click_t));
                    ev->window = active_window;
                    ev->mouse_x = local_x;
                    ev->mouse_y = local_y;
                    
                    push_deferred_task(window_click_handler, (void*)ev);
                }
            }

        }
    }
    else {

        window_t *currentWindow = get_window_focus(mx, my);

        if(currentWindow != NULL && currentWindow->on_mouse != NULL) {

            int body_x = currentWindow->x + 2;
            int body_y = currentWindow->y + 2 + 22;
            int body_w = currentWindow->width - 4;
            int body_h = currentWindow->height - 4 - 22;

            int local_x = mx - body_x;
            int local_y = my - body_y;

            if (mx >= body_x && mx < (body_x + body_w) && my >= body_y && my < (body_y + body_h)) {
                currentWindow->on_mouse(currentWindow, local_x, local_y, is_left_button_pressed);
            }
        }

        if (dragging_window != NULL) {
            dragging_window->is_dragging = 0;
            dragging_window = NULL;
        }
    }
}