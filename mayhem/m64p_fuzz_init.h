#ifndef M64P_FUZZ_INIT_H
#define M64P_FUZZ_INIT_H

#define M64P_CORE_PROTOTYPES 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api/m64p_types.h"
#include "api/m64p_common.h"
#include "api/m64p_frontend.h"
#include "main/version.h"

static void m64p_fuzz_quiet(void *ctx, int level, const char *msg)
{
    (void)ctx; (void)level; (void)msg;
}

/* Create a private scratch directory under $TMPDIR (default /tmp); aborts on failure. */
static void m64p_fuzz_mkdtemp(const char *name, char *out, size_t outsz)
{
    const char *tmp = getenv("TMPDIR");
    if (tmp == NULL || tmp[0] == '\0')
        tmp = "/tmp";
    if ((size_t)snprintf(out, outsz, "%s/%s-XXXXXX", tmp, name) >= outsz || mkdtemp(out) == NULL)
        abort();
}

/* Start the core once. datadir (may be NULL) is where mupen64plus.ini is looked up. */
static void m64p_fuzz_startup(const char *datadir)
{
    char cfgdir[256];
    m64p_fuzz_mkdtemp("m64p-cfg", cfgdir, sizeof(cfgdir));
    if (CoreStartup(FRONTEND_API_VERSION, cfgdir, datadir, NULL, m64p_fuzz_quiet, NULL, NULL) != M64ERR_SUCCESS)
        abort();
}

#endif
