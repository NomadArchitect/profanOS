#include <syscall.h>
#include <filesys.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vitrail.h>

#define SCROLL_LINES 8
#define PIXEL_OFFSET 4
#define malloc_as_kernel(size) ((void *) c_mem_alloc(size, 0, 6))

#define WIDTH 500
#define HEIGHT 748

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t charcount;
    uint32_t charsize;

    uint8_t *data;
} font_data_t;

typedef struct {
    char content;
    char color;
} screen_char_t;

typedef struct {
    int cursor_x;
    int cursor_y;
    int scroll_offset;

    int max_lines;
    int max_cols;

    int saved_cursor_x;
    int saved_cursor_y;

    uint8_t cursor_is_hidden;
    char color;

    window_t *window;

    screen_char_t *screen_buffer;

    font_data_t *font;
} panda_global_t;

panda_global_t *g_panda;

void init_panda();

int main(void) {
    init_panda();
    return 0;
}

font_data_t *load_psf_font(char *file) {
    sid_t sid = fu_path_to_sid(ROOT_SID, file);
    if (IS_NULL_SID(sid)) return NULL;

    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t charcount;
    uint32_t charsize;
    uint32_t height;
    uint32_t width;

    fu_file_read(sid, &magic, 0, 4);
    fu_file_read(sid, &version, 4, 4);
    fu_file_read(sid, &headersize, 8, 4);
    fu_file_read(sid, &charcount, 16, 4);
    fu_file_read(sid, &charsize, 20, 4);
    fu_file_read(sid, &height, 24, 4);
    fu_file_read(sid, &width, 28, 4);

    if (magic != 0x864ab572 || version != 0)
        return NULL;

    uint8_t *font = malloc_as_kernel(charcount * charsize);
    fu_file_read(sid, font, headersize, charcount * charsize);

    font_data_t *psf = malloc_as_kernel(sizeof(font_data_t));
    psf->width = width;
    psf->height = height;
    psf->charcount = charcount;
    psf->charsize = charsize;
    psf->data = font;

    return psf;
}

void free_font(font_data_t *pff) {
    free(pff->data);
    free(pff);
}

uint32_t compute_color(uint8_t color) {
    uint32_t rgb[] = {
        0x99000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA,
        0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
        0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF,
        0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF
    };
    if (color > 0xF) return 0xFFFFFFFF;
    return rgb[(int) color];
}

char compute_ansi_color(char ansi_nb, int part, char old_color) {
    char fg = old_color & 0xF;
    char bg = (old_color >> 4) & 0xF;

    switch (ansi_nb) {
        case '0': ansi_nb = 0; break;
        case '1': ansi_nb = 4; break;
        case '2': ansi_nb = 2; break;
        case '3': ansi_nb = 6; break;
        case '4': ansi_nb = 1; break;
        case '5': ansi_nb = 5; break;
        case '6': ansi_nb = 3; break;
        default:  ansi_nb = 7; break;
    }

    if (part == 0) {
        fg = ansi_nb;
    } else if (part == 1) {
        fg = ansi_nb + 8;
    } else if (part == 2) {
        bg = ansi_nb;
    }

    return (bg << 4) | fg;
}

void print_char(uint32_t xo, uint32_t yo, uint8_t c, uint8_t color_code) {
    uint32_t bg_color = compute_color((color_code >> 4) & 0xF);
    uint32_t fg_color = compute_color(color_code & 0xF);

    uint8_t *char_data = g_panda->font->data + (c * g_panda->font->charsize);

    uint32_t x = 0;
    uint32_t y = 0;
    for (uint32_t i = 0; i < g_panda->font->charsize; i++) {
        if (x >= g_panda->font->width) {
            x = 0;
            y++;
        }
        for (int j = 7; j >= 0; j--) {
            windows_set_pixel_alpha(g_panda->window, xo + x + PIXEL_OFFSET, yo + y + PIXEL_OFFSET, char_data[i] & (1 << j) ? fg_color : bg_color);
            if (x >= g_panda->font->width) break;
            x++;
        }
    }
}

void panda_clear_screen(void) {
    if (!g_panda) return;
    for (int i = 0; i < g_panda->max_lines; i++) {
        for (int j = 0; j < g_panda->max_cols; j++) {
            if (g_panda->screen_buffer[i * g_panda->max_cols + j].content == ' ' &&
                g_panda->screen_buffer[i * g_panda->max_cols + j].color == 0xF
            ) continue;
            g_panda->screen_buffer[i * g_panda->max_cols + j].content = ' ';
            g_panda->screen_buffer[i * g_panda->max_cols + j].color = 0xF;
            print_char(j * g_panda->font->width, i * g_panda->font->height, ' ', 0xF);
        }
    }
    g_panda->cursor_x = 0;
    g_panda->cursor_y = 0;
    g_panda->scroll_offset = 0;
    g_panda->saved_cursor_x = 0;
    g_panda->saved_cursor_y = 0;
}

int compute_ansi_escape(char *str, panda_global_t *g_panda) {
    char *start = str;

    if (str[1] == '[') str += 2;
    else return 1;

    // cursor save and restore
    if (str[0] == 's') {
        g_panda->saved_cursor_x = g_panda->cursor_x;
        g_panda->saved_cursor_y = g_panda->cursor_y;
    } else if (str[0] == 'u') {
        if (g_panda->saved_cursor_y < g_panda->scroll_offset) {
            g_panda->saved_cursor_y = g_panda->scroll_offset;
        } else {
            g_panda->cursor_y = g_panda->saved_cursor_y;
        }
        g_panda->cursor_x = g_panda->saved_cursor_x;
    } else if (str[0] == 'K') {
        for (int i = g_panda->cursor_x; i < g_panda->max_cols; i++) {
            g_panda->screen_buffer[(g_panda->cursor_y - g_panda->scroll_offset) * g_panda->max_cols + i].content = ' ';
            g_panda->screen_buffer[(g_panda->cursor_y - g_panda->scroll_offset) * g_panda->max_cols + i].color = 0x0F;
            print_char(i * g_panda->font->width, (g_panda->cursor_y - g_panda->scroll_offset) * g_panda->font->height, ' ', 0x0F);
        }
    }

    // font color
    if (str[0] == '3' && str[2] == 'm') {
        g_panda->color = compute_ansi_color(str[1], 0, g_panda->color);
        return 4;
    }

    // highlight font color
    if (str[0] == '9' && str[2] == 'm') {
        g_panda->color = compute_ansi_color(str[1], 1, g_panda->color);
        return 4;
    }

    // background color
    if (str[0] == '4' && str[2] == 'm') {
        g_panda->color = compute_ansi_color(str[1], 2, g_panda->color);
        return 4;
    }

    // reset color
    if (str[0] == '0' && str[1] == 'm') {
        g_panda->color = 0xF;
        return 3;
    }

    // cursor hide and show
    if (strncmp(str, "?25", 3) == 0) {
        if (str[3] == 'l') {
            g_panda->cursor_is_hidden = 0;
        } else if (str[3] == 'h') {
            g_panda->cursor_is_hidden = 1;
        }
        return 5;
    }

    // clear screen
    if (str[0] == '2' && str[1] == 'J') {
        panda_clear_screen();
        return 3;
    }

    // set top left
    if (str[0] == 'H') {
        g_panda->cursor_x = 0;
        g_panda->cursor_y = 0;
        g_panda->scroll_offset = 0;
        return 2;
    }

    // number
    char *tmp = str;
    while (*tmp >= '0' && *tmp <= '9') tmp++;

    // cursor up
    if (tmp[0] == 'A') {
        int n = atoi(str);
        g_panda->cursor_y -= n;
    }

    // cursor down
    if (tmp[0] == 'B') {
        int n = atoi(str);
        g_panda->cursor_y += n;
    }

    // cursor forward
    if (tmp[0] == 'C') {
        int n = atoi(str);
        g_panda->cursor_x += n;
        if (g_panda->cursor_x >= g_panda->max_cols) {
            g_panda->cursor_x -= g_panda->max_cols;
            g_panda->cursor_y++;
        }
    }

    // cursor backward
    if (tmp[0] == 'D') {
        int n = atoi(str);
        g_panda->cursor_x -= n;
        if (g_panda->cursor_x < 0) {
            g_panda->cursor_x += g_panda->max_cols;
            g_panda->cursor_y--;
        }
    }

    return tmp - start;
}

