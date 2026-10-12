#include "pipeflow.h"

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>

int wait_for_workers(const pid_t *pids, size_t count)
{
    int overall_status = 0;

    for (size_t i = 0; i < count; i++) {
        int status;
        pid_t result;

        do {
            result = waitpid(pids[i], &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid");
            overall_status = 1;
            continue;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);

            if (exit_code != 0) {
                fprintf(stderr,
                        "PipeFlow: stage %zu failed with exit code %d\n",
                        i + 1, exit_code);
                overall_status = 1;
            }
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr,
                    "PipeFlow: stage %zu terminated by signal %d\n",
                    i + 1, WTERMSIG(status));
            overall_status = 1;
        }
    }

    return overall_status;
}
