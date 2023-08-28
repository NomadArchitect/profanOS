#include <kernel/snowflake.h>
#include <kernel/process.h>
#include <drivers/diskiso.h>
#include <drivers/serial.h>
#include <minilib.h>
#include <system.h>

#include <gui/gnrtx.h>


/* * * * * * * * * * * * * * * * * * * *
 *        _             _              *
 *   ___ | | _   _   __| | _ __   ___  *
 *  / _ \| || | | | / _` || '__| / _ | *
 * |  __/| || |_| || (_| || |   |  __/ *
 *  \___||_| \__, | \__,_||_|    \___| *
 *           |___/                     *
 *                                     *
 *    rip b3  (sep 2022 - dec 2022)    *
 * * * * * * * * * * * * * * * * * * * */

allocated_part_t *MEM_PARTS;
uint32_t alloc_count;
uint32_t free_count;
uint32_t phys_size;
int part_size;

uint32_t mem_alloc(uint32_t size, uint32_t allign, int state);
int mem_free_addr(uint32_t addr);

uint32_t mem_get_phys_size() {
    uint32_t *addr_min = (uint32_t *) 0x200000;
    uint32_t *addr_max = (uint32_t *) 0x40000000;
    uint32_t *addr_test;
    uint32_t old_value;
    while (addr_max - addr_min > 1) {
        addr_test = addr_min + (addr_max - addr_min) / 2;
        old_value = *addr_test;
        *addr_test = 0x1234;
        if (*addr_test == 0x1234) {
            addr_min = addr_test;
            *addr_test = old_value;
        }
        else addr_max = addr_test;
    }
    return (uint32_t) addr_max;
}

int instance_count;

int mem_init() {
    alloc_count = 0;
    free_count = 0;

    instance_count = 0;

    phys_size = mem_get_phys_size();

    part_size = PARTS_COUNT;

    MEM_PARTS = (allocated_part_t *) (MEM_BASE_ADDR + 1);
    for (int i = 0; i < part_size; i++) {
        MEM_PARTS[i].state = 0;
    }

    MEM_PARTS[0].state = 2;
    MEM_PARTS[0].size = 1;
    MEM_PARTS[0].addr = MEM_BASE_ADDR;
    MEM_PARTS[0].next = 1;
    MEM_PARTS[0].task_id = 0;

    if (mem_alloc(sizeof(allocated_part_t) * part_size, 0, 3) != (uint32_t) MEM_PARTS) {
        sys_fatal("snowflake address is illogical");
    }

    // allocate the diskiso module if needed
    if (!diskiso_get_size()) return 0;

    uint32_t start = mem_alloc(diskiso_get_size(), 0, 1);

    if (start != diskiso_get_start()) {
        sys_error("diskiso address is illogical");
        mem_free_addr(start);
    }

    return 0;
}

int mm_get_unused_index(int not_index) {
    for (int i = 0; i < part_size; i++) {
        if (MEM_PARTS[i].state == 0 && i != not_index) return i;
    }

    sys_fatal("no more memory, dynamizing do not work");
    return -1;
}

void del_occurence(int index) {
    int i = 0;
    for (int count = 0; MEM_PARTS[i].state; count++) {
        if (MEM_PARTS[i].next == index) {
            MEM_PARTS[i].next = mm_get_unused_index(index);
            return;
        }
        if (count > part_size) {
            sys_fatal("recursive linked list detected in del_occurence");
        }
        i = MEM_PARTS[i].next;
    }
}

void dynamize_mem() {
    int sum = 0;
    for (int i = 0; i < part_size; i++) {
        sum += !MEM_PARTS[i].state;
        if (sum > 4) return;
    }

    process_disable_sheduler();

    uint32_t new_add = mem_alloc(sizeof(allocated_part_t) * (part_size + GROW_SIZE), 0, 3);

    if (new_add == 0) {
        sys_fatal("memory dynamizing failed");
        return;
    }

    // fill the new part with 0 and copy the old part
    mem_set((uint8_t *) new_add, 0, sizeof(allocated_part_t) * (part_size + GROW_SIZE));
    mem_copy((uint8_t *) new_add, (uint8_t *) MEM_PARTS, sizeof(allocated_part_t) * part_size);

    uint32_t old_add = (uint32_t) MEM_PARTS;
    MEM_PARTS = (allocated_part_t *) new_add;
    part_size += GROW_SIZE;
    mem_free_addr(old_add);

    serial_debug("SNOWFLAKE", "memory successfully dynamized");
}

