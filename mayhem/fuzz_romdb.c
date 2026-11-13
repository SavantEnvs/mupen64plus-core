/* Feeds arbitrary bytes to the ROM database (mupen64plus.ini) loader, then probes the index
 * built from it. The file is placed in a private shared-data directory the core searches. */
#include <stdint.h>
#include <stddef.h>

#include "m64p_fuzz_init.h"
#include "main/rom.h"
#include "main/cheat.h"

static char l_path[320];

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    char dir[256];
    (void)argc; (void)argv;
    m64p_fuzz_mkdtemp("m64p-fuzz-romdb", dir, sizeof(dir));
    snprintf(l_path, sizeof(l_path), "%s/mupen64plus.ini", dir);
    m64p_fuzz_startup(dir);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *f;
    char *text;
    int probes = 0;

    if (size > (1u << 20))
        return -1;
    f = fopen(l_path, "wb");
    if (f == NULL)
        return -1;
    if (size > 0 && fwrite(data, 1, size, f) != size) {
        fclose(f);
        return -1;
    }
    fclose(f);

    romdatabase_close();
    romdatabase_open();

    /* Probe the CRC index with every "CRC=XXXXXXXX XXXXXXXX" line of the input. */
    text = malloc(size + 1);
    if (text != NULL) {
        char *p;
        memcpy(text, data, size);
        text[size] = '\0';
        for (p = text; (p = strstr(p, "CRC=")) != NULL && probes < 32; p += 4) {
            unsigned int crc1 = 0, crc2 = 0;
            if (sscanf(p, "CRC=%8x %8x", &crc1, &crc2) == 2) {
                romdatabase_entry *e = ini_search_by_crc(crc1, crc2);
                if (e != NULL && e->cheats != NULL) {
                    struct cheat_ctx ctx;
                    cheat_init(&ctx);
                    cheat_add_hacks(&ctx, e->cheats);
                    cheat_delete_all(&ctx);
                    cheat_uninit(&ctx);
                }
                probes++;
            }
        }
        free(text);
    }

    romdatabase_close();
    return 0;
}
