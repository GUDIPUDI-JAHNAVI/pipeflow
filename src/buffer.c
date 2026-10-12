#include "pipeflow.h"

#include <stdio.h>
#include <unistd.h>

int create_pipes(int pipes[][2], size_t pipe_count)
{
    size_t created = 0;

    for (size_t i = 0; i < pipe_count; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");

            for (size_t j = 0; j < created; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return -1;
        }

        created++;
    }

    return 0;
}

void close_pipes(int pipes[][2], size_t pipe_count)
{
    for (size_t i = 0; i < pipe_count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}
