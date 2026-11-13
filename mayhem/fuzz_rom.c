/* Feeds arbitrary bytes to open_rom(): header validation, byte-order detection and swapping,
 * MD5 + CRC lookup in the real ROM database, and the cheat-hack string parser. */
#include <stdint.h>
#include <stddef.h>

#include "m64p_fuzz_init.h"
#include "main/rom.h"
#include "main/cheat.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    const char *d = getenv("M64P_DATA_DIR");
    (void)argc; (void)argv;
    m64p_fuzz_startup(d != NULL ? d : "/mayhem/data");
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct cheat_ctx ctx;

    if (size < 4096 || size > (64u << 20))
        return -1;
    if (open_rom(data, (unsigned int)size) != M64ERR_SUCCESS)
        return -1;

    cheat_init(&ctx);
    cheat_add_hacks(&ctx, ROM_PARAMS.cheats);
    cheat_delete_all(&ctx);
    cheat_uninit(&ctx);

    close_rom();
    return 0;
}
