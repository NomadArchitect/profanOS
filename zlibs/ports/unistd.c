#include <syscall.h>
#include <filesys.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <type.h>

int main(void) {
    return 0;
}

int access(const char *a, int b) {
    puts("access is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

unsigned alarm(unsigned a) {
    puts("alarm is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int chdir(const char *a) {
    puts("chdir is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int chown(const char *a, uid_t b, gid_t c) {
    puts("chown is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int close(int a) {
    puts("close is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

size_t confstr(int a, char *b, size_t c) {
    puts("confstr is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

char *crypt(const char *a, const char *b) {
    puts("crypt is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

char *ctermid(char *a) {
    puts("ctermid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int dup(int a) {
    puts("dup is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int dup2(int a, int b) {
    puts("dup2 is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

void  encrypt(char a[64], int b) {
    puts("encrypt is not implemented yet, WHY DO YOU USE IT ?");
}

int execl(const char *a, const char *b, ...) {
    puts("execl is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int execle(const char *a, const char *b, ...) {
    puts("execle is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int execlp(const char *a, const char *b, ...) {
    puts("execlp is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int execv(const char *a, char *const *b) {
    puts("execv is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int execve(const char *a, char *const *b, char *const *c) {
    puts("execve is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int execvp(const char *a, char *const *b) {
    puts("execvp is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

void  _exit(int a) {
    puts("exit is not implemented yet, WHY DO YOU USE IT ?");
}

int fchown(int a, uid_t b, gid_t c) {
    puts("fchown is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int fchdir(int a) {
    puts("fchdir is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int fdatasync(int a) {
    puts("fdatasync is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t fork(void) {
    puts("fork is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

long  fpathconf(int a, int b) {
    puts("fpathconf is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int fsync(int a) {
    puts("fsync is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int ftruncate(int a, off_t b) {
    puts("ftruncate is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

char *getcwd(char *a, size_t b) {
    puts("getcwd is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

gid_t getegid(void) {
    puts("getegid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

uid_t geteuid(void) {
    puts("geteuid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

gid_t getgid(void) {
    puts("getgid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int getgroups(int a, gid_t *b) {
    puts("getgroups is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

long gethostid(void) {
    puts("gethostid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int gethostname(char *a, size_t n) {
    puts("gethostname is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

char *getlogin(void) {
    puts("getlogin is not implemented yet, WHY DO YOU USE IT ?");
    return NULL;
}

int getlogin_r(char *a, size_t n) {
    puts("getlogin_r is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int getopt(int a, char * const *b, const char *c) {
    puts("getopt is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t getpgid(pid_t a) {
    puts("getpgid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t getpgrp(void) {
    puts("getpgrp is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t getpid(void) {
    puts("getpid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t getppid(void) {
    puts("getppid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t getsid(pid_t a) {
    puts("getsid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

uid_t getuid(void) {
    puts("getuid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

char *getwd(char *a) {
    puts("getwd is not implemented yet, WHY DO YOU USE IT ?");
    return NULL;
}

int isatty(int id) {
    // check if the linked file is a fcft

    if (id < 0 || id > 2) {
        return 0;
    }

    sid_t sid;
    if (id == 0)
        sid = fu_path_to_sid(ROOT_SID, "/dev/stdin");
    else if (id == 1)
        sid = fu_path_to_sid(ROOT_SID, "/dev/stdout");
    else
        sid = fu_path_to_sid(ROOT_SID, "/dev/stderr");

    if (IS_NULL_SID(sid)) {
        return 0;
    }

    char **paths;
    int *pids;

    int link_count = fu_link_get_all(sid, &pids, &paths);

    if (link_count < 1) {
        return 1;
    }

    int pid = c_process_get_pid();
    char *path = NULL;

    // check if the pid is in the array
    // else check for the ppids
    while (!path) {
        for (int i = 0; i < link_count; i++) {
            if (pids[i] == pid) {
                path = strdup(paths[i]);
                break;
            }
        }
        if (pid == 0) break;

        pid = c_process_get_ppid(pid);
        if (pid == -1) break;
    }

    // free the arrays
    for (int i = 0; i < link_count; i++) {
        free(paths[i]);
    }
    free(paths);
    free(pids);

    if (!path) {
        return 1;
    }

    sid_t new_sid = fu_path_to_sid(ROOT_SID, path);
    free(path);

    if (IS_NULL_SID(new_sid)) {
        return 1;
    }

    return fu_is_fctf(new_sid);
}

int lchown(const char *a, uid_t b, gid_t c) {
    puts("lchown is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int link(const char *a, const char *b) {
    puts("link is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int lockf(int a, int b, off_t c) {
    puts("lockf is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

off_t lseek(int a, off_t b, int c) {
    puts("lseek is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int nice(int a) {
    puts("nice is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

long pathconf(const char *a, int b) {
    puts("pathconf is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int pause(void) {
    puts("pause is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int pipe(int a[2]) {
    puts("pipe is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

ssize_t pread(int a, void *b, size_t c, off_t d) {
    puts("pread is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

ssize_t pwrite(int a, const void *b, size_t c, off_t d) {
    puts("pwrite is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

ssize_t read(int a, void *b, size_t c) {
    puts("read is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

ssize_t readlink(const char *restrict a, char *restrict b, size_t c) {
    puts("readlink is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int rmdir(const char *a) {
    puts("rmdir is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int setegid(gid_t a) {
    puts("setegid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int seteuid(uid_t a) {
    puts("seteuid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int setgid(gid_t a) {
    puts("setgid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int setpgid(pid_t a, pid_t b) {
    puts("setpgid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t setpgrp(void) {
    puts("setpgrp is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int setregid(gid_t a, gid_t b) {
    puts("setregid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int setreuid(uid_t a, uid_t b) {
    puts("setreuid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t setsid(void) {
    puts("setsid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int setuid(uid_t a) {
    puts("setuid is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

unsigned sleep(unsigned seconds) {
    if (seconds == 0) return 0;
    c_process_sleep(c_process_get_pid(), seconds * 1000);
    return 0;
}

void swab(const void *restrict a, void *restrict n, ssize_t c) {
    puts("swab is not implemented yet, WHY DO YOU USE IT ?");
}

int symlink(const char *a, const char *b) {
    puts("symlink is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

void  sync(void) {
    puts("sync is not implemented yet, WHY DO YOU USE IT ?");
}

long  sysconf(int a) {
    puts("sysconf is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

pid_t tcgetpgrp(int a) {
    puts("tcgetpgrp is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int tcsetpgrp(int a, pid_t b) {
    puts("tcsetpgrp is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int truncate(const char *a, off_t b) {
    puts("truncate is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

char *ttyname(int a) {
    puts("ttyname is not implemented yet, WHY DO YOU USE IT ?");
    return NULL;
}

int ttyname_r(int a, char *b, size_t c) {
    puts("ttyname_r is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

useconds_t ualarm(useconds_t a, useconds_t b) {
    puts("ualarm is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int unlink(const char *a) {
    puts("unlink is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

int usleep(useconds_t usec) {
    if (usec == 0) return 0;
    c_process_sleep(c_process_get_pid(), usec / 1000);
    return 0;
}

pid_t vfork(void) {
    puts("vfork is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}

ssize_t  write(int a, const void *b, size_t c) {
    puts("write is not implemented yet, WHY DO YOU USE IT ?");
    return 0;
}