void panda_scroll(uint32_t line_count) {
    // fill the rest of the line with spaces
    int offset;
    while (g_panda->cursor_x < g_panda->max_cols) {
        offset = (g_panda->cursor_y - g_panda->scroll_offset) * g_panda->max_cols + g_panda->cursor_x;
        if (g_panda->screen_buffer[offset].content == '\0') {
            g_panda->screen_buffer[offset].content = ' ';
        }
        g_panda->cursor_x++;
    }

    g_panda->cursor_x = 0;
    g_panda->cursor_y++;

    if (g_panda->cursor_y - g_panda->scroll_offset < g_panda->max_lines) {
        return;
    }
    g_panda->scroll_offset += line_count;

    // scroll the display and print it
    int new_offset;
    for (uint32_t i = 0; i < g_panda->max_lines - line_count; i++) {
        for (int j = 0; j < g_panda->max_cols; j++) {
            new_offset = i * g_panda->max_cols + j;
            offset = new_offset + g_panda->max_cols * line_count;
            if (g_panda->screen_buffer[new_offset].content == g_panda->screen_buffer[offset].content &&
                g_panda->screen_buffer[new_offset].color == g_panda->screen_buffer[offset].color
            ) continue;
            g_panda->screen_buffer[new_offset].content = g_panda->screen_buffer[offset].content;
            g_panda->screen_buffer[new_offset].color = g_panda->screen_buffer[offset].color;
            print_char(j * g_panda->font->width, i * g_panda->font->height, g_panda->screen_buffer[new_offset].content, g_panda->screen_buffer[new_offset].color);
        }
    }

    // clear the last line
    for (uint32_t i = 0; i < line_count; i++) {
        for (int j = 0; j < g_panda->max_cols; j++) {
            new_offset = (g_panda->max_lines - 1 - i) * g_panda->max_cols + j;
            if (g_panda->screen_buffer[new_offset].content == ' ' && g_panda->screen_buffer[new_offset].color == 0xF) continue;
            g_panda->screen_buffer[new_offset].content = ' ';
            g_panda->screen_buffer[new_offset].color = 0xF;
            print_char(j * g_panda->font->width, (g_panda->max_lines - 1 - i) * g_panda->font->height, g_panda->screen_buffer[new_offset].content, g_panda->screen_buffer[new_offset].color);
        }
    }
}

void draw_cursor(int errase) {
    uint32_t offset;
    if (!errase) {
        for (uint32_t i = 0; i < g_panda->font->height; i++) {
            windows_set_pixel_alpha(g_panda->window, g_panda->cursor_x * g_panda->font->width + 1 + PIXEL_OFFSET, (g_panda->cursor_y - g_panda->scroll_offset) * g_panda->font->height + i + PIXEL_OFFSET, 0xFFFFFFFF);
        }
    } else {
        offset = (g_panda->cursor_y - g_panda->scroll_offset) * g_panda->max_cols + g_panda->cursor_x;
        print_char(g_panda->cursor_x * g_panda->font->width,
                  (g_panda->cursor_y - g_panda->scroll_offset) * g_panda->font->height,
                   g_panda->screen_buffer[offset].content,
                   g_panda->screen_buffer[offset].color
        );
    }
}

void panda_print_string(char *string, int len) {
    if (!g_panda) return;
    int tmp, y;
    for (int i = 0; (len < 0) ? (string[i]) : (i < len); i++) {
        if (!g_panda->cursor_is_hidden)
            draw_cursor(1);
        if (string[i] == '\n')
            panda_scroll(SCROLL_LINES);
        else if (string[i] == '\r')
            g_panda->cursor_x = 0;
        else if (string[i] == '\t') {
            tmp = g_panda->cursor_x + 4 - (g_panda->cursor_x % 4);
            for (; g_panda->cursor_x < tmp; g_panda->cursor_x++)
                print_char(g_panda->cursor_x * g_panda->font->width,
                    (g_panda->cursor_y - g_panda->scroll_offset) * g_panda->font->height,
                    ' ',
                    g_panda->color
                );
        } else if (string[i] == '\e')
            i += compute_ansi_escape(string + i, g_panda);
        else {
            y = g_panda->cursor_y - g_panda->scroll_offset;
            g_panda->screen_buffer[y * g_panda->max_cols + g_panda->cursor_x].content = string[i];
            g_panda->screen_buffer[y * g_panda->max_cols + g_panda->cursor_x].color = g_panda->color;
            print_char(g_panda->cursor_x * g_panda->font->width,
                y * g_panda->font->height,
                string[i],
                g_panda->color
            );
            g_panda->cursor_x++;
        }
        if (g_panda->cursor_x >= g_panda->max_cols)
            panda_scroll(SCROLL_LINES);
        if (g_panda->cursor_y - g_panda->scroll_offset >= g_panda->max_lines)
            panda_scroll(SCROLL_LINES);
        if (!g_panda->cursor_is_hidden)
            draw_cursor(0);
    }
}

