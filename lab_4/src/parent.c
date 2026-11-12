#include "ipc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int open_input(void)
{
    char *name = NULL;
    size_t capacity = 0;
    if (fputs("Введите имя файла: ", stderr) == EOF) {
        return -1;
    }
    ssize_t length = getline(&name, &capacity, stdin);
    if (length < 0) {
        if (!feof(stdin))
            perror("Ошибка чтения имени файла");
        free(name);
        return -1;
    }
    name[strcspn(name, "\r\n")] = '\0';
    int fd = open(name, O_RDONLY);
    free(name);
    if (fd == -1) {
        perror("Ошибка открытия входного файла");
        return -1;
    }
    return fd;
}

static int create_mapping(struct mailbox **box)
{
    char path[] = "/tmp/XXXXXX";
    int fd = mkstemp(path);
    if (fd == -1) {
        perror("Ошибка создания временного файла");
        return -1;
    }
    if (unlink(path) == -1) {
        perror("Ошибка удаления имени временного файла");
        close_checked(fd);
        return -1;
    }
    if (ftruncate(fd, (off_t)sizeof(**box)) == -1) {
        perror("Ошибка изменения размера временного файла");
        close_checked(fd);
        return -1;
    }
    *box = mmap(NULL, sizeof(**box), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*box =("Ошибка отображения файла у родителя");
        close_= MAP_FAILED) {
        perrorchecked(fd);
        return -1;
    }
    memset(*box, 0, sizeof(**box));
    return fd;
}

static void start_child(int input, int mapping_fd, pid_t parent)
{
    if (dup2(input, STDIN_FILENO) == -1) {
        perror("Ошибка подключения входного файла");
        _exit(EXIT_FAILURE);
    }
    if (input != STDIN_FILENO && close_checked(input) == -1)
        _exit(EXIT_FAILURE);
    char fd_text[32], pid_text[32];
    int a = snprintf(fd_text, sizeof(fd_text), "%d", mapping_fd);
    int b = snprintf(pid_text, sizeof(pid_text), "%ld", (long)parent);
    if (a < 0 || (size_t)a >= sizeof(fd_text) ||
        b < 0 || (size_t)b >= sizeof(pid_text)) {
        fprintf(stderr, "Ошибка подготовки аргументов дочернего процесса\n");
        _exit(EXIT_FAILURE);
    }
    execl("./child", "child", fd_text, pid_text, (char *)NULL);
    perror("Ошибка запуска дочерней программы");
    _exit(EXIT_FAILURE);
}

static int receive_results(struct mailbox *box, pid_t child)
{
    for (;;) { //бесконечный цикл пока ребенок работает
        if (wait_signal(SIGUSR1, child, 1) == -1)
            return -1;
        enum message_kind kind = box->kind;
        if (kind == RESULT) {
            if (printf("%.9g\n", (double)box->sum) < 0 || fflush(stdout) == EOF) {
                perror("Ошибка вывода результата");
                return -1;
            }
        } else if (kind == FAILURE) {
            fprintf(stderr, "[родитель PID=%ld] строка=%zu: %.*s\n",
                    (long)getpid(), box->line,
                    (int)sizeof(box->error), box->error);
        }
        if (notify_peer(child, SIGUSR2) == -1)
            return -1;
        if (kind == FINISHED)
            return 0;
        if (kind == FAILURE)
            return 1;
    }
}

int main(void)
{
    int input = open_input();
    if (input == -1)
        return EXIT_FAILURE;
    struct mailbox *box;
    int mapping_fd = create_mapping(&box);
    if (mapping_fd == -1) {
        close_checked(input);
        return EXIT_FAILURE;
    }
    if (prepare_signals() == -1) {
        close_checked(input);
        close_checked(mapping_fd);
        unmap_checked(box);
        return EXIT_FAILURE;
    }
    pid_t parent = getpid();
    pid_t child = fork();
    if (child == -1) {
        perror("Ошибка создания дочернего процесса");
        close_checked(input);
        close_checked(mapping_fd);
        unmap_checked(box);
        return EXIT_FAILURE;
    }
    if (child == 0)
        start_child(input, mapping_fd, parent);
    int failed = 0;
    if (close_checked(input) == -1)
        failed = 1;
    if (close_checked(mapping_fd) == -1)
        failed = 1;
    int received = failed ? -1 : receive_results(box, child);
    int child_status = reap_child(child, received == -1);
    if (unmap_checked(box) == -1)
        failed = 1;
    return failed || received != 0 || child_status != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
