/* Feeds arbitrary text to the ROM-database cheat string parser (cheat_add_hacks). */
#include <stdint.h>
#include <stddef.h>

#include "m64p_fuzz_init.h"
#include "main/cheat.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    m64p_fuzz_startup(NULL);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct cheat_ctx ctx;
    char *str;

    if (size > 65536)
        return -1;
    str = malloc(size + 1);
    if (str == NULL)
        return -1;
    memcpy(str, data, size);
    str[size] = '\0';

    cheat_init(&ctx);
    cheat_add_hacks(&ctx, str);
    cheat_delete_all(&ctx);
    cheat_uninit(&ctx);
    free(str);
    return 0;
}