#define offset_to_cursor_y(offset, max_cols) ((offset) / (2 * (max_cols)))

void panda_set_start(int kernel_cursor) {
    if (!g_panda) return;
    uint32_t kmax_cols = WIDTH / 8;

    g_panda->cursor_x = 0;
    g_panda->cursor_y = ((offset_to_cursor_y(kernel_cursor, kmax_cols) + 1) * 16) / g_panda->font->height;
    g_panda->scroll_offset = 0;
}

void panda_get_cursor(uint32_t *x, uint32_t *y) {
    if (!g_panda) {
        *x = 0;
        *y = 0;
    } else {
        *x = g_panda->cursor_x;
        *y = g_panda->cursor_y;
    }
}

void panda_get_size(uint32_t *x, uint32_t *y) {
    if (!g_panda) {
        *x = 0;
        *y = 0;
    } else {
        *x = g_panda->max_cols;
        *y = g_panda->max_lines;
    }
}

void panda_full_clean(uint32_t color) {
    if (!g_panda) return;
    for (int i = 0; i < g_panda->max_lines; i++) {
        for (int j = 0; j < g_panda->max_cols; j++) {
            g_panda->screen_buffer[i * g_panda->max_cols + j].content = ' ';
            g_panda->screen_buffer[i * g_panda->max_cols + j].color = 0xF;
        }
    }
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            g_panda->window->pixels[i * WIDTH + j] = color;
        }
    }
    window_refresh(g_panda->window);

    g_panda->cursor_x = 0;
    g_panda->cursor_y = 0;
    g_panda->scroll_offset = 0;

    g_panda->saved_cursor_x = 0;
    g_panda->saved_cursor_y = 0;
}

int panda_change_font(char *file) {
    if (!g_panda) return 1;
    font_data_t *font = load_psf_font(file);
    if (font == NULL) return 1;

    free_font(g_panda->font);
    g_panda->font = font;
    g_panda->max_lines = HEIGHT / g_panda->font->height;
    g_panda->max_cols = WIDTH / g_panda->font->width;
    panda_full_clean(compute_color(0));
    return 0;
}

void init_panda(void) {
    if (!c_vesa_does_enable()) {
        printf("[panda] VESA is not enabled\n");
        g_panda = NULL;
        return;
    }

    g_panda = malloc_as_kernel(sizeof(panda_global_t));
    g_panda->font = load_psf_font("/zada/fonts/lat38-bold18.psf");

    if (g_panda->font == NULL) {
        printf("\n Failed to load font\n");
    }

    g_panda->cursor_x = 0;
    g_panda->cursor_y = 0;

    g_panda->saved_cursor_x = 0;
    g_panda->saved_cursor_y = 0;

    g_panda->scroll_offset = 0;

    g_panda->cursor_is_hidden = 1;
    g_panda->color = 0x0F;

    g_panda->max_lines = (HEIGHT - 2 * PIXEL_OFFSET - 1) / g_panda->font->height;
    g_panda->max_cols = (WIDTH - 2 * PIXEL_OFFSET - 1) / g_panda->font->width;

    g_panda->window = window_create(10, 10, WIDTH, HEIGHT);

    int buffer_size = g_panda->max_lines * g_panda->max_cols * sizeof(screen_char_t);
    g_panda->screen_buffer = malloc_as_kernel(buffer_size);
    memset(g_panda->screen_buffer, 0, buffer_size);
    panda_full_clean(compute_color(0));
}
