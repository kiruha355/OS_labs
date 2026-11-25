#ifndef COMMANDS_H
#define COMMANDS_H

struct contracts {
    float (*integral)(float, float, float);
    char *(*translate)(long);
};

int run_commands(struct contracts *functions, void (*switch_library)(struct contracts *));

#endif
