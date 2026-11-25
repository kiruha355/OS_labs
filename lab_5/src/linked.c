#include "commands.h"
#include "contracts.h"

#include <stddef.h>
#include <stdio.h>

int main(void)
{
    struct contracts functions = { SinIntegral, translation };
    puts("Подключение библиотек реализации 1");
    return run_commands(&functions, NULL); //нельзя переключить a
}
