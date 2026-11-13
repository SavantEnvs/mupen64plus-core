/* Known-answer probe of mupen64plus-core internals, built with the project's normal flags.
 * Prints key=value lines that mayhem/test.sh compares to fixed expected values. */
#define M64P_CORE_PROTOTYPES 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api/m64p_types.h"
#include "api/m64p_common.h"
#include "api/m64p_frontend.h"
#include "api/m64p_config.h"
#include "main/version.h"
#include "main/rom.h"
#include "main/cheat.h"
#include "backends/file_storage.h"
#include "device/dd/disk.h"

#define ROMSZ 4096

static void quiet(void *c, int l, const char *m) { (void)c; (void)l; (void)m; }

static void make_rom(uint8_t *rom, uint32_t crc1, uint32_t crc2, const char *name)
{
    size_t i;
    memset(rom, 0, ROMSZ);
    rom[0] = 0x80; rom[1] = 0x37; rom[2] = 0x12; rom[3] = 0x40;
    rom[4] = 0x00; rom[5] = 0x00; rom[6] = 0x00; rom[7] = 0x0F;       /* clock rate */
    rom[8] = 0x80; rom[9] = 0x00; rom[10] = 0x04; rom[11] = 0x00;     /* PC */
    rom[16] = crc1 >> 24; rom[17] = crc1 >> 16; rom[18] = crc1 >> 8; rom[19] = crc1;
    rom[20] = crc2 >> 24; rom[21] = crc2 >> 16; rom[22] = crc2 >> 8; rom[23] = crc2;
    memset(rom + 32, ' ', 20);
    memcpy(rom + 32, name, strlen(name));
    rom[0x3E] = 'E';                                                   /* country: USA */
    for (i = 0x40; i < ROMSZ; i++)
        rom[i] = (uint8_t)(i * 7 + 3);
}

static void to_v64(uint8_t *dst, const uint8_t *src)
{
    size_t i;
    for (i = 0; i < ROMSZ; i += 2) { dst[i] = src[i + 1]; dst[i + 1] = src[i]; }
}

static void to_n64(uint8_t *dst, const uint8_t *src)
{
    size_t i;
    for (i = 0; i < ROMSZ; i += 4) {
        dst[i] = src[i + 3]; dst[i + 1] = src[i + 2]; dst[i + 2] = src[i + 1]; dst[i + 3] = src[i];
    }
}

static void report_rom(const char *tag, const uint8_t *img, size_t size)
{
    m64p_error rc = open_rom(img, (unsigned int)size);
    printf("%s.rc=%d\n", tag, (int)rc);
    if (rc == M64ERR_SUCCESS) {
        printf("%s.md5=%s\n", tag, ROM_SETTINGS.MD5);
        printf("%s.name=%s\n", tag, ROM_PARAMS.headername);
        printf("%s.goodname=%s\n", tag, ROM_SETTINGS.goodname);
        printf("%s.systemtype=%d\n", tag, (int)ROM_PARAMS.systemtype);
        printf("%s.savetype=%d\n", tag, (int)ROM_SETTINGS.savetype);
        printf("%s.players=%d\n", tag, (int)ROM_SETTINGS.players);
        printf("%s.status=%d\n", tag, (int)ROM_SETTINGS.status);
        printf("%s.cheats=%s\n", tag, ROM_PARAMS.cheats != NULL ? ROM_PARAMS.cheats : "(none)");
        close_rom();
    }
}

