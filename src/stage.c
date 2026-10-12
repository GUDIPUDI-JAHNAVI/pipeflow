#include "pipeflow.h"

#include <stdio.h>
#include <unistd.h>

static void redirect_fd(int old_fd, int new_fd)
{
    if (old_fd != new_fd && dup2(old_fd, new_fd) == -1) {
        perror("dup2");
        _exit(126);
    }
}

void run_stage_child(
    const Pipeline *pipeline,
    size_t stage_index,
    int pipes[][2],
    size_t pipe_count,
    int input_fd,
    int output_fd)
{
    if (stage_index == 0 && input_fd >= 0) {
        redirect_fd(input_fd, STDIN_FILENO);
    } else if (stage_index > 0) {
        redirect_fd(pipes[stage_index - 1][0], STDIN_FILENO);
    }

    if (stage_index + 1 == pipeline->stage_count &&
        output_fd >= 0) {
        redirect_fd(output_fd, STDOUT_FILENO);
    } else if (stage_index + 1 < pipeline->stage_count) {
        redirect_fd(pipes[stage_index][1], STDOUT_FILENO);
    }

    close_pipes(pipes, pipe_count);

    if (input_fd >= 0) {
        close(input_fd);
    }

    if (output_fd >= 0) {
        close(output_fd);
    }

    execvp(pipeline->stages[stage_index].argv[0],
           pipeline->stages[stage_index].argv);

    perror(pipeline->stages[stage_index].argv[0]);
    _exit(127);
}
