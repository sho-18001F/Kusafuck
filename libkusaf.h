#ifndef LIBKUSAF_H
#define LIBKUSAF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "kusa_stdlib.h"

typedef struct kusa_context kusa_context;

kusa_context *kusa_create_context(void);
void kusa_destroy_context(kusa_context *ctx);

int kusa_load_library(kusa_context *ctx, const char *filename);
int kusa_run_source(kusa_context *ctx, const char *source);
int kusa_run_file(kusa_context *ctx, const char *filename);

unsigned short kusa_get_memory(const kusa_context *ctx, int index);
void kusa_set_memory(kusa_context *ctx, int index, unsigned short value);
int kusa_get_data_pointer(const kusa_context *ctx);
void kusa_set_data_pointer(kusa_context *ctx, int index);
const char *kusa_last_error(const kusa_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