int main(void)
{
    uint8_t rom[ROMSZ], alt[ROMSZ];
    char cfg[] = "/tmp/m64p-selftest-XXXXXX";
    char cfgdir[64];
    char ini_path[96];
    const char *datadir = getenv("M64P_DATA_DIR");
    FILE *f;

    snprintf(cfgdir, sizeof(cfgdir), "/tmp/m64p-selftest-cfg-%d", (int)getpid());
    if (CoreStartup(FRONTEND_API_VERSION, cfgdir, datadir != NULL ? datadir : "/mayhem/data",
                    NULL, quiet, NULL, NULL) != M64ERR_SUCCESS) {
        printf("startup=failed\n");
        return 1;
    }
    printf("startup=ok\n");

    /* ROM header parsing in the three byte orders; unknown ROM */
    make_rom(rom, 0x12345678u, 0x9ABCDEF0u, "MAYHEM TEST ROM");
    report_rom("z64", rom, ROMSZ);
    to_v64(alt, rom);
    report_rom("v64", alt, ROMSZ);
    to_n64(alt, rom);
    report_rom("n64", alt, ROMSZ);

    /* ROM database lookup by CRC */
    make_rom(rom, 0xF7F52DB8u, 0x2195E636u, "ZELDA");
    report_rom("zelda", rom, ROMSZ);

    /* invalid image is rejected */
    memset(rom, 0x55, sizeof(rom));
    printf("bad.rc=%d\n", (int)open_rom(rom, ROMSZ));

    /* external config reader */
    mkdtemp(cfg);
    snprintf(ini_path, sizeof(ini_path), "%s/test.cfg", cfg);
    f = fopen(ini_path, "wb");
    if (f != NULL) {
        fputs("# comment\n[Video-General]\nFullscreen = True\nScreenWidth = 1024\n\n"
              "[Core]\nVersion = 1.01\nSaveStatePath = \"/x y\"\n", f);
        fclose(f);
    }
    {
        m64p_handle h = NULL;
        char v[64];
        printf("cfg.open=%d\n", (int)ConfigExternalOpen(ini_path, &h));
        v[0] = 0; printf("cfg.width.rc=%d\n", (int)ConfigExternalGetParameter(h, "Video-General", "ScreenWidth", v, sizeof(v)));
        printf("cfg.width=%s\n", v);
        v[0] = 0; ConfigExternalGetParameter(h, "Core", "SaveStatePath", v, sizeof(v));
        printf("cfg.path=%s\n", v);
        v[0] = 0; ConfigExternalGetParameter(h, "core", "version", v, sizeof(v));
        printf("cfg.version=%s\n", v);
        printf("cfg.missing.rc=%d\n", (int)ConfigExternalGetParameter(h, "Core", "Nope", v, sizeof(v)));
        ConfigExternalClose(h);
    }
    unlink(ini_path);
    rmdir(cfg);

    /* cheat hack string parser */
    {
        struct cheat_ctx ctx;
        cheat_init(&ctx);
        cheat_add_hacks(&ctx, "D109A814 0320,8109A814 0000;garbage;81000000 1234");
        printf("cheat.hack0=%d\n", cheat_set_enabled(&ctx, "HACK0", 1));
        printf("cheat.hack1=%d\n", cheat_set_enabled(&ctx, "HACK1", 1));
        printf("cheat.hack2=%d\n", cheat_set_enabled(&ctx, "HACK2", 1));
        cheat_delete_all(&ctx);
        cheat_uninit(&ctx);
    }

    /* 64DD disk: a minimal D64 image is recognised and expanded */
    {
        uint8_t *d;
        uint8_t *out;
        unsigned int format = 99, development = 99;
        size_t off_sys = 1, off_id = 1, off_ram = 1, size_ram = 1;
        size_t size;
        /* disk type 0, rom_lba_end 0 -> ROM area is LBAToByteA(0, 24, 1) bytes */
        size = D64_OFFSET_DATA + LBAToByteA(0, 24, 1);
        d = calloc(1, size);
        d[0] = 0xE8; d[1] = 0x48; d[2] = 0xD3; d[3] = 0x16;           /* region: US */
        d[5] = 0x00;                                                  /* type 0 */
        d[6] = 0x00; d[7] = 0x20;                                     /* ipl_load_blk */
        d[28] = 0x80; d[29] = 0x00; d[30] = 0x10; d[31] = 0x00;       /* ipl_load_addr */
        d[0xE0] = 0; d[0xE1] = 0;                                     /* rom_lba_end */
        d[0xE2] = 0xFF; d[0xE3] = 0xFF; d[0xE4] = 0xFF; d[0xE5] = 0xFF;
        out = scan_and_expand_disk_format(d, size, &format, &development, &off_sys, &off_id, &off_ram, &size_ram);
        printf("dd.ok=%d\n", out != NULL);
        printf("dd.format=%u\n", format);
        printf("dd.development=%u\n", development);
        printf("dd.off_sys=%zu\n", off_sys);
        printf("dd.off_id=%zu\n", off_id);
        printf("dd.off_ram=%zu\n", off_ram);
        printf("dd.size_ram=%zu\n", size_ram);
        printf("dd.format_name=%s\n", get_disk_format_name(format));
        free(out != NULL ? out : d);
    }

    CoreShutdown();
    printf("done=1\n");
    return 0;
}
