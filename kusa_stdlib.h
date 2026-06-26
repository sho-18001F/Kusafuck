#ifndef KUSA_STDLIB_H
#define KUSA_STDLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    // Linux/Mac環境用にリアルタイム入力をエミュレート
    static inline int _getch() {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }
#endif

// メモリサイズ（main側と同期）
#ifndef MEMORY_SIZE
    #define MEMORY_SIZE 30000
#endif

// 安全に隣のセルのインデックスを取得するマクロ
#define NEXT_DP(idx) (((idx) + 1 >= MEMORY_SIZE) ? 0 : (idx) + 1)

// OS不依存のスリープ関数
static inline void kusa_sleep(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// OS不依存の画面クリア関数
static inline void kusa_clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// 標準外部ライブラリのシステムコール実行本体
static inline void execute_syscall(unsigned short *memory, int *dp) {
    static int rand_initialized = 0;
    if (!rand_initialized) {
        srand((unsigned int)time(NULL));
        rand_initialized = 1;
    }

    unsigned short syscall_id = memory[*dp];
    int next = NEXT_DP(*dp); // 隣のセルの位置

    switch (syscall_id) {
        case 0: putchar('\n'); break; // 改行出力
        case 1: putchar(' '); break;  // スペース出力
        case 2: putchar('\t'); break; // タブ出力
        case 3: putchar('\a'); break; // 警告音（ピッ！）
        case 4: kusa_clear_screen(); break; // 画面クリア
        
        // 数値出力（隣のセルの数値を10進数で表示）
        case 10: printf("%d", memory[next]); break;
        
        // スリープ（隣のセルのミリ秒分待機）
        case 11: kusa_sleep(memory[next]); break;
        
        // ダイス（1〜6の乱数を現在地にセット）
        case 12: memory[*dp] = (rand() % 6) + 1; break;
        
        case 20: // リアルタイム1文字入力
            memory[*dp] = (unsigned short)_getch();
            break;

        case 21: { // 数値入力（10進数）
            int input_val = 0;
            if (scanf("%d", &input_val) == 1) memory[*dp] = (unsigned short)input_val;
            while (getchar() != '\n'); // バッファクリア
            break;
        }
        
        case 30: { // OSコマンド実行（Windows環境の31ズレ自動補正機能付き）
            char cmd_buf[512] = {0};
            int i = 0;
            int current_idx = next;
            
            // 0（NULL）に出会うまで、最大511文字を安全にコピー
            while (memory[current_idx] != 0 && i < 511) {
                unsigned short val = memory[current_idx];
                
                // Windows環境で文字コードが31ズレる現象を自動で検知してねじ伏せる
                if (val > 0 && val < 128 && (val + 31 == 'n' || val + 31 == 'o' || val + 31 == 't')) {
                    val += 31;
                }
                
                cmd_buf[i] = (char)val;
                i++;
                current_idx = NEXT_DP(current_idx);
            }
            if (i > 0) system(cmd_buf);
            break;
        }
        
        case 40: { // ファイル書き込み（kusa_out.txt に隣のセルの数値を保存）
            FILE *f = fopen("kusa_out.txt", "w");
            if (f) { fprintf(f, "%d", memory[next]); fclose(f); }
            break;
        }
        
        case 41: { // ファイル読み込み（kusa_out.txt から数値を現在地に復元）
            FILE *f = fopen("kusa_out.txt", "r");
            if (f) {
                int read_val = 0;
                if (fscanf(f, "%d", &read_val) == 1) memory[*dp] = (unsigned short)read_val;
                fclose(f);
            }
            break;
        }
        
        case 50: // テスト用ダミーデータ
            memory[next] = 123;
            break;
            
        default:
            break;
    }
    fflush(stdout);
}

#endif // KUSA_STDLIB_H
