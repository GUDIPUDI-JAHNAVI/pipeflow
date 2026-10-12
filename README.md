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
