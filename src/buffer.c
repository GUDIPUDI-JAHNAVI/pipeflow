
#include "pipeflow.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int create_pipes(int pipes[][2], size_t pipe_count)
{
    if (pipe_count == 0) {
        return 0;
    }

    if (pipes == NULL || pipe_count > PIPEFLOW_MAX_STAGES - 1) {
        errno = EINVAL;
        perror("PipeFlow: invalid pipe arguments");
        return -1;
    }

    size_t created = 0;

    while (created < pipe_count) {
        if (pipe(pipes[created]) == -1) {
            int saved_errno = errno;
            perror("PipeFlow: pipe");

            for (size_t i = 0; i < created; i++) {
                (void)close(pipes[i][0]);
                (void)close(pipes[i][1]);
                pipes[i][0] = -1;
                pipes[i][1] = -1;
            }

            errno = saved_errno;
            return -1;
        }

        created++;
    }

    return 0;
}

void close_pipes(int pipes[][2], size_t pipe_count)
{
    if (pipes == NULL) {
        return;
    }

    for (size_t i = 0; i < pipe_count; i++) {
        if (pipes[i][0] >= 0) {
            (void)close(pipes[i][0]);
            pipes[i][0] = -1;
        }

        if (pipes[i][1] >= 0) {
            (void)close(pipes[i][1]);
            pipes[i][1] = -1;
        }
    }
}
