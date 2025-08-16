
## Phase-wise Plan

### Phase 1: Project Initialization & Network Setup

**Goals:**

* Set up project structure using CMake.
* Implement basic TCP socket server-client connection.
* Establish peer-to-peer connection over local IP.

**Tasks:**

* [ ] Initialize Git repository and `.gitignore`.
* [ ] Set up directory structure (`src/`, `include/`, `tests/`, `build/`).
* [ ] Write `CMakeLists.txt` for project and submodules.
* [ ] Implement:

  * TCP server (`Server.cpp`)
  * TCP client (`Client.cpp`)
* [ ] Test one-way message passing between two terminals.

**Milestone Deliverable:**

* Simple client-server message exchange via terminal.

---

### Phase 2: Peer Identity and Bootstrap

**Goals:**

* Define peer identity (IP, port, file list).
* Enable peer discovery (via manual input or bootstrap node).

**Tasks:**

* [ ] Define a `Peer` class with metadata.
* [ ] Implement peer list maintenance (`PeerManager.cpp`).
* [ ] Add bootstrap server for initial peer list sharing.
* [ ] Design and parse a simple protocol message format (e.g., JSON or custom).

**Milestone Deliverable:**

* Bootstrap connection and peer listing visible on CLI.

---

### Phase 3: File Listing and Metadata Sharing

**Goals:**

* Allow peers to share file metadata (filenames, sizes, hash).

**Tasks:**

* [ ] Implement local file scanning and metadata hashing (e.g., SHA-256).
* [ ] Exchange file lists with connected peers.
* [ ] Store peer-file mapping for search.

**Milestone Deliverable:**

* Interactive CLI command to list available files from all peers.

---

### Phase 4: File Transfer with Concurrency

**Goals:**

* Implement file upload and download between peers.
* Enable concurrent transfers using multithreading.

**Tasks:**

* [ ] Use POSIX threads to handle incoming requests per thread.
* [ ] Design message types: `REQ_FILE`, `SEND_FILE`, `ACK`, `ERR`.
* [ ] Buffering system for efficient transfer (custom chunk size).
* [ ] Support resumable downloads using byte-range requests.

**Milestone Deliverable:**

* Download file from one or more peers concurrently via CLI.

---

### Phase 5: Scaling with Non-blocking I/O

**Goals:**

* Switch from blocking sockets to non-blocking I/O.
* Optimize for 200+ simultaneous connections.

**Tasks:**

* [ ] Use `select()`/`poll()` or `epoll()` for multiplexing.
* [ ] Handle socket readiness and errors gracefully.
* [ ] Tune socket buffer sizes and chunked reading/writing.
* [ ] Benchmark throughput and latency under load.

**Milestone Deliverable:**

* Stable operation with 200 concurrent connections + performance report.

---

### Phase 6: Command-line Interface & CLI Shell

**Goals:**

* Build an interactive terminal CLI for users.

**Tasks:**

* [ ] CLI commands:

  * `peers` – list known peers
  * `files` – list available files
  * `get <filename>` – request file
  * `share <filepath>` – share new file
  * `exit` – exit the network
* [ ] Optional: Use `readline` for better shell input.

**Milestone Deliverable:**

* Interactive CLI to perform all file-sharing operations.

---

### Phase 7: Testing & Debugging

**Goals:**

* Unit test and debug core modules.

**Tasks:**

* [ ] Write unit tests for `Peer`, `FileManager`, and `PeerManager` using GTest.
* [ ] Use GDB to track memory leaks, race conditions.
* [ ] Validate file hash integrity post-download.
* [ ] Simulate packet loss and partial transfer recovery.

**Milestone Deliverable:**

* Clean test suite passing + stable download/upload verification.

---

### Phase 8: Packaging & Deployment

**Goals:**

* Create build scripts and documentation.

**Tasks:**

* [ ] Add installable build via CMake.
* [ ] Write `README.md` with usage instructions.
* [ ] Package project as `.tar.gz` or `.deb`.
* [ ] Add sample configs and bootstrap server IPs.

**Milestone Deliverable:**

* Fully packaged project with documentation and sample run.

---

## Phase 9 (Post-MVP), will work on this later above MVP of 8 phase

* Add encrypted file transfer (TLS).
* Use protocol buffers or Cap’n Proto for efficient messaging.
* Implement a DHT for decentralized discovery.
* Introduce a torrent-style multi-source download.
* Add stats dashboard (CLI or web).
* NAT traversal using STUN/TURN (advanced).

---