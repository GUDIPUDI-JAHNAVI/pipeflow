
#include "pipeflow.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int parse_arguments(int argc, char **argv, Pipeline *pipeline)
{
    int index = 1;

    if (pipeline == NULL || argc < 1 || argv == NULL) {
        fprintf(stderr, "PipeFlow: invalid arguments\n");
        return -1;
    }

    pipeline->stage_count = 0;
    pipeline->input_path = NULL;
    pipeline->output_path = NULL;

    while (index < argc && strcmp(argv[index], "--") != 0) {
        if (strcmp(argv[index], "-i") == 0 ||
            strcmp(argv[index], "-o") == 0) {
            int is_input = strcmp(argv[index], "-i") == 0;

            if (index + 1 >= argc ||
                strcmp(argv[index + 1], "--") == 0) {
                fprintf(stderr, "PipeFlow: missing file path\n");
                return -1;
            }

            if (is_input) {
                if (pipeline->input_path != NULL) {
                    fprintf(stderr, "PipeFlow: duplicate input option\n");
                    return -1;
                }

                pipeline->input_path = argv[index + 1];
            } else {
                if (pipeline->output_path != NULL) {
                    fprintf(stderr, "PipeFlow: duplicate output option\n");
                    return -1;
                }

                pipeline->output_path = argv[index + 1];
            }

            index += 2;
        } else {
            fprintf(stderr, "PipeFlow: unknown option '%s'\n",
                    argv[index]);
            return -1;
        }
    }

    if (index >= argc || strcmp(argv[index], "--") != 0) {
        fprintf(stderr, "PipeFlow: expected -- before commands\n");
        return -1;
    }

    index++;

    while (index < argc) {
        if (pipeline->stage_count >= PIPEFLOW_MAX_STAGES) {
            fprintf(stderr, "PipeFlow: too many stages\n");
            return -1;
        }

        Stage *stage = &pipeline->stages[pipeline->stage_count];
        size_t arg_count = 0;

        while (index < argc && strcmp(argv[index], ":::") != 0) {
            if (arg_count >= PIPEFLOW_MAX_ARGS) {
                fprintf(stderr, "PipeFlow: too many arguments\n");
                return -1;
            }

            stage->argv[arg_count++] = argv[index++];
        }

        if (arg_count == 0) {
            fprintf(stderr, "PipeFlow: empty processing stage\n");
            return -1;
        }

        stage->argv[arg_count] = NULL;
        pipeline->stage_count++;

        if (index < argc && strcmp(argv[index], ":::") == 0) {
            index++;

            if (index >= argc || strcmp(argv[index], ":::") == 0) {
                fprintf(stderr, "PipeFlow: missing command after :::\n");
                return -1;
            }
        }
    }

    if (pipeline->stage_count == 0) {
        fprintf(stderr, "PipeFlow: no commands specified\n");
        return -1;
    }

    return 0;
}

int run_pipeline(const Pipeline *pipeline)
{
    if (pipeline == NULL ||
        pipeline->stage_count == 0 ||
        pipeline->stage_count > PIPEFLOW_MAX_STAGES) {
        fprintf(stderr, "PipeFlow: invalid pipeline stage count\n");
        return 1;
    }

    int pipes[PIPEFLOW_MAX_STAGES - 1][2];
    pid_t pids[PIPEFLOW_MAX_STAGES];

    size_t pipe_count = pipeline->stage_count - 1;
    size_t launched = 0;

    int input_fd = -1;
    int output_fd = -1;

    if (pipeline->input_path != NULL) {
        input_fd = open(pipeline->input_path, O_RDONLY);

        if (input_fd == -1) {
            perror(pipeline->input_path);
            return 1;
        }
    }

    if (pipeline->output_path != NULL) {
        output_fd = open(pipeline->output_path,
                         O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (output_fd == -1) {
            perror(pipeline->output_path);

            if (input_fd >= 0) {
                close(input_fd);
            }

            return 1;
        }
    }

    if (create_pipes(pipes, pipe_count) == -1) {
        if (input_fd >= 0) {
            close(input_fd);
        }

        if (output_fd >= 0) {
            close(output_fd);
        }

        return 1;
    }

    for (size_t i = 0; i < pipeline->stage_count; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");

            close_pipes(pipes, pipe_count);

            if (input_fd >= 0) {
                close(input_fd);
            }

            if (output_fd >= 0) {
                close(output_fd);
            }

            /*
             * Reap every child already started before returning.
             */
            (void)wait_for_workers(pids, launched);

            return 1;
        }

        if (pid == 0) {
            run_stage_child(pipeline, i, pipes, pipe_count,
                            input_fd, output_fd);
            _exit(127);
        }

        pids[launched++] = pid;
    }

    close_pipes(pipes, pipe_count);

    if (input_fd >= 0) {
        close(input_fd);
    }

    if (output_fd >= 0) {
        close(output_fd);
    }

    return wait_for_workers(pids, launched);
}
