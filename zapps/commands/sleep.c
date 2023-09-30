#include <i_time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: sleep <time>\ntime is in milliseconds\n");
        return 1;
    }

    int time = atoi(argv[1]);
    if (time <= 0) {
        printf("'%s' is not a valid time\n", argv[1]);
        return 1;
    }

    ms_sleep(time);

    return 0;
}