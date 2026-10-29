#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int rounds_left;
    int score1;
    int score2;
    int experiments;
    unsigned int random_state;
    int wins1;
    int wins2;
    int draws;
} Worker;

static int parse_number(const char *text, int *value)
{
    if (*text == '\0') {
        return 0;
    }
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
    }

    long parsed = strtol(text, NULL, 10);

    if (parsed > INT_MAX) {
        return 0; 
    }

    *value = (int)parsed;
    return 1;
}

static unsigned int make_state(int seed, int index)
{
    unsigned int x = (unsigned int)seed + ((unsigned int)index + 1U) * 0x9e3779b9U;
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x != 0 ? x : 1;
}

static unsigned int random_number(unsigned int *state)
{
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x * 0x9e3779bbU - 1U;
}

static int roll_die(unsigned int *state)
{
    const unsigned int limit = UINT_MAX - UINT_MAX % 6U;
    unsigned int number;
    do {
        number = random_number(state);
    } while (number >= limit);
    return 1 + (int)(number % 6U);
}

static void *simulate(void *argument)
{
    Worker *worker = argument;
    unsigned int state = worker->random_state;
    int wins1 = 0;
    int wins2 = 0;
    int draws = 0;

    for (int i = 0; i < worker->experiments; ++i) {
        int score1 = worker->score1;
        int score2 = worker->score2;

        for (int round = 0; round < worker->rounds_left; ++round) {
            score1 += roll_die(&state);
            score1 += roll_die(&state);
            score2 += roll_die(&state);
            score2 += roll_die(&state);
        }

        if (score1 > score2) {
            ++wins1;
        } else if (score2 > score1) {
            ++wins2;
        } else {
            ++draws;
        }
    }

    worker->wins1 = wins1;
    worker->wins2 = wins2;
    worker->draws = draws;
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 8 && argc != 10) {
        return EXIT_FAILURE;
    }

    int input[5];
    for (int i = 0; i < 5; ++i) {
        if (!parse_number(argv[i + 1], &input[i])) {
            return EXIT_FAILURE;
        }
    }

    int thread_limit;
    time_t now = time(NULL);
    int seed = now == (time_t)-1 ? 1 : (int)(now % INT_MAX);
    if (strcmp(argv[6], "--threads") != 0 || !parse_number(argv[7], &thread_limit) || thread_limit == 0) {
        return EXIT_FAILURE;
    }
    if (argc == 10 && (strcmp(argv[8], "--seed") != 0 || !parse_number(argv[9], &seed))) {
        return EXIT_FAILURE;
    }

    const int rounds = input[0];
    const int completed = input[1];
    const int experiments = input[4];
    if (rounds == 0 || rounds > INT_MAX / 12 || completed > rounds || experiments == 0) {
        return EXIT_FAILURE;
    }
    if (input[2] < 2 * completed || input[2] > 12 * completed ||
        input[3] < 2 * completed || input[3] > 12 * completed) {
        return EXIT_FAILURE;
    }

    int thread_count = thread_limit < experiments ? thread_limit : experiments;
    if ((size_t)thread_count > (size_t)-1 / sizeof(pthread_t) ||
        (size_t)thread_count > (size_t)-1 / sizeof(Worker)) {
        fprintf(stderr, "Слишком много потоков для памяти.\n");
        return EXIT_FAILURE;
    }
    pthread_t *threads = calloc((size_t)thread_count, sizeof(*threads));
    Worker *workers = calloc((size_t)thread_count, sizeof(*workers));
    if (threads == NULL || workers == NULL) {
        fprintf(stderr, "Ошибка выделения памяти.\n");
        free(threads);
        free(workers);
        return EXIT_FAILURE;
    }

    struct timespec start;
    struct timespec finish;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        free(threads);
        free(workers);
        return EXIT_FAILURE;
    }

    int created = 0;
    for (int i = 0; i < thread_count; ++i) {
        workers[i].rounds_left = rounds - completed;
        workers[i].score1 = input[2];
        workers[i].score2 = input[3];
        workers[i].experiments = experiments / thread_count + (i < experiments % thread_count ? 1 : 0);
        workers[i].random_state = make_state(seed, i);

        int error = pthread_create(&threads[i], NULL, simulate, &workers[i]);
        if (error != 0) {
            break;
        }
        ++created;
    }

    int wins1 = 0;
    int wins2 = 0;
    int draws = 0;

    for (int i = 0; i < created; ++i) {
        int error = pthread_join(threads[i], NULL);

        if (error != 0) {
            fprintf(stderr, "Ошибка ожидания потока (код ошибки: %d)\n",
                    error);
            return EXIT_FAILURE;
        }

        wins1 += workers[i].wins1;
        wins2 += workers[i].wins2;
        draws += workers[i].draws;
    }

    free(threads);
    free(workers);

    if (created != thread_count) {
        return EXIT_FAILURE;
    }


    double elapsed = (double)(finish.tv_sec - start.tv_sec) +(double)(finish.tv_nsec - start.tv_nsec) / 1e9;

    printf("Победы первого игрока: %d (%.4f%%)\n", wins1,
           100.0 * (double)wins1 / (double)experiments);

    printf("Победы второго игрока: %d (%.4f%%)\n", wins2,
           100.0 * (double)wins2 / (double)experiments);

    printf("Ничьи: %d (%.4f%%)\n", draws,
           100.0 * (double)draws / (double)experiments);

    printf("Время выполнения: %.6f с\n", elapsed);

    return EXIT_SUCCESS;
}
