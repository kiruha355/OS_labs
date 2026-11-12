#include "ipc.h"

#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int close_checked(int fd)
{
    if (close(fd) == -1) {
        perror("Ошибка закрытия файла");
        return -1;
    }
    return 0;
}

int unmap_checked(struct mailbox *box)
{
    if (munmap(box, sizeof(*box)) == -1) {
        perror("Ошибка снятия отображения");
        return -1;
    }
    return 0;
}

static int make_wait_set(sigset_t *set)
{
    if (sigemptyset(set) == -1 ||   
        sigaddset(set, SIGUSR1) == -1 || //ребенок родителю
        sigaddset(set, SIGUSR2) == -1 ||
        sigaddset(set, SIGTERM) == -1) { //завершение процесса
        perror("Ошибка подготовки набора сигналов");
        return -1;
    }
    return 0;
}

int prepare_signals(void)
{
    sigset_t set;
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    if (sigemptyset(&action.sa_mask) == -1 ||
        sigaction(SIGPIPE, &action, NULL) == -1) {
        perror("Ошибка настройки сигнала SIGPIPE");
        return -1;
    }
    if (make_wait_set(&set) == -1)
        return -1;
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("Ошибка блокировки сигналов");
        return -1;
    }
    return 0;
}

int notify_peer(pid_t peer, int signal_number)
{
    if (kill(peer, signal_number) == -1) {
        perror("Ошибка отправки сигнала");
        return -1;
    }
    return 0;
}

static int check_peer(pid_t peer, int parent_side)
{
    if (parent_side) {
        siginfo_t state = {0};

        //проверить завершение ребёнка
        //вернуться сразу, если ребёнок ещё работает
        if (waitid(P_PID, (id_t)peer, &state,
                WEXITED | WNOHANG ) == -1) {
            if (errno == EINTR)
                return 0;
            perror("Ошибка проверки дочернего процесса");
            return -1;
        }

        if (state.si_pid == peer) {
            fprintf(stderr, "[родитель PID=%ld] дочерний процесс %ld завершился до отправки сообщения\n",
                    (long)getpid(), (long)peer);
            return -1;
        }
    } else if (getppid() != peer) {
        fprintf(stderr, "[ребёнок PID=%ld] родительский процесс завершился\n",
                (long)getpid());
        return -1;
    }

    return 0;
}

int wait_signal(int expected, pid_t peer, int parent_side)
{
    sigset_t set;
    if (make_wait_set(&set) == -1)
        return -1;
    for (;;) {
        const struct timespec timeout = { 1, 0 };
        siginfo_t info;
        int signal_number = sigtimedwait(&set, &info, &timeout);
        if (signal_number >= 0) {
            if (signal_number == SIGTERM) {
                fprintf(stderr, "[PID=%ld] прерван сигналом %d\n",
                        (long)getpid(), signal_number);
                return -1;
            }
            if (signal_number == expected && info.si_pid == peer)
                return 0;
            continue;
        }
        
        if (errno != EAGAIN) {
            perror("Ошибка ожидания сигнала");
            return -1;
        }

        if (check_peer(peer, parent_side) == -1)
            return -1;
    }
}

int reap_child(pid_t child, int terminate)
{
    if (terminate && kill(child, SIGKILL) == -1 && errno != ESRCH) {
        perror("Ошибка завершения дочернего процесса");
    }
    int status;
    pid_t result;
    do {
        result = waitpid(child, &status, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        perror("Ошибка ожидания дочернего процесса");
        return -1;
    }
    if (WIFEXITED(status)) // ребёнок завершился обычным способом
        return WEXITSTATUS(status); // получить код завершения
    if (WIFSIGNALED(status)) // ребёнок завершился из-за сигнала
        fprintf(stderr, "[родитель PID=%ld] дочерний процесс=%ld завершён сигналом=%d\n",
                (long)getpid(), (long)child, WTERMSIG(status)); // номер сигнала
    return -1;
}
