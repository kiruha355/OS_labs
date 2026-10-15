#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void close_resources(int file, int read_end, int write_end) {
    if (file != -1) {
        close(file);
    }
    if (read_end != -1) {
        close(read_end);
    }
    if (write_end != -1) {
        close(write_end);
    }
}


void pause_between_checks(void) {
    struct timespec remaining = {0, 10000000L};//10мс

    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR) {
            perror("nanosleep");
            break;
        }
    }
}

int main(void) {
    char *filename = NULL;
    size_t capacity = 0;

    printf("File name: ");
    fflush(stdout);

    if (getline(&filename, &capacity, stdin) == -1) {
        fprintf(stderr, "Ошибка ввода имени файла\n");
        free(filename);
        return 1;
    }

    filename[strcspn(filename, "\n")] = '\0';

    int file = open(filename, O_RDONLY);
    free(filename);

    if (file == -1) {
        perror("open");
        return 1;
    }

    int channel[2] = {-1, -1};

    int result = pipe(channel);

    if (result == -1) {
        perror("pipe");
        close_resources(file, -1, -1);
        return 1;
    }

    int flags = fcntl(channel[0], F_GETFL);

    flags = flags | O_NONBLOCK; //чтобы не ждать отевта от ребенка 
    fcntl(channel[0], F_SETFL, flags);


    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        close_resources(file, channel[0], channel[1]);
        return 1;
    }

    if (pid == 0) {
        close(channel[0]);

        if (dup2(file, STDIN_FILENO) == -1 || dup2(channel[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }

        close_resources(file, -1, channel[1]);

        execl("./child", "child", (char *)NULL);

        perror("execl");
        _exit(1);
    }

    close_resources(file, -1, channel[1]);

    char buffer[4096];

    while (1) {
        ssize_t count = read(channel[0], buffer, sizeof(buffer));

        if (count > 0) {
            fwrite(buffer, 1, (size_t)count, stdout);
            fflush(stdout);
        } else if (count == 0) {
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            pause_between_checks();
        } else {
            perror("read");
            close_resources(-1, channel[0], -1);
            return 1;
        }
    }

    close_resources(-1, channel[0], -1);

    int status;

    while (1) {
        pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == pid) {
            break;
        }

        if (result == -1) {
            perror("waitpid");
            return 1;
        }

        pause_between_checks();
    }

    if (!WIFEXITED(status)) {
        return 1;
    }

    return WEXITSTATUS(status);
}
