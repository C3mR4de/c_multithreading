#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <threads.h>
#include <errno.h>

mtx_t mutex;
cnd_t cond_numbers;
cnd_t cond_letters;

enum Turn
{
    NUMBERS = 0,
    LETTERS
}
turn = NUMBERS;

bool numbers_finished = false;
bool letters_finished = false;

int print_letters(const char count)
{
    for (char i = 'A'; i < 'A' + count; ++i)
    {
        mtx_lock(&mutex);

        while (turn != LETTERS && !numbers_finished)
        {
            cnd_wait(&cond_letters, &mutex);
        }

        printf("%c ", i);

        if (!numbers_finished)
        {
            turn = NUMBERS;
            cnd_signal(&cond_numbers);
        }

        mtx_unlock(&mutex);
    }

    letters_finished = true;
    return EXIT_SUCCESS;
}

int print_numbers(const size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        mtx_lock(&mutex);

        while (turn != NUMBERS && !letters_finished)
        {
            cnd_wait(&cond_numbers, &mutex);
        }

        printf("%zu ", i);

        if (!letters_finished)
        {
            turn = LETTERS;
            cnd_signal(&cond_letters);
        }

        mtx_unlock(&mutex);
    }

    numbers_finished = true;
    return EXIT_SUCCESS;
}

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char* const argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "error: wrong arguments count!\n");
        return EXIT_FAILURE;
    }

    char* end[1] = {nullptr};

    const size_t numbers_count = strtoul(argv[1], end, 10);

    if (errno == ERANGE)
    {
        errno = 0;
        fprintf(stderr, "error: wrong numbers count!\n");
        return EXIT_FAILURE;
    }

    const size_t letters_count = strtoul(argv[2], end, 10);

    if (errno == ERANGE || letters_count > 26)
    {
        errno = 0;
        fprintf(stderr, "error: wrong letters count!\n");
        return EXIT_FAILURE;
    }

    thrd_t numbers_thread;
    thrd_t letters_thread;

    int numbers_status = thrd_create(&numbers_thread, (thrd_start_t)print_numbers, (void*)numbers_count);
    int letters_status = thrd_create(&letters_thread, (thrd_start_t)print_letters, (void*)letters_count);

    if (numbers_status != thrd_success || letters_status != thrd_success)
    {
        fprintf(stderr, "error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    thrd_join(numbers_thread, nullptr);
    thrd_join(letters_thread, nullptr);

    puts("");

    return EXIT_SUCCESS;
}
