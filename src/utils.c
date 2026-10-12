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
