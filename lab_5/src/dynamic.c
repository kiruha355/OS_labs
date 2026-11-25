#include "commands.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const library_paths[] = { "./libfirst.so", "./libsecond.so" };
static const char *const library_labels[] = {
    "Библиотека 1. Прямоугольники, двоичная система.",
    "Библиотека 2. Трапеции, троичная система."
};
static void *handle;
static int current;

static void *find_function(void *library, const char *name)
{
    void *address = dlsym(library, name);
    const char *error = dlerror();
    if (error != NULL || address == NULL) {
        fprintf(stderr, "Не удалось найти функцию %s. %s\n",
                name, error != NULL ? error : "Получен пустой адрес.");
        return NULL;
    }
    return address;
}

static int close_library(void *library)
{
    if (dlclose(library) == 0)
        return 0;
    fprintf(stderr, "Не удалось закрыть библиотеку. %s\n", dlerror());
    return -1;
}

static int load_library(int index, struct contracts *functions)
{
    void *library = dlopen(library_paths[index], RTLD_NOW | RTLD_LOCAL);
    if (library == NULL) {
        fprintf(stderr, "Не удалось открыть %s. %s\n", library_paths[index], dlerror());
        return -1;
    }

    void *integral = find_function(library, "SinIntegral");
    void *translate = find_function(library, "translation");
    if (integral == NULL || translate == NULL) {
        close_library(library);
        return -1;
    }

    if (handle != NULL && close_library(handle) == -1) {
        close_library(library);
        return -1;
    }

    memcpy(&functions->integral, &integral, sizeof(integral));
    memcpy(&functions->translate, &translate, sizeof(translate));

    handle = library;
    current = index;
    puts(library_labels[current]);
    return 0;
}

static void switch_library(struct contracts *functions)
{
    load_library(1 - current, functions);
}

int main(void)
{
    struct contracts functions;
    if (load_library(0, &functions) == -1)
        return EXIT_FAILURE;
    int status = run_commands(&functions, switch_library);
    if (close_library(handle) == -1)
        return EXIT_FAILURE;
    return status;
}
