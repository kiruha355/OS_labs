#include "commands.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int token_ended(char c)
{
    return c == '\0' || isspace((unsigned char)c);
}

static int read_long(char **text, long *value)
{
    char *end;
    errno = 0;
    *value = strtol(*text, &end, 10);
    if (end == *text || !token_ended(*end))
        return 0;
    *text = end;
    return 1;
}

static int read_float(char **text, float *value)
{
    char *end;
    errno = 0;
    *value = strtof(*text, &end);
    if (end == *text || !token_ended(*end))
        return 0;
    *text = end;
    return 1;
}

static int at_end(const char *text)
{
    while (isspace((unsigned char)*text))
        ++text;
    return *text == '\0';
}

int run_commands(struct contracts *functions, void (*switch_library)(struct contracts *))
{
    puts("Введите номер функции и ее параматеры:");
    puts("1 начало конец шаг     вычислить интеграл sin(x)");
    puts("2 число     перевести число из десятичной системы");
    if (switch_library != NULL)
        puts("нажмите 0 чтобы переключить библиотеку");

    char line[128];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
            fputs("Слишком длинная команда.\n", stderr);
            continue;
        }
        if (at_end(line))
            continue;

        char *args = line;
        long command;
        if (!read_long(&args, &command)) {
            fputs("Введите номер команды.\n", stderr);
            continue;
        }

        if (command == 0 && at_end(args) && switch_library != NULL) {
            switch_library(functions);
        } else if (command == 1) {
            float A, B, e;
            if (!read_float(&args, &A) || !read_float(&args, &B) ||
                !read_float(&args, &e) || !at_end(args) || A > B || e <= 0) {
                fputs("После 1 введите через пробел начало отрезка, конец и шаг.\n"
                    "Начало должно быть не больше конца. Шаг должен быть больше нуля.\n", stderr);
                continue;
            }
            float result = functions->integral(A, B, e);
            if (isfinite(result))
                printf("%.9g\n", (double)result);
            else
                fputs("Не удалось вычислить интеграл с заданными параметрами.\n", stderr);
        } else if (command == 2) {
            long number;
            if (!read_long(&args, &number) || !at_end(args)) {
                fputs("Введите одно целое число.\n", stderr);
                continue;
            }
            char *result = functions->translate(number);
            if (result == NULL) {
                fputs("Не удалось выделить память для результата.\n", stderr);
                continue;
            }
            puts(result);
            free(result);
        } else {
            fputs("Неизвестная команда или лишние аргументы.\n", stderr);
        }
    }

    if (ferror(stdin)) {
        perror("Ошибка чтения команды");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
