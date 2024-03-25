#include <syscall.h>
#include <filesys.h>
#include <stdarg.h>
#include <type.h>

#define malloc(size) ((void *) c_mem_alloc((size), 0, 1))

int main(void) {
    return 0;
}

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);


void *calloc_func(uint32_t nmemb, uint32_t lsize, int as_kernel) {
    uint32_t size = lsize * nmemb;
    int addr = c_mem_alloc(size, 0, as_kernel ? 6 : 1);
    if (addr == 0)
        return NULL;
    memset((uint8_t *) addr, 0, size);
    return (void *) addr;
}

void free(void *mem) {
    if (mem == NULL)
        return;
    c_mem_free_addr((int) mem);
}

void *realloc_func(void *mem, uint32_t new_size, int as_kernel) {
    if (mem == NULL)
        return (void *) c_mem_alloc(new_size, 0, as_kernel ? 6 : 1);

    uint32_t old_size = c_mem_get_alloc_size((uint32_t) mem);
    uint32_t new_addr = c_mem_alloc(new_size, 0, as_kernel ? 6 : 1);
    if (new_addr == 0)
        return NULL;

    memcpy((uint8_t *) new_addr, (uint8_t *) mem, old_size < new_size ? old_size : new_size);
    free(mem);
    return (void *) new_addr;
}

void *memcpy(void *dest, const void *src, size_t n) {
    register uint8_t *d = (uint8_t *) dest;
    register const uint8_t *s = (const uint8_t *) src;

    while (n--) {
        *d++ = *s++;
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    register const uint8_t *r1 = (const uint8_t *) s1;
    register const uint8_t *r2 = (const uint8_t *) s2;

    int r = 0;

    while (n-- && ((r = ((int)(*r1++)) - *r2++) == 0));
    return r;
}

void *memset(void *s, int c, size_t n) {
    register uint8_t *p = (uint8_t *) s;
    register uint8_t v = (uint8_t) c;

    while (n--) {
        *p++ = v;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    register uint8_t *d = (uint8_t *) dest;
    register const uint8_t *s = (const uint8_t *) src;

    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }

    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char *) s1 - *(unsigned char *) s2;
}

int strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

char *strdup(const char *s) {
    size_t len = strlen(s);
    char *d = malloc(len + 1);
    if (d == NULL)
        return NULL;
    memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- && *s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return n ? *(unsigned char *) s1 - *(unsigned char *) s2 : 0;
} 

char *strcat(char *dest, const char *src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

void fd_putchar(int fd, char c) {
    fm_write(fd, &c, 1);
}

void fd_putstr(int fd, const char *str) {
    fm_write(fd, (void *) str, strlen(str));
}

void fd_putint(int fd, int n) {
    if (n < 0) {
        fd_putchar(fd, '-');
        n = -n;
    }
    if (n / 10) {
        fd_putint(fd, n / 10);
    }
    fd_putchar(fd, n % 10 + '0');
}

void fd_printf(int fd, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_end(args);

    for (int i = 0; fmt[i] != '\0';) {
        if (fmt[i] == '%') {
            i++;
            if (fmt[i] == 's') {
                char *tmp = va_arg(args, char *);
                for (int j = 0; tmp[j] != '\0'; j++) {
                    fd_putchar(fd, tmp[j]);
                }
            } else if (fmt[i] == 'c') {
                fd_putchar(fd, va_arg(args, int));
            } else if (fmt[i] == 'd') {
                fd_putint(fd, va_arg(args, int));
            } else {
                fd_putchar(fd, '%');
            }
        } else {
            fd_putchar(fd, fmt[i]);
        }
        i++;
    }
}

int atoi(const char *str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; i++) {
        res = res * 10 + str[i] - '0';
    }

    return sign * res;
}
