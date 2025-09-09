# P2P File Sharing App 

 **Objective**: Build a high-performance, scalable, command-line peer-to-peer (P2P) file sharing application in C++17 on Linux. The app will use **TCP/IP socket communication** and **POSIX multithreading**, with performance tuning for **200+ concurrent peers**.

## Features

- **High Performance**: Supports 200+ simultaneous peer connections
- **Non-blocking I/O**: Uses epoll for efficient event-driven networking
- **Concurrent Downloads**: Multi-threaded file transfers with progress tracking
- **Resumable Transfers**: Support for interrupted download recovery
- **Interactive CLI**: Full-featured command-line interface
- **Protocol**: Custom binary protocol with CRC32 validation
- **Cross-platform**: Designed for Linux with POSIX compliance

## Tech Stack

- **Language**: C++17
- **Platform**: Linux
- **Networking**: POSIX Sockets, TCP/IP
- **Concurrency**: POSIX Threads (pthreads)
- **Build System**: CMake
- **Testing**: Google Test (GTest)
- **Debugging**: GDB, Valgrind
- **Dependencies**: OpenSSL, readline

## Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake libssl-dev libreadline-dev

# For testing and development
sudo apt-get install libgtest-dev valgrind clang-format
```

### Build

```bash
# Clone repository
git clone https://github.com/sanskarpan/File-Share.git
cd File-share

# Build release version
make release

# Or build debug version
make debug
```

### Usage

1. **Start first peer**:
```bash
cd build-release
./p2p-share -p 8888 -d ./shared1/
```

2. **Start second peer** (in another terminal):
```bash
cd build-release
./p2p-share -p 8889 -d ./shared2/
```

3. **Basic commands**:
```
p2p> help                    # Show all commands
p2p> connect 127.0.0.1 8888  # Connect to another peer
p2p> share /path/to/file.txt # Share a file
p2p> files                   # List all available files
p2p> get filename.txt        # Download a file
p2p> peers                   # Show connected peers
p2p> status                  # Show node status
p2p> exit                    # Exit application
```

## Architecture

### Core Components

- **HighPerformanceServer**: Epoll-based server handling 200+ connections
- **Client**: Manages outgoing connections and downloads
- **PeerManager**: Maintains peer discovery and heartbeat
- **FileManager**: Handles local file scanning and metadata
- **Protocol**: Custom binary protocol for reliable communication
- **ThreadPool**: Manages concurrent operations

### Network Protocol

Custom binary protocol with:
- Message headers with magic numbers and CRC32 validation
- Support for peer discovery, file listing, and chunk transfer
- Error handling and timeout management
- Resumable download support

### Performance Optimizations

- **Non-blocking I/O**: Epoll for handling thousands of connections
- **Custom buffering**: Optimized buffer sizes for network I/O
- **Connection pooling**: Efficient connection reuse
- **Edge-triggered events**: Minimized system calls

## Development

### Project Structure

```
p2p-file-sharing/
├── src/                 # Source files
├── include/            # Header files  
├── tests/              # Unit tests
├── docs/               # Documentation
├── CMakeLists.txt      # Build configuration
├── Makefile           # Development shortcuts
└── README.md          # This file
```

### Testing

```bash
# Run all tests
make test

# Run with verbose output
make test-verbose

# Performance testing
make performance

# Memory leak checking
make memcheck

# Code coverage
make coverage
```

### Debugging

```bash
# Build debug version
make debug

# Run with GDB
cd build-debug
gdb ./p2p-share

# Memory analysis with Valgrind
make memcheck
```

## Performance Metrics

- **Throughput**: 100+ MB/s on gigabit networks
- **Latency**: <1ms for small messages
- **Scalability**: 200+ concurrent connections
- **Memory**: <50MB base memory usage
- **CPU**: Efficient event-driven architecture

## API Documentation

### CLI Commands

| Command | Description | Example |
|---------|-------------|---------|
| `peers` | List connected peers | `peers` |
| `files [local\|peer_id]` | Show available files | `files local` |
| `get <filename> [dest]` | Download file | `get video.mp4 ~/Downloads/` |
| `share <filepath>` | Share file with network | `share /home/user/document.pdf` |
| `connect <ip> <port>` | Connect to peer | `connect 192.168.1.100 8888` |
| `status` | Show node statistics | `status` |
| `downloads` | Show transfer progress | `downloads` |

### Protocol Messages

- `PEER_LIST_REQUEST/RESPONSE`: Peer discovery
- `FILE_LIST_REQUEST/RESPONSE`: File metadata exchange  
- `FILE_REQUEST`: Request file download
- `FILE_CHUNK`: File data transfer
- `ERROR_MESSAGE`: Error reporting
- `PING/PONG`: Heartbeat mechanism

### Code Style

- C++17 standard
- Follow existing naming conventions
- Use `clang-format` for formatting: `make format`
- Run static analysis: `make lint`


## Acknowledgments

- POSIX socket programming examples
- Linux epoll documentation
- C++17 networking best practices
- High-performance server design patterns




