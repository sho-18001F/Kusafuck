#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// 💡 分離した標準ライブラリヘッダーを読み込む
#include "kusa_stdlib.h"
#include "libkusaf.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#define MAX_LIBS 10
#define MAX_FUNCTIONS 100
#define CALL_STACK_SIZE 100

struct kusa_context {
    unsigned short memory[MEMORY_SIZE];
    int dp;
    char *libs[MAX_LIBS];
    int lib_count;
    int function_pcs[MAX_FUNCTIONS];
    int function_count;
    int call_stack[CALL_STACK_SIZE];
    int call_stack_top;
    char last_error[256];
};

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (buf) {
        size_t read_bytes = fread(buf, 1, size, file);
        buf[read_bytes] = '\0';
    }
    fclose(file);
    return buf;
}

static void set_error(kusa_context *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(ctx->last_error, sizeof(ctx->last_error), fmt, args);
    va_end(args);
}

static void cleanup_code(char *code) {
    if (code) free(code);
}

static void cleanup_context_libs(kusa_context *ctx) {
    for (int i = 0; i < ctx->lib_count; i++) {
        if (ctx->libs[i]) free(ctx->libs[i]);
        ctx->libs[i] = NULL;
    }
    ctx->lib_count = 0;
}

static int preprocess_functions(kusa_context *ctx, const char *code) {
    ctx->function_count = 0;
    int scan_pc = 0;
    while (code[scan_pc] != '\0') {
        if (code[scan_pc] == '(') {
            while (code[scan_pc] != ')' && code[scan_pc] != '\0') scan_pc++;
        } else if (code[scan_pc] == '{') {
            if (ctx->function_count < MAX_FUNCTIONS) {
                ctx->function_pcs[ctx->function_count++] = scan_pc + 1;
            }
            int brace_count = 1;
            while (brace_count > 0 && code[scan_pc] != '\0') {
                scan_pc++;
                if (code[scan_pc] == '{') brace_count++;
                if (code[scan_pc] == '}') brace_count--;
            }
        }
        if (code[scan_pc] != '\0') scan_pc++;
    }
    return 0;
}

static int run_code(kusa_context *ctx, char *code) {
    int pc = 0;
    ctx->call_stack_top = 0;
    ctx->dp = 0;
    preprocess_functions(ctx, code);

    while (code[pc] != '\0') {
        char command = code[pc];

        switch (command) {
            case '+': ctx->memory[ctx->dp]++; break;
            case '-': ctx->memory[ctx->dp]--; break;
            case '>':
                ctx->dp++;
                if (ctx->dp >= MEMORY_SIZE) ctx->dp = 0;
                break;
            case ',':
                ctx->dp--;
                if (ctx->dp < 0) ctx->dp = MEMORY_SIZE - 1;
                break;

            case '.': {
                unsigned int val = ctx->memory[ctx->dp];
                if (val < 0x80) {
                    putchar(val);
                } else if (val < 0x800) {
                    putchar(0xC0 | (val >> 6));
                    putchar(0x80 | (val & 0x3F));
                } else {
                    putchar(0xE0 | (val >> 12));
                    putchar(0x80 | ((val >> 6) & 0x3F));
                    putchar(0x80 | (val & 0x3F));
                }
                fflush(stdout);
                break;
            }

            case ';':
                return 0;

            case '(':
                while (code[pc] != ')' && code[pc] != '\0') pc++;
                if (code[pc] == '\0') {
                    set_error(ctx, "'(' に対応する ')' が閉じられていません");
                    return 1;
                }
                break;

            case '?':
                printf("\n\033[1;36m--- kusa言語 命令ヘルプ ---\033[0m\n");
                printf(" + : 値+1   - : 値-1\n");
                printf(" > : 右移動  , : 左移動\n");
                printf(" . : 出力    ; : HALT(終了)\n");
                printf(" [ ]: ループ { } /: 関数\n");
                printf(" d : メモリダンプ  ? : ヘルプ\n");
                printf("\033[1;36m---------------------------\033[0m\n");
                break;

            case 'd':
                printf("\n\033[1;33m--- memory dump (現在位置: %d) ---\033[0m\n", ctx->dp);
                for (int i = 0; i < 10; i++) {
                    if (i == ctx->dp) printf("\033[1;32m[%d]: %d *\033[0m | ", i, ctx->memory[i]);
                    else printf("[%d]: %d | ", i, ctx->memory[i]);
                }
                printf("\n\033[1;33m---------------------------------\033[0m\n");
                break;

            case '%': {
                int lib_idx = 1;
                if (code[pc + 1] >= '1' && code[pc + 1] <= '9') {
                    lib_idx = code[pc + 1] - '0';
                    pc++;
                }
                if (lib_idx <= ctx->lib_count && ctx->libs[lib_idx] != NULL) {
                    int remaining_len = strlen(&code[pc + 1]);
                    int lib_len = strlen(ctx->libs[lib_idx]);
                    char *new_code = malloc(lib_len + remaining_len + 1);
                    if (!new_code) {
                        set_error(ctx, "メモリ不足です");
                        return 1;
                    }
                    strcpy(new_code, ctx->libs[lib_idx]);
                    strcat(new_code, &code[pc + 1]);
                    cleanup_code(code);
                    code = new_code;
                    pc = -1;
                }
                break;
            }

            case '{': {
                int brace_count = 1;
                while (brace_count > 0) {
                    pc++;
                    if (code[pc] == '\0') {
                        set_error(ctx, "'{' に対応する '}' が見つかりません");
                        return 1;
                    }
                    if (code[pc] == '{') brace_count++;
                    if (code[pc] == '}') brace_count--;
                }
                break;
            }

            case '}':
                if (ctx->call_stack_top > 0) {
                    pc = ctx->call_stack[--ctx->call_stack_top];
                }
                break;

            case '/':
                if (ctx->function_count > 0 && ctx->memory[ctx->dp] < ctx->function_count) {
                    if (ctx->call_stack_top < CALL_STACK_SIZE) {
                        ctx->call_stack[ctx->call_stack_top++] = pc;
                        pc = ctx->function_pcs[ctx->memory[ctx->dp]] - 1;
                    } else {
                        set_error(ctx, "コールスタックがオーバーフローしました");
                        return 1;
                    }
                } else {
                    execute_syscall(ctx->memory, &ctx->dp);
                }
                break;

            case '[':
                if (ctx->memory[ctx->dp] == 0) {
                    int loop_count = 1;
                    while (loop_count > 0) {
                        pc++;
                        if (code[pc] == '\0') {
                            set_error(ctx, "'[' に対応する ']' が見つかりません");
                            return 1;
                        }
                        if (code[pc] == '[') loop_count++;
                        if (code[pc] == ']') loop_count--;
                    }
                }
                break;

            case ']':
                if (ctx->memory[ctx->dp] != 0) {
                    int loop_count = 1;
                    while (loop_count > 0) {
                        pc--;
                        if (pc < 0) {
                            set_error(ctx, "']' に対応する '[' が見つかりません");
                            return 1;
                        }
                        if (code[pc] == ']') loop_count++;
                        if (code[pc] == '[') loop_count--;
                    }
                }
                break;
        }
        pc++;
    }

    cleanup_code(code);
    return 0;
}

