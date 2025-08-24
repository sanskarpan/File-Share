.PHONY: all build clean test install run debug release performance docs

# Default target
all: build

# Build configurations
build:
	mkdir -p build
	cd build && cmake .. && make -j$(shell nproc)

debug:
	mkdir -p build-debug
	cd build-debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(shell nproc)

release:
	mkdir -p build-release
	cd build-release && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(shell nproc)

# Testing
test: build
	cd build && make test

test-verbose: build
	cd build && ctest --verbose

performance: build
	cd build && make perf-test

coverage: debug
	cd build-debug && cmake -DENABLE_COVERAGE=ON .. && make coverage

# Memory testing
memcheck: debug
	cd build-debug && valgrind --leak-check=full --show-leak-kinds=all ./p2p-share

# Installation
install: release
	cd build-release && sudo make install

uninstall:
	sudo rm -f /usr/local/bin/p2p-share
	sudo rm -rf /usr/local/share/p2p-share

# Package creation
package: release
	cd build-release && make package

deb: release
	cd build-release && cpack -G DEB

# Development utilities
clean:
	rm -rf build* *.log downloads/* shared/* .coverage

format:
	find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i

lint:
	find src include -name "*.cpp" -o -name "*.h" | xargs cpplint

docs:
	doxygen Doxyfile

# Runtime
run: build
	cd build && ./p2p-share

run-peer1: build
	cd build && ./p2p-share -p 8888

run-peer2: build
	cd build && ./p2p-share -p 8889

run-peer3: build
	cd build && ./p2p-share -p 8890

# Development setup
setup-dev:
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libssl-dev libreadline-dev
	sudo apt-get install -y libgtest-dev valgrind clang-format cpplint
	sudo apt-get install -y doxygen graphviz

# Network testing
network-test: build
	@echo "Starting network test with 3 peers..."
	cd build && (./p2p-share -p 8888 &) && (./p2p-share -p 8889 &) && (./p2p-share -p 8890 &)
	sleep 30
	pkill p2p-share

# Benchmark
benchmark: release
	cd build-release && echo "test file content" > shared/test.txt
	cd build-release && (./p2p-share -p 8888 &)
	sleep 2
	cd build-release && time ./p2p-share -p 8889 --benchmark --duration 60
	pkill p2p-share
