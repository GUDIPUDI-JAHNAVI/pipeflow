# PipeFlow - A Dataflow Stream Engine

PipeFlow is a process-based pipeline runner written in C for Linux.
It connects external commands so that the output of one stage becomes
the input of the next stage.

## Features

- Create child processes using `fork()`.
- Connect commands using Unix pipes.
- Redirect standard input and output using `dup2()`.
- Execute external programs using `execvp()`.
- Synchronize child processes using `waitpid()`.
- Read pipeline input from a file with `-i`.
- Write pipeline output to a file with `-o`.
- Validate pipeline stages and command arguments.
- Handle pipe creation failures and close file descriptors.
- Include a bounded-buffer component with FIFO and concurrency tests.
- Include integration and stress tests.

## Requirements

- Linux
- GCC
- GNU Make
- POSIX-compatible system interfaces

## Build

Compile the project:

    make

Remove generated build files:

    make clean

## Usage

General syntax:

    ./build/pipeflow [-i input_file] [-o output_file] -- command1 ::: command2

The `--` separates PipeFlow options from the pipeline commands.
Use `:::` to separate consecutive pipeline stages.

### Examples

Run a two-stage pipeline:

    ./build/pipeflow -- cat ::: wc -l

Read input from a file:

    ./build/pipeflow -i examples/input.txt -- grep ERROR ::: tr a-z A-Z

Save pipeline output to a file:

    ./build/pipeflow -i examples/input.txt -o build/output.txt -- grep ERROR ::: tr a-z A-Z

## Testing

Run the automated test suite:

    make test

Run the stress tests:

    ./tests/test_stress.sh

The tests cover pipeline parsing and validation, bounded-buffer FIFO
behavior, concurrent buffer access, pipeline execution, file handling,
command failures, nonzero exit statuses, and signal-related failures.

## Project Structure

    include/
        bounded_buffer.h
        pipeflow.h

    src/
        main.c
        pipeline.c
        stage.c
        buffer.c
        bounded_buffer.c
        utils.c
        worker.c

    tests/
        test_pipeline.c
        test_buffer.c
        test_stress.sh

    examples/
        input.txt

## Limitations

- Pipeline stages are external Linux commands.
- Shell quoting, wildcard expansion, and shell operators are not
  interpreted by a shell.
- Commands and arguments are passed directly to external programs.
- The bounded-buffer component is tested separately; it is not
  described as an integrated queue between pipeline stages.
- This project does not currently provide a custom plugin API or
  a thread pool.

## Implementation Notes

PipeFlow creates the pipes needed to connect pipeline stages,
starts child processes, redirects their standard input and output,
and waits for the child processes to finish.

Pipe creation and cleanup include error handling. The test suite
checks important parsing, execution, buffer, and failure scenarios.
