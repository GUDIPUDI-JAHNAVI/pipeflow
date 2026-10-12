CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -pthread

SOURCES = src/main.c src/pipeline.c src/stage.c \
          src/buffer.c src/worker.c src/utils.c

TARGET = build/pipeflow
PIPELINE_TEST = build/test_pipeline
BUFFER_TEST = build/test_buffer

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES) include/pipeflow.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(PIPELINE_TEST): tests/test_pipeline.c $(SOURCES) include/pipeflow.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_pipeline.c \
		src/pipeline.c src/stage.c src/buffer.c \
		src/worker.c src/utils.c -o $(PIPELINE_TEST)

$(BUFFER_TEST): tests/test_buffer.c src/bounded_buffer.c include/bounded_buffer.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_buffer.c \
		src/bounded_buffer.c -o $(BUFFER_TEST)

test: $(TARGET) $(PIPELINE_TEST) $(BUFFER_TEST)
	./$(PIPELINE_TEST)
	./$(BUFFER_TEST)
	printf 'INFO started\nERROR disk full\nINFO retrying\nERROR network down\n' | ./$(TARGET) -- grep ERROR ::: tr a-z A-Z > build/test_output.txt
	grep -q '^ERROR DISK FULL$$' build/test_output.txt
	grep -q '^ERROR NETWORK DOWN$$' build/test_output.txt
	! grep -q '^INFO' build/test_output.txt
	! ./$(TARGET) -- nonexistent_command >/dev/null 2>&1
	! ./$(TARGET) -i missing_input.txt -- cat >/dev/null 2>&1
	! ./$(TARGET) -- echo hello ::: nonexistent_command ::: wc -l >/dev/null 2>&1
	! ./$(TARGET) -- sh -c 'exit 7' >/dev/null 2>&1
	! ./$(TARGET) -- sh -c 'kill -TERM $$$$' >/dev/null 2>&1
	! ./$(TARGET) -- sh -c 'exit 0' ::: sh -c 'exit 7' >/dev/null 2>&1
	@echo "All PipeFlow tests passed."

clean:
	rm -f $(TARGET) $(PIPELINE_TEST) $(BUFFER_TEST) build/test_output.txt