kusa_context *kusa_create_context(void) {
    kusa_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    ctx->last_error[0] = '\0';
    return ctx;
}

void kusa_destroy_context(kusa_context *ctx) {
    if (!ctx) return;
    cleanup_context_libs(ctx);
    free(ctx);
}

int kusa_load_library(kusa_context *ctx, const char *filename) {
    if (!ctx || !filename) {
        return -1;
    }
    if (ctx->lib_count >= MAX_LIBS) {
        set_error(ctx, "ライブラリが多すぎます");
        return -1;
    }

    char *lib_code = read_file(filename);
    if (!lib_code) {
        set_error(ctx, "ライブラリ '%s' が開けません", filename);
        return -1;
    }

    ctx->libs[ctx->lib_count++] = lib_code;
    return 0;
}

int kusa_run_source(kusa_context *ctx, const char *source) {
    if (!ctx || !source) {
        return -1;
    }

    size_t len = strlen(source) + 1;
    char *code = malloc(len);
    if (!code) {
        set_error(ctx, "メモリ不足です");
        return -1;
    }
    memcpy(code, source, len);
    int result = run_code(ctx, code);
    if (result != 0) {
        cleanup_code(code);
        return result;
    }
    return 0;
}

int kusa_run_file(kusa_context *ctx, const char *filename) {
    if (!ctx || !filename) {
        return -1;
    }

    char *code = read_file(filename);
    if (!code) {
        set_error(ctx, "メインファイル '%s' が開けません", filename);
        return -1;
    }

    int result = run_code(ctx, code);
    if (result != 0) {
        cleanup_code(code);
        return result;
    }
    return 0;
}

unsigned short kusa_get_memory(const kusa_context *ctx, int index) {
    if (!ctx || index < 0 || index >= MEMORY_SIZE) return 0;
    return ctx->memory[index];
}

void kusa_set_memory(kusa_context *ctx, int index, unsigned short value) {
    if (!ctx || index < 0 || index >= MEMORY_SIZE) return;
    ctx->memory[index] = value;
}

int kusa_get_data_pointer(const kusa_context *ctx) {
    return ctx ? ctx->dp : 0;
}

void kusa_set_data_pointer(kusa_context *ctx, int index) {
    if (!ctx) return;
    ctx->dp = index;
}

const char *kusa_last_error(const kusa_context *ctx) {
    return ctx ? ctx->last_error : "";
}

#ifndef KUSA_NO_MAIN
int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // 出力をUTF-8に固定
#endif

    kusa_context *ctx = kusa_create_context();
    if (!ctx) {
        fprintf(stderr, "メモリ不足です\n");
        return 1;
    }

    char *main_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc) {
                if (kusa_load_library(ctx, argv[i + 1]) != 0) {
                    fprintf(stderr, "\033[1;31m%s\033[0m\n", kusa_last_error(ctx));
                    kusa_destroy_context(ctx);
                    return 1;
                }
                i++;
            }
        } else {
            main_file = argv[i];
        }
    }

    if (!main_file) {
        printf("使用方法: %s <メインファイル.kf> [-l <ライブラリ.kf> ...] \n", argv[0]);
        kusa_destroy_context(ctx);
        return 1;
    }

    if (kusa_run_file(ctx, main_file) != 0) {
        fprintf(stderr, "\033[1;31m%s\033[0m\n", kusa_last_error(ctx));
        kusa_destroy_context(ctx);
        return 1;
    }

    kusa_destroy_context(ctx);
    return 0;
}
#endif
