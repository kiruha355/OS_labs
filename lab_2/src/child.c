#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(void) {
    char *line = NULL;
    size_t capacity = 0;

    while (getline(&line, &capacity, stdin) != -1) {
        char *position = line;
        float sum = 0.0f;
        int has_numbers = 0;

        while (1) {
            while (isspace((unsigned char)*position)) {
                position++;
            }

            if (*position == '\0') {
                break;
            }

            char *end;
            float number = strtof(position, &end);

            if (end == position) {
                fprintf(stderr, "Ошибка: некорректное число в строке\n");
                free(line);
                return 1;
            }

            sum += number;
            has_numbers = 1;
            position = end;
        }

        if (!has_numbers) {
            continue;
        }

        printf("%g\n", sum);
        fflush(stdout);
    }

    if (!feof(stdin)) {
        fprintf(stderr, "Ошибка чтения строки\n");
        free(line);
        return 1;
    }

    free(line);
    return 0;
}
