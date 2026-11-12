#ifndef LAB4_IPC_H
#define LAB4_IPC_H

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>

enum message_kind { RESULT = 1, FINISHED, FAILURE };


struct mailbox {
    enum message_kind kind;
    size_t line;
    float sum;
    char error[192];
};

int prepare_signals(void);
int wait_signal(int expected, pid_t peer, int parent_side);
int notify_peer(pid_t peer, int signal_number);
int close_checked(int fd);
int unmap_checked(struct mailbox *box);
int reap_child(pid_t child, int terminate);

#endif
