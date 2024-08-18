/*****************************************************************************\
|   === ctr_init.c : 2024 ===                                                 |
|                                                                             |
|    Kernel filesystem control functions                           .pi0iq.    |
|                                                                 d"  . `'b   |
|    This file is part of profanOS and is released under          q. /|\  "   |
|    the terms of the GNU General Public License                   `// \\     |
|                                                                  //   \\    |
|   === elydre : https://github.com/elydre/profanOS ===         #######  \\   |
\*****************************************************************************/

#include <kernel/butterfly.h>
#include <minilib.h>
#include <system.h>


int fs_cnt_init_sector(vdisk_t *vdisk, sid_t sid, int type) {
    uint8_t *data;

    // check if sector unused
    if (vdisk_is_sector_used(vdisk, sid)) {
        return 1;
    }

    vdisk_note_sector_used(vdisk, sid);

    data = calloc(FS_SECTOR_SIZE);

    // add sector identifier
    data[0] = ST_CONT;
    data[1] = type;

    for (int i = 2; i < FS_SECTOR_SIZE; i++) {
        data[i] = 0;
    }

    vdisk_write_sector(vdisk, sid, data);

    free(data);

    return 0;
}

#define fs_cnt_init_loca_in_sector(vdisk, sid) fs_cnt_init_sector(vdisk, sid, SF_LOCA)
#define fs_cnt_init_core_in_sector(vdisk, sid) fs_cnt_init_sector(vdisk, sid, SF_CORE)

sid_t fs_cnt_init(filesys_t *filesys, uint32_t device_id, char *meta) {
    kprintf_serial("fs_cnt_init\n");
    vdisk_t *vdisk;
    sid_t main_sid;
    sid_t loca_sid;

    uint8_t *data;
    int ret_sect;

    main_sid.device = device_id;
    kprintf_serial("device_id: %d\n", device_id);
    kprintf_serial("device_id: %d\n", main_sid.device);

    vdisk = fs_get_vdisk(filesys, main_sid.device);
    kprintf_serial("fs_cnt_ini 2 455422 t\n");
    if (vdisk == NULL) {
        sys_warning("[cnt_init] vdisk not found");
        return NULL_SID;
    }


    // get unused sector for header
    ret_sect = vdisk_get_unused_sector(vdisk);
    if (ret_sect == -1) {
        sys_error("[cnt_init] No more free sectors");
        return NULL_SID;
    }
    main_sid.sector = (uint32_t) ret_sect;
    vdisk_note_sector_used(vdisk, main_sid);

    // get unused sector for locator
    loca_sid.device = main_sid.device;
    ret_sect = vdisk_get_unused_sector(vdisk);
    if (ret_sect == -1) {
        sys_error("[cnt_init] No more free sectors");
        vdisk_note_sector_unused(vdisk, main_sid);
        return NULL_SID;
    }
    loca_sid.sector = (uint32_t) ret_sect;

    // init locator
    if (fs_cnt_init_loca_in_sector(vdisk, loca_sid)) {
        sys_error("[cnt_init] Could not init locator");
        vdisk_note_sector_unused(vdisk, main_sid);
        vdisk_note_sector_unused(vdisk, loca_sid);
        return NULL_SID;
    }

    data = calloc(FS_SECTOR_SIZE);

    // add sector identifier
    data[0] = ST_CONT;
    data[1] = SF_HEAD;

    for (int i = 2; i < FS_SECTOR_SIZE; i++) {
        data[i] = 0;
    }

    // add meta and core sid
    mem_copy(data + 2, meta, min(str_len(meta), META_MAXLEN - 1));

    mem_copy(data + LAST_SID_OFFSET, &loca_sid, sizeof(sid_t));

    vdisk_write_sector(vdisk, main_sid, data);

    free(data);

    return main_sid;
}

char *fs_cnt_meta(filesys_t *filesys, sid_t sid, char *meta) {
    vdisk_t *vdisk;
    uint8_t *data;

    kprintf_serial("fs_cnt_meta\n");

    vdisk = fs_get_vdisk(filesys, sid.device);
    if (vdisk == NULL) {
        sys_warning("[cnt_meta] vdisk not found");
        return NULL;
    }

    data = vdisk_load_sector(vdisk, sid);

    if (data == NULL)
        return NULL;

    if (meta) {
        mem_copy(data + 2, meta, META_MAXLEN - 1);
    } else {
        meta = calloc(META_MAXLEN);
        mem_copy(meta, data + 2, META_MAXLEN - 1);
    }

    vdisk_unload_sector(vdisk, sid, data, SAVE);
    kprintf_serial("fs_cnt_metasdqds\n");

    return meta;
}
