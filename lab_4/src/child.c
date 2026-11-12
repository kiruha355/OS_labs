#include "ipc.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int parse_argument(const char *text, long *value)
{
    char *end;
    errno = 0;
    *value = strtol(text, &end, 10);
    return errno == 0 && end != text && *end == '\0' &&
           *value > 0 && *value <= INT_MAX ? 0 : -1;
}

static int sum_line(char *line, float *sum)
{
    char *position = line;
    int has_numbers = 0;
    *sum = 0.0f;
    for (;;) {
        while (isspace((unsigned char)*position))
            ++position;
        if (*position == '\0')
            return has_numbers;
        char *end;
        errno = 0;
        float number = strtof(position, &end);
        if (end == position || errno == ERANGE || !isfinite(number) ||
            (*end != '\0' && !isspace((unsigned char)*end)))
            return -1;
        double next = (double)*sum + (double)number;
        if (next > FLT_MAX || next < -FLT_MAX)
            return -2;
        *sum = (float)next;
        has_numbers = 1;
        position = end;
    }
}

static int publish(pid_t parent)
{
    if (getppid() != parent) {
        fprintf(stderr, "[ребёнок PID=%ld] родительский процесс завершился\n", (long)getpid());
        return -1;
    }
    if (notify_peer(parent, SIGUSR1) == -1)
        return -1;
    return wait_signal(SIGUSR2, parent, 0);
}

static int calculate(struct mailbox *box, pid_t parent)
{
    char *line = NULL;
    size_t capacity = 0, line_number = 0;
    int failed = 0;
    for (;;) {
        errno = 0;
        ssize_t count = getline(&line, &capacity, stdin);
        if (count == -1) {
            if (!feof(stdin)) {
                int reason = errno;
                snprintf(box->error, sizeof(box->error), "Ошибка чтения строки: %s",
                        strerror(reason ? reason : EIO));
                failed = 1;
            }
            break;
        }
        ++line_number;
        int result = memchr(line, '\0', (size_t)count) == NULL ?
                    sum_line(line, &box->sum) : -1;
        if (result < 0) {
            snprintf(box->error, sizeof(box->error), "%s",
                    result == -2 ? "переполнение суммы float" :
                    "некорректное число или значение вне диапазона float");
            failed = 1;
            break;
        }
        if (result == 0)
            continue;
        box->kind = RESULT;
        box->line = line_number;
        if (publish(parent) == -1) {
            free(line);
            if (fclose(stdin) == EOF)
                perror("Ошибка закрытия входного файла");
            return EXIT_FAILURE;
        }
    }
    free(line);
    if (fclose(stdin) == EOF) {
        perror("Ошибка закрытия входного файла");
        snprintf(box->error, sizeof(box->error), "Не удалось закрыть входной файл");
        failed = 1;
    }
    box->kind = failed ? FAILURE : FINISHED;
    box->line = line_number;
    if (publish(parent) == -1)
        return EXIT_FAILURE;
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    long fd_value, parent_value;
    if (argc != 3 || parse_argument(argv[1], &fd_value) == -1 ||
        parse_argument(argv[2], &parent_value) == -1 || fd_value < 3) {
        fprintf(stderr, "Использование: child <дескриптор отображаемого файла> <PID родителя>\n");
        return EXIT_FAILURE;
    }
    int fd = (int)fd_value;
    pid_t parent = (pid_t)parent_value;
    struct stat info;
    if (fstat(fd, &info) == -1) {
        perror("Ошибка получения сведений о временном файле");
        close_checked(fd);
        return EXIT_FAILURE;
    }
    if (!S_ISREG(info.st_mode) || info.st_size != (off_t)sizeof(struct mailbox)) {
        fprintf(stderr, "Некорректный файл для отображения\n");
        close_checked(fd);
        return EXIT_FAILURE;
    }
    struct mailbox *box = mmap(NULL, sizeof(*box), PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);
    if (box == MAP_FAILED) {
        perror("Ошибка отображения файла у дочернего процесса");
        close_checked(fd);
        return EXIT_FAILURE;
    }
    int failed = close_checked(fd) == -1;
    if (prepare_signals() == -1)
        failed = 1;
    int status = failed ? EXIT_FAILURE : calculate(box, parent);
    if (unmap_checked(box) == -1)
        status = EXIT_FAILURE;
    return status;
}
