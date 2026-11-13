/* Feeds arbitrary bytes to the external config (INI) file reader: ConfigExternalOpen +
 * ConfigExternalGetParameter, which runs ini_parse_line over the whole file for each lookup.
 *
 * Upstream bug (mayhem/fuzz_config/known-findings/config-external-not-terminated): ConfigExternalOpen
 * never NUL-terminates the file buffer, so a file whose last byte is not '\n' makes strchr read past
 * the allocation. It fires on a large share of mutated inputs, which stops Mayhem's -runs=5 start-up
 * probe and hides every other parser path, and it cannot be patched additively. The harness therefore
 * ends the file with '\n' (what a text config file normally ends with); setting
 * M64P_FUZZ_RAW_CONFIG=1 writes the bytes untouched to reproduce the finding. */
#include <stdint.h>
#include <stddef.h>

#include "m64p_fuzz_init.h"
#include "api/m64p_config.h"

static char l_path[320];
static int l_raw;

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    char dir[256];
    (void)argc; (void)argv;
    m64p_fuzz_startup(NULL);
    l_raw = getenv("M64P_FUZZ_RAW_CONFIG") != NULL;
    m64p_fuzz_mkdtemp("m64p-fuzz-config", dir, sizeof(dir));
    snprintf(l_path, sizeof(l_path), "%s/input.ini", dir);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *f;
    m64p_handle h = NULL;
    char value[64];
    char section[64] = "Core";
    size_t pos = 0;
    int lookups = 0;

    if (size > 65536)
        return -1;
    f = fopen(l_path, "wb");
    if (f == NULL)
        return -1;
    if (size > 0 && fwrite(data, 1, size, f) != size) {
        fclose(f);
        return -1;
    }
    if (!l_raw && (size == 0 || data[size - 1] != '\n'))
        fputc('\n', f);
    fclose(f);

    if (ConfigExternalOpen(l_path, &h) != M64ERR_SUCCESS)
        return -1;

    /* Look up the sections and keys the input itself names, plus a fixed one. */
    ConfigExternalGetParameter(h, "Core", "Version", value, sizeof(value));
    while (pos < size && lookups < 16) {
        size_t start = pos, len;
        char name[64];
        while (pos < size && data[pos] != '\n')
            pos++;
        len = pos - start;
        pos++;
        if (len == 0)
            continue;
        if (data[start] == '[') {
            size_t n = 0;
            for (size_t i = start + 1; i < start + len && data[i] != ']' && n < sizeof(section) - 1; i++)
                section[n++] = (char)data[i];
            section[n] = '\0';
        } else {
            size_t n = 0;
            for (size_t i = start; i < start + len && data[i] != '=' && data[i] != ' ' && n < sizeof(name) - 1; i++)
                name[n++] = (char)data[i];
            name[n] = '\0';
            if (n > 0) {
                ConfigExternalGetParameter(h, section, name, value, sizeof(value));
                lookups++;
            }
        }
    }

    ConfigExternalClose(h);
    return 0;
}
