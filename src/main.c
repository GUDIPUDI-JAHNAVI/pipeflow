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
