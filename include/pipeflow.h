#ifndef PIPEFLOW_H
#define PIPEFLOW_H

#include <stddef.h>
#include <sys/types.h>

#define PIPEFLOW_MAX_STAGES 32
#define PIPEFLOW_MAX_ARGS 64

typedef struct {
    char *argv[PIPEFLOW_MAX_ARGS + 1];
} Stage;

typedef struct {
    Stage stages[PIPEFLOW_MAX_STAGES];
    size_t stage_count;
    const char *input_path;
    const char *output_path;
} Pipeline;

int parse_arguments(int argc, char **argv, Pipeline *pipeline);
int run_pipeline(const Pipeline *pipeline);
int create_pipes(int pipes[][2], size_t pipe_count);
void close_pipes(int pipes[][2], size_t pipe_count);

void run_stage_child(
    const Pipeline *pipeline,
    size_t stage_index,
    int pipes[][2],
    size_t pipe_count,
    int input_fd,
    int output_fd
);

int wait_for_workers(const pid_t *pids, size_t count);
void print_usage(const char *program_name);

#endif
