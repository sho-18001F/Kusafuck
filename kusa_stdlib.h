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
    int _getch() {
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

// OS不依存のスリープ関数
void kusa_sleep(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// OS不依存の画面クリア関数
void kusa_clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void execute_syscall(unsigned short *memory, int *dp) {
    static int rand_initialized = 0;
    if (!rand_initialized) {
        srand((unsigned int)time(NULL));
        rand_initialized = 1;
    }

    unsigned short syscall_id = memory[*dp];

    switch (syscall_id) {
        case 0: putchar('\n'); break;
        case 1: putchar(' '); break;
        case 2: putchar('\t'); break;
        case 3: putchar('\a'); break;
        case 4: kusa_clear_screen(); break;
        case 10: printf("%d", memory[*dp]); break;
        case 11: kusa_sleep(memory[*dp + 1]); break;
        case 12: memory[*dp] = (rand() % 6) + 1; break;
        
        case 20: // リアルタイム1文字入力
#ifdef _WIN32
            memory[*dp] = (unsigned short)_getch();
#else
            memory[*dp] = (unsigned short)_getch();
#endif
            break;

        case 21: {
            int input_val = 0;
            if (scanf("%d", &input_val) == 1) memory[*dp] = (unsigned short)input_val;
            while (getchar() != '\n');
            break;
        }
        case 30: {
            char cmd_buf[512] = {0};
            int i = 0;
            while (memory[*dp + 1 + i] != 0 && i < 511) {
                cmd_buf[i] = (char)memory[*dp + 1 + i];
                i++;
            }
            if (i > 0) system(cmd_buf);
            break;
        }
        case 40: {
            FILE *f = fopen("kusa_out.txt", "w");
            if (f) { fprintf(f, "%d", memory[*dp + 1]); fclose(f); }
            break;
        }
        case 41: {
            FILE *f = fopen("kusa_out.txt", "r");
            if (f) {
                int read_val = 0;
                if (fscanf(f, "%d", &read_val) == 1) memory[*dp] = (unsigned short)read_val;
                fclose(f);
            }
            break;
        }
        case 50:
            memory[*dp + 1] = 123;
            break;
        default:
            break;
    }
    fflush(stdout);
}

#endif // KUSA_STDLIB_H
