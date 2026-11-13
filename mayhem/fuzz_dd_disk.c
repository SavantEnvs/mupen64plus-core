/* Feeds arbitrary bytes to the 64DD disk image scanner (format detection, D64 expansion), then
 * builds the LBA table and resolves sectors the way the emulated 64DD would. */
#include <stdint.h>
#include <stddef.h>

#include "m64p_fuzz_init.h"
#include "backends/file_storage.h"
#include "device/dd/disk.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    m64p_fuzz_startup(NULL);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static const unsigned int tracks[] = {
        0, 1, 5, 6, 0x8f, 0x90, 0x91, 0x13c, 0x1d1, 0x266, 0x2fb, 0x390, 0x425, 0x48a, 0x4a0
    };
    struct file_storage fs;
    struct dd_disk disk;
    unsigned int format = 0, development = 0;
    size_t offset_sys = 0, offset_id = 0, offset_ram = 0, size_ram = 0;
    uint8_t *buf, *scanned;
    unsigned int head, t, block, sector, lba;

    if (size < 0x100 || size > MAME_FORMAT_DUMP_SIZE + 0x1000)
        return -1;
    buf = malloc(size);
    if (buf == NULL)
        return -1;
    memcpy(buf, data, size);

    scanned = scan_and_expand_disk_format(buf, size, &format, &development,
                                          &offset_sys, &offset_id, &offset_ram, &size_ram);
    if (scanned == NULL) {
        free(buf);
        return -1;
    }

    memset(&fs, 0, sizeof(fs));
    fs.data = scanned;
    fs.size = (format == DISK_FORMAT_D64) ? offset_ram + size_ram : size;

    memset(&disk, 0, sizeof(disk));
    disk.storage = &fs;
    disk.istorage = &g_ifile_storage_ro;
    disk.format = (uint8_t)format;
    disk.development = (uint8_t)development;
    disk.offset_sys = offset_sys;
    disk.offset_id = offset_id;
    disk.offset_ram = offset_ram;

    GenerateLBAToPhysTable(&disk);

    for (head = 0; head < 2; head++)
        for (t = 0; t < sizeof(tracks) / sizeof(tracks[0]); t++)
            for (block = 0; block < 2; block++)
                for (sector = 0; sector < SECTORS_PER_BLOCK; sector += 42)
                    (void)get_sector_base(&disk, head, tracks[t], block, sector, 232);

    /* The 64DD seeks by LBA: visit the physical position of sampled LBAs (system data, disk ID,
     * ROM and RAM areas) through the table GenerateLBAToPhysTable just built. */
    if (disk.format != DISK_FORMAT_MAME) {
        for (lba = 0; lba < SIZE_LBA; lba += (lba < 32) ? 1 : 61) {
            uint16_t phys = disk.lba_phys_table[lba];
            for (sector = 0; sector < SECTORS_PER_BLOCK; sector += 42)
                (void)get_sector_base(&disk, (phys >> 12) & 1, phys & 0xfff, (phys >> 13) & 1, sector, 232);
        }
    }

    free(scanned);
    return 0;
}
