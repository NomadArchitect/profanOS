#include <kernel/filesystem.h>
#include <kernel/snowflake.h>
#include <kernel/process.h>
#include <driver/serial.h>
#include <minilib.h>
#include <system.h>
#include <type.h>

/************************************************************
 *     - kernel runtime memory layout and sharing -     e  *
 *                                                      l  *
 *  |             |                   | E               y  *
 *  | kernel      | allocable memory  | N               d  *
 *  | -0x100000   | - 0x200000        | D               r  *
 *  |             |                   |                 e  *
 *        ^     ^     ^     ^     ^                        *
 *        |     |     |     |     '---SCUBA----.           *
 *        |     |     |     |                  |           *
 *  |                                 | : |             |  *
 *  | physical (shared)   [STACK]     | : | virtual     |  *
 *  |                                 | : | -0xC0000000 |  *
 *  |                                 | : |             |  *
************************************************************/

typedef struct {
    int pid;
    int ret;
} direct_return_t;

direct_return_t last_return;

void tasked_program() {
    int pid = process_get_pid();
    int ppid = process_get_ppid(pid);

    comm_struct_t *comm = process_get_comm(pid);
    uint32_t stack_size = comm->stack_size;

    int vsize = comm->vcunt;
    int fsize = fs_get_file_size(comm->path);
    fsize = fsize + (0x1000 - (fsize % 0x1000));

    if (vsize < fsize * 2)
        vsize = fsize * 2;

    // setup private memory
    scuba_create_virtual(process_get_directory(pid), comm->vbase, vsize / 0x1000);

    // load binary
    fs_read_file(comm->path, (char *) comm->vbase);

    // malloc a new stack
    comm->stack = mem_alloc(stack_size, 0, 4);
    mem_set((uint8_t *) comm->stack, 0, stack_size);

    // setup stack
    asm volatile("mov %0, %%esp" :: "r" (comm->stack + stack_size - 0x80));

    // call main
    int (*main)(int, char **) = (int (*)(int, char **)) comm->vbase;
    int ret = main(comm->argc, comm->argv) & 0xFF;

    // free forgeted allocations
    int not_free_mem = mem_get_info(7, pid);

    if (not_free_mem) {
        sys_warning("Memory leak detected");

        kprintf("[auto free] %d alloc will be auto freed (total: %d bytes, pid: %d)\n",
                not_free_mem,
                mem_get_info(8, pid),
                pid
        );

        mem_free_all(pid);
    }

    // free the comm struct
    for (int i = 0; i < comm->argc; i++)
        free((void *) comm->argv[i]);

    free((void *) comm->stack);
    free((void *) comm->path);
    free((void *) comm->argv);
    free(comm);

    // set return value
    last_return.pid = pid;
    last_return.ret = ret;

    // wake up the parent process
    int pstate = process_get_state(ppid);

    if (pstate == PROCESS_TSLPING || pstate == PROCESS_FSLPING)
        process_wakeup(ppid);

    process_kill(pid);
}

int run_binary(char *path, int argc, char **argv, uint32_t vbase, uint32_t vcunt, uint32_t stack_size) {
    // TODO: check if file is executable

    serial_debug("RUNTIME", path);
    int pid = process_create(tasked_program, 2, path);
    if (pid == 1) last_return.pid = -1;

    // duplicate path
    char *npath = (char *) mem_alloc(str_len(path) + 1, 0, 6);
    str_cpy(npath, path);

    // duplicate argv
    int size = sizeof(char *) * (argc + 1);
    char **nargv = (char **) mem_alloc(size, 0, 6);
    mem_set((void *) nargv, 0, size);

    for (int i = 0; i < argc; i++) {
        nargv[i] = (char *) mem_alloc(str_len(argv[i]) + 1, 0, 6);
        str_cpy(nargv[i], argv[i]);
    }

    comm_struct_t *comm = (comm_struct_t *) mem_alloc(sizeof(comm_struct_t), 0, 6);
    comm->argc = argc;
    comm->argv = nargv;
    comm->path = npath;

    comm->vbase = vbase;
    comm->vcunt = vcunt;
    comm->stack_size = stack_size;

    process_set_comm(pid, comm);
    process_handover(pid);

    return (last_return.pid == pid) ? last_return.ret : 0;
}

int run_ifexist_full(char *path, int argc, char **argv, uint32_t vbase, uint32_t vcunt, uint32_t stack) {
    vbase = vbase ? vbase : RUN_BIN_VBASE;
    vcunt = vcunt ? vcunt : RUN_BIN_VCUNT;
    stack = stack ? stack : RUN_BIN_STACK;

    if (fs_does_path_exists(path) && fs_get_sector_type(fs_path_to_id(path)) == 2)
        return run_binary(path, argc, argv, vbase, vcunt, stack);

    sys_error("Program not found");
    return -1;
}