uint32_t mem_alloc(uint32_t size, uint32_t allign, int state) {
    if (!(size && state)) return 0;

    if (state != 3) {
        dynamize_mem();
        process_disable_sheduler();
        instance_count++;
    }

    if (instance_count > 1) {
        sys_fatal("instance count is too high in mem_alloc");
    }

    // traversing the list of allocated parts
    int index, old_index, exit_mode;
    uint32_t last_addr, gap;
    index = 0;

    last_addr = MEM_BASE_ADDR;
    for (int i = 0; i <= part_size; i++) {
        if (i == part_size) {
            sys_fatal("recursive linked list detected in mem_alloc");
        }

        // calculate the gap
        gap = allign ? (allign - (last_addr % allign)) % allign : 0;

        // if the part is free
        if (MEM_PARTS[index].state == 0) {
            exit_mode = 1 + (last_addr + size + gap > phys_size);
            break;
        }

        if (MEM_PARTS[index].addr - last_addr >= size + gap) {
            // we can allocate the part here
            exit_mode = 0;
            break;
        }

        last_addr = MEM_PARTS[index].addr + MEM_PARTS[index].size;

        old_index = index;
        index = MEM_PARTS[index].next;
    }

    if (exit_mode == 2) {
        sys_error("Cannot alloc: out of physical memory");
        if (state != 3) instance_count--;
        process_enable_sheduler();
        return 0;
    }

    if (exit_mode) {
        MEM_PARTS[index].addr = last_addr + gap;
        MEM_PARTS[index].size = size;
        MEM_PARTS[index].task_id = (state == 1) ? process_get_pid(): 0;
        MEM_PARTS[index].state = state;
        MEM_PARTS[index].next = mm_get_unused_index(-1);
    }

    else {
        int new_index = mm_get_unused_index(-1);
        del_occurence(new_index);

        MEM_PARTS[new_index].addr = last_addr + gap;
        MEM_PARTS[new_index].size = size;
        MEM_PARTS[new_index].task_id = (state == 1) ? process_get_pid(): 0;
        MEM_PARTS[new_index].state = state;

        MEM_PARTS[old_index].next = new_index;
        MEM_PARTS[new_index].next = index;
    }

    alloc_count++;
    if (state != 3) instance_count--;
    process_enable_sheduler();

    return last_addr + gap;
}

int mem_free_addr(uint32_t addr) {
    process_disable_sheduler();
    instance_count++;

    if (instance_count > 1) {
        sys_fatal("instance count is too high in mem_free_addr");
    }

    int index = 0;
    int last_index = -1;
    while (MEM_PARTS[index].state) {
        if (!(MEM_PARTS[index].addr == addr && last_index != -1)) {
            last_index = index;
            index = MEM_PARTS[index].next;
            continue;
        }

        if (MEM_PARTS[index].state == 2) {
            sys_error("cannot free first block");
            instance_count--;
            process_enable_sheduler();
            return 0; // error
        }

        free_count++;
        MEM_PARTS[last_index].next = MEM_PARTS[index].next;
        MEM_PARTS[index].state = 0;
        instance_count--;
        process_enable_sheduler();

        return 1; // success
    }

    sys_warning("block not found");
    instance_count--;
    process_enable_sheduler();
    return 0; // error
}

uint32_t mem_get_alloc_size(uint32_t addr) {
    uint32_t index = 0;
    while (MEM_PARTS[index].state) {
        if (MEM_PARTS[index].addr == addr) {
            return MEM_PARTS[index].size;
        }
        index = MEM_PARTS[index].next;
    }
    return 0; // block not found
}

int mem_free_all(int task_id) {
    uint32_t index = 0;
    int count = 0;
    while (MEM_PARTS[index].state) {
        if (MEM_PARTS[index].task_id == task_id) {
            count += mem_free_addr(MEM_PARTS[index].addr);
        }
        index = MEM_PARTS[index].next;
    }
    return count;
}

int mem_get_info(char get_mode, int get_arg) {
    // check the header for informations about the get_mode

    if (get_mode == 0) return phys_size;
    if (get_mode == 1) return MEM_BASE_ADDR;
    if (get_mode == 2) return sizeof(allocated_part_t) * part_size;
    if (get_mode == 3) return (int) MEM_PARTS;
    if (get_mode == 4) return alloc_count;
    if (get_mode == 5) return free_count;

    int index = 0;

    int info[7];
    for (int i = 0; i < 7; i++) info[i] = 0;

    while (MEM_PARTS[index].state) {
        info[0] += MEM_PARTS[index].size;
        if (MEM_PARTS[index].task_id == get_arg && MEM_PARTS[index].state > 0) {
            info[1]++;
            info[2] += MEM_PARTS[index].size;
        }
        if (MEM_PARTS[index].state == 4) { // bin_run
            info[3]++;
            info[4] += MEM_PARTS[index].size;
        }
        if (MEM_PARTS[index].state == 5) { // libs
            info[5]++;
            info[6] += MEM_PARTS[index].size;
        }
        index = MEM_PARTS[index].next;
    }

    if (get_mode > 5 && get_mode < 13) return info[get_mode - 6];
    return -1;
}