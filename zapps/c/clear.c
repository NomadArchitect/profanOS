/*****************************************************************************\
|   === clear.c : 2024 ===                                                    |
|                                                                             |
|    Unix command implementation - clears the terminal screen      .pi0iq.    |
|                                                                 d"  . `'b   |
|    This file is part of profanOS and is released under          q. /|\  "   |
|    the terms of the GNU General Public License                   `// \\     |
|                                                                  //   \\    |
|   === elydre : https://github.com/elydre/profanOS ===         #######  \\   |
\*****************************************************************************/

#include <profan/syscall.h>
#include <stdio.h>

#define ANSI_CLEAR "\e[2J\e[H"

int ansiclear(void) {
    printf(ANSI_CLEAR);
    fflush(stdout);
    return 0;
}

int hardclear(void) {
    syscall_kprint(ANSI_CLEAR);
    ansiclear();
    return 0;
}

int showhelp(void) {
    puts(
        "Usage: clear [option]\n"
        "  -a   use ANSI escape codes to clear the screen\n"
        "  -h   show this help message\n"
        "  -x   set all pixels to black and clear the screen"
    );
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) return ansiclear();
    if (argc > 2) return showhelp();
    if (argv[1][0] != '-') return showhelp();
    switch (argv[1][1]) {
        case 'a':
            return ansiclear();
        case 'h':
            return showhelp();
        case 'x':
            return hardclear();
        default:
            fprintf(stderr, "clear: invalid option -- '%c'\n", argv[1][1]);
            fputs("Try 'clear -h' for more information.\n", stderr);
            return 1;
    }
    return 0;
}
