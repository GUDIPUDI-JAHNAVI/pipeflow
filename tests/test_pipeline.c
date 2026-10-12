
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

    if (parse_arguments(8, args, &p) != 0 ||
        p.stage_count != 2) {
        return 1;
    }

    if (strcmp(p.stages[0].argv[0], "grep") != 0 ||
        strcmp(p.stages[1].argv[0], "tr") != 0) {
        return 1;
    }

    return 0;
}

static int test_file_options(void)
{
    char *args[] = {
        "pipeflow", "-i", "input.txt", "-o", "output.txt",
        "--", "cat", NULL
    };

    Pipeline p = {0};

    if (parse_arguments(7, args, &p) != 0) {
        return 1;
    }

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

static int test_zero_stage_pipeline(void)
{
    Pipeline p = {0};

    return run_pipeline(&p) == 1 ? 0 : 1;
}

static int test_too_many_stages(void)
{
    Pipeline p = {0};

    p.stage_count = PIPEFLOW_MAX_STAGES + 1;

    return run_pipeline(&p) == 1 ? 0 : 1;
}

int main(void)
{
    int failures = 0;

    if (test_two_stages() == 0) {
        puts("PASS: two-stage argument parsing");
    } else {
        puts("FAIL: two-stage argument parsing");
        failures++;
    }

    if (test_file_options() == 0) {
        puts("PASS: input/output file options");
    } else {
        puts("FAIL: input/output file options");
        failures++;
    }

    if (test_empty_stage() == 0) {
        puts("PASS: invalid empty stage rejected");
    } else {
        puts("FAIL: invalid empty stage rejected");
        failures++;
    }

    if (test_zero_stage_pipeline() == 0) {
        puts("PASS: zero-stage pipeline rejected");
    } else {
        puts("FAIL: zero-stage pipeline rejected");
        failures++;
    }

    if (test_too_many_stages() == 0) {
        puts("PASS: excessive stage count rejected");
    } else {
        puts("FAIL: excessive stage count rejected");
        failures++;
    }

    if (failures != 0) {
        fprintf(stderr, "%d pipeline test(s) failed\n", failures);
        return 1;
    }

    puts("All pipeline unit tests passed.");
    return 0;
}
