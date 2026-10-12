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

$(TEST_TARGET): tests/test_pipeline.c $(SOURCES) include/pipeflow.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_pipeline.c \
		src/pipeline.c src/stage.c src/buffer.c \
		src/worker.c src/utils.c -o $(TEST_TARGET)

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)
	printf 'INFO started\nERROR disk full\nINFO retrying\nERROR network down\n' \
		| ./$(TARGET) -- grep ERROR ::: tr a-z A-Z \
		> build/test_output.txt
	grep -q '^ERROR DISK FULL$$' build/test_output.txt
	grep -q '^ERROR NETWORK DOWN$$' build/test_output.txt
	! grep -q '^INFO' build/test_output.txt
	! ./build/pipeflow -- nonexistent_command >/dev/null 2>&1
	! ./build/pipeflow -i missing_input.txt -- cat >/dev/null 2>&1
	! ./build/pipeflow -- echo hello ::: nonexistent_command ::: wc -l >/dev/null 2>&1
	! ./build/pipeflow -- sh -c 'exit 7' >/dev/null 2>&1
	! ./build/pipeflow -- sh -c 'kill -TERM $$$$' >/dev/null 2>&1
	! ./build/pipeflow -- sh -c 'exit 0' ::: sh -c 'exit 7' >/dev/null 2>&1
	@echo "All PipeFlow tests passed."

clean:
	rm -f $(TARGET) $(TEST_TARGET) build/test_output.txt
