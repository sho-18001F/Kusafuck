#include <stdio.h>
#include "libkusaf.h"

int main(void) {
    kusa_context *ctx = kusa_create_context();
    if (!ctx) {
        fputs("failed to create context\n", stderr);
        return 1;
    }

    int result = kusa_run_source(ctx, "+.+.;");
    if (result != 0) {
        fprintf(stderr, "run failed: %s\n", kusa_last_error(ctx));
        kusa_destroy_context(ctx);
        return 1;
    }

    printf("memory0=%u\n", kusa_get_memory(ctx, 0));
    kusa_destroy_context(ctx);
    return 0;
}
