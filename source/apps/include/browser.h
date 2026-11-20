#include "core.h"

void launch_browser();

#define CHAR_W 5
#define CHAR_H 15
#define LINE_HEIGHT (CHAR_H + 4)
#define TOOLBAR_H 40

typedef enum {
    ITEM_TEXT,
    ITEM_LINK,
    ITEM_HEADER,
    ITEM_BOLD,
    ITEM_CODE,
    ITEM_NEWLINE,
    ITEM_PARAGRAPH,
    ITEM_LI,
    ITEM_HR,
    ITEM_IMG
} ItemType;

typedef struct {
    ItemType type;
    char text[128];
    char href[256];
    int x, y, w, h;
    int indent;
} RenderItem;

#define MAX_RENDER_ITEMS 8192

typedef struct {
    char url[256];
    int url_len;
    int url_focused;
    int scroll_y;
    
    RenderItem items[MAX_RENDER_ITEMS];
    int item_count;
    
    button_t btn_back;
    button_t btn_go;
    int last_body_w;

    char page_title[128];

} BrowserState;