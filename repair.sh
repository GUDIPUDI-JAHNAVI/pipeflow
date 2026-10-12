#!/bin/bash
set -eu

mkdir -p include src tests examples docs build

cat > include/pipeflow.h <<'EOF'
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
EOF

cat > src/main.c <<'EOF'
#include "pipeflow.h"

#include <string.h>

int main(int argc, char **argv)
{
    Pipeline pipeline = {0};

    if (argc == 2 &&
        (strcmp(argv[1], "-h") == 0 ||
         strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }

    if (parse_arguments(argc, argv, &pipeline) != 0) {
        print_usage(argv[0]);
        return 2;
    }

    return run_pipeline(&pipeline);
}
EOF

cat > src/utils.c <<'EOF'
#include "pipeflow.h"

#include <stdio.h>

void print_usage(const char *program_name)
{
    fprintf(stderr,
        "PipeFlow - A Dataflow Stream Engine\n\n"
        "Usage:\n"
        "  %s [-i INPUT] [-o OUTPUT] -- COMMAND [ARGS...] "
        "[::: COMMAND [ARGS...]]...\n\n"
        "Options:\n"
        "  -i INPUT   Read from an input file\n"
        "  -o OUTPUT  Write to an output file\n"
        "  -h, --help Show this help\n\n"
        "Examples:\n"
        "  %s -- cat ::: wc -l\n"
        "  %s -i examples/input.txt -- grep ERROR ::: "
        "tr a-z A-Z\n",
        program_name, program_name, program_name);
}
EOF

cat > src/buffer.c <<'EOF'
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
EOF

cat > src/stage.c <<'EOF'
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
EOF

cat > src/worker.c <<'EOF'
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

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "PipeFlow: stage %zu failed\n", i + 1);
            overall_status = 1;
        }
    }

    return overall_status;
}
EOF

cat > src/pipeline.c <<'EOF'
#include "pipeflow.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int parse_arguments(int argc, char **argv, Pipeline *pipeline)
{
    int index = 1;

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
    int pipes[PIPEFLOW_MAX_STAGES - 1][2];
    pid_t pids[PIPEFLOW_MAX_STAGES];
    size_t pipe_count = pipeline->stage_count - 1;
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
            if (input_fd >= 0) close(input_fd);
            return 1;
        }
    }

    if (create_pipes(pipes, pipe_count) == -1) {
        if (input_fd >= 0) close(input_fd);
        if (output_fd >= 0) close(output_fd);
        return 1;
    }

    size_t launched = 0;

    for (size_t i = 0; i < pipeline->stage_count; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            close_pipes(pipes, pipe_count);
            if (input_fd >= 0) close(input_fd);
            if (output_fd >= 0) close(output_fd);
            wait_for_workers(pids, launched);
            return 1;
        }

        if (pid == 0) {
            run_stage_child(pipeline, i, pipes, pipe_count,
                            input_fd, output_fd);
        }

        pids[launched++] = pid;
    }

    close_pipes(pipes, pipe_count);
    if (input_fd >= 0) close(input_fd);
    if (output_fd >= 0) close(output_fd);

    return wait_for_workers(pids, launched);
}
EOF

cat > tests/test_pipeline.c <<'EOF'
#include "pipeflow.h"

#include <stdio.h>
#include <string.h>

static int test_two_stages(void)
{
    char *args[] = {
        "pipeflow", "--", "grep", "ERROR",
        ":::", "tr", "a-z", "A-Z", NULL
    };
    Pipeline p = {0};

    if (parse_arguments(8, args, &p) != 0 || p.stage_count != 2)
        return 1;

    if (strcmp(p.stages[0].argv[0], "grep") != 0 ||
        strcmp(p.stages[1].argv[0], "tr") != 0)
        return 1;

    return 0;
}

static int test_file_options(void)
{
    char *args[] = {
        "pipeflow", "-i", "input.txt", "-o", "output.txt",
        "--", "cat", NULL
    };
    Pipeline p = {0};

    if (parse_arguments(7, args, &p) != 0)
        return 1;

    return p.stage_count != 1 ||
           strcmp(p.input_path, "input.txt") != 0 ||
           strcmp(p.output_path, "output.txt") != 0;
}

static int test_empty_stage(void)
{
    char *args[] = {
        "pipeflow", "--", "cat", ":::", ":::", NULL
    };
    Pipeline p = {0};

    return parse_arguments(5, args, &p) == 0;
}

int main(void)
{
    int failures = 0;

    if (test_two_stages() == 0)
        puts("PASS: two-stage argument parsing");
    else {
        puts("FAIL: two-stage argument parsing");
        failures++;
    }

    if (test_file_options() == 0)
        puts("PASS: input/output file options");
    else {
        puts("FAIL: input/output file options");
        failures++;
    }

    if (test_empty_stage() == 0)
        puts("PASS: invalid empty stage rejected");
    else {
        puts("FAIL: invalid empty stage rejected");
        failures++;
    }

    return failures == 0 ? 0 : 1;
}
EOF

cat > examples/input.txt <<'EOF'
INFO service started
ERROR disk full
INFO retrying connection
ERROR network down
INFO service stopped
EOF

cat > README.md <<'EOF'
# PipeFlow - A Dataflow Stream Engine

PipeFlow is a process-based C pipeline runner for Linux.

## Concepts demonstrated

- Process creation with fork()
- Inter-process communication with pipe()
- File descriptor redirection with dup2()
- Program execution with execvp()
- Child-process synchronization with waitpid()
- Input/output file handling
- Error handling and automated tests

## Build

    make

## Test

    make test

## Examples

    ./build/pipeflow -- cat ::: wc -l

    ./build/pipeflow -i examples/input.txt \
        -- grep ERROR ::: tr a-z A-Z

    ./build/pipeflow -i examples/input.txt \
        -o build/output.txt -- grep ERROR ::: tr a-z A-Z

## Limitations

Stages are external Linux commands. Shell quoting and shell operators
are not interpreted. This initial version does not include a custom
plugin API, a thread pool, or an application-managed bounded queue.
EOF

cat > Makefile <<'EOF'
CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

SOURCES = src/main.c src/pipeline.c src/stage.c \
          src/buffer.c src/worker.c src/utils.c

TARGET = build/pipeflow
TEST_TARGET = build/test_pipeline

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES) include/pipeflow.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(TEST_TARGET): tests/test_pipeline.c src/pipeline.c \
                src/utils.c include/pipeflow.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_pipeline.c \
		src/pipeline.c src/utils.c -o $(TEST_TARGET)

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)
	printf 'INFO started\nERROR disk full\nINFO retrying\nERROR network down\n' \
		| ./$(TARGET) -- grep ERROR ::: tr a-z A-Z \
		> build/test_output.txt
	grep -q '^ERROR DISK FULL$$' build/test_output.txt
	grep -q '^ERROR NETWORK DOWN$$' build/test_output.txt
	! grep -q '^INFO' build/test_output.txt
	@echo "All PipeFlow tests passed."

clean:
	rm -f $(TARGET) $(TEST_TARGET) build/test_output.txt
EOF

echo "PipeFlow source files have been recreated."
