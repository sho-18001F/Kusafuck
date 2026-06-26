#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kusa_stdlib.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#define MEMORY_SIZE 30000
#define MAX_LIBS 10         
#define MAX_FUNCTIONS 100   
#define CALL_STACK_SIZE 100 

char *libs[MAX_LIBS];
int lib_count = 0;

int function_pcs[MAX_FUNCTIONS];
int function_count = 0;

int call_stack[CALL_STACK_SIZE];
int call_stack_top = 0;

// 解放処理を共通化
void cleanup(char *code) {
    if (code) free(code);
    for (int i = 1; i <= lib_count; i++) {
        if (libs[i]) free(libs[i]);
    }
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb"); // Windows環境を考慮し、バイナリモードを推奨
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

int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001); 
#endif

    char *main_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc && (lib_count + 1) < MAX_LIBS) {
                lib_count++;
                libs[lib_count] = read_file(argv[i + 1]);
                if (!libs[lib_count]) {
                    printf("\033[1;31mエラー: ライブラリ '%s' が開けません。\033[0m\n", argv[i + 1]);
                    return 1;
                }
                i++;
            }
        } else {
            main_file = argv[i];
        }
    }

    if (!main_file) {
        printf("使用方法: %s <メインファイル.kf> [-l <ライブラリ.kf> ...]\n", argv[0]); // argv から argv[0] に修正
        return 1;
    }

    char *code = read_file(main_file);
    if (!code) {
        printf("\033[1;31mエラー: メインファイル '%s' が開けません。\033[0m\n", main_file);
        return 1;
    }

    unsigned short memory[MEMORY_SIZE] = {0};
    int dp = 0;
    int pc = 0;

    while (code[pc] != '\0') {
        char command = code[pc];

        switch (command) {
            case '+': memory[dp]++; break;
            case '-': memory[dp]--; break;
            case '>': 
                dp++;
                if (dp >= MEMORY_SIZE) dp = 0;
                break;
            case ',': 
                dp--;
                if (dp < 0) dp = MEMORY_SIZE - 1;
                break;

            case '.': { 
                unsigned int val = memory[dp];
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
                cleanup(code);
                return 0;

            case '(': 
                while (code[pc] != ')' && code[pc] != '\0') pc++;
                if (code[pc] == '\0') {
                    printf("\033[1;31mエラー: '(' に対応する ')' が閉じられていません。\033[0m\n");
                    cleanup(code);
                    return 1;
                }
                // pcは今 ')' を指しているので、switch文を出た後の pc++ で次の文字に進む
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
                printf("\n\033[1;33m--- MEMORY DUMP (現在位置: %d) ---\033[0m\n", dp);
                for (int i = 0; i < 10; i++) {
                    if (i == dp) printf("\033[1;32m[%d]: %d *\033[0m | ", i, memory[i]);
                    else printf("[%d]: %d | ", i, memory[i]);
                }
                printf("\n\033[1;33m---------------------------------\033[0m\n");
                break;

            case '%': { 
                int lib_idx = 1; 
                if (code[pc + 1] >= '1' && code[pc + 1] <= '9') {
                    lib_idx = code[pc + 1] - '0';
                    pc++; 
                }
                if (lib_idx <= lib_count && libs[lib_idx] != NULL) {
                    int remaining_len = strlen(&code[pc + 1]);
                    int lib_len = strlen(libs[lib_idx]);
                    char *new_code = malloc(lib_len + remaining_len + 1);
                    strcpy(new_code, libs[lib_idx]);
                    strcat(new_code, &code[pc + 1]);
                    
                    free(code);
                    code = new_code;
                    pc = -1; // 💡本来はインクルード用の別スタック管理が望ましいですが、一旦そのままにします
                }
                break;
            }

            case '{': 
                if (function_count < MAX_FUNCTIONS) {
                    function_pcs[function_count++] = pc + 1;
                } else {
                    printf("\033[1;31mエラー: 関数定義の最大数を超えました。\033[0m\n");
                    cleanup(code);
                    return 1;
                }
                int brace_count = 1;
                while (brace_count > 0) {
                    pc++;
                    if (code[pc] == '\0') {
                        printf("\033[1;31mエラー: '{' に対応する '}' が見つかりません。\033[0m\n");
                        cleanup(code);
                        return 1;
                    }
                    if (code[pc] == '{') brace_count++;
                    if (code[pc] == '}') brace_count--;
                }
                break;

            case '}': 
                if (call_stack_top > 0) {
                    pc = call_stack[--call_stack_top];
                }
                break;

            case '/': 
                if (memory[dp] < function_count) {
                    if (call_stack_top < CALL_STACK_SIZE) {
                        call_stack[call_stack_top++] = pc; 
                        pc = function_pcs[memory[dp]] - 1; 
                    } else {
                        printf("\033[1;31mエラー: コールスタックがオーバーフローしました。\033[0m\n");
                        cleanup(code);
                        return 1;
                    }
                } else {
                    // 標準外部ライブラリのシステムコール呼び出し
                    execute_syscall(memory, &dp);
                }
                break;

            case '[':
                if (memory[dp] == 0) {
                    int loop_count = 1;
                    while (loop_count > 0) {
                        pc++;
                        if (code[pc] == '\0') {
                            printf("\033[1;31mエラー: '[' に対応する ']' が見つかりません。\033[0m\n");
                            cleanup(code);
                            return 1;
                        }
                        if (code[pc] == '[') loop_count++;
                        if (code[pc] == ']') loop_count--;
                    }
                }
                break;

            case ']':
                if (memory[dp] != 0) {
                    int loop_count = 1;
                    while (loop_count > 0) {
                        pc--;
                        if (pc < 0) {
                            printf("\033[1;31mエラー: ']' に対応する '[' が見つかりません。\033[0m\n");
                            cleanup(code);
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

    cleanup(code);
    return 0;
}
