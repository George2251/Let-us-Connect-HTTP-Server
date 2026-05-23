Let-us-Connect HTTP Server

Multithreaded HTTP/1.1 Server in C

Academic Engineering Report & Open-Source Reference

Repository: Let-us-Connect-HTTP-Server

Language: C (C11/GNU Extensions)

Build System: GNU Make

Platform: Linux (POSIX)

Abstract

Let-us-Connect is a fully functional, multithreaded HTTP/1.1 server implemented from first principles in the C programming language. The project demonstrates a complete vertical slice of systems programming: from raw POSIX socket management through a custom-built thread pool, a FIFO work queue, a hand-written HTTP request parser, a content-dispatching request handler, and a generic data-structures library.

Unlike server frameworks that abstract away low-level details, every layer of this system is purpose-built, making it an outstanding educational artifact and a practical foundation for production-grade development.

Table of Contents

Project Overview

Repository Structure

System Architecture

Networking Architecture

Thread Pool Design

FIFO Queue Design

HTTP Parsing

Request Handler

Data Structures

Concurrency Model

API Reference

Build System

Memory Management

Error Handling

Security Considerations

Performance Considerations

Limitations & Future Improvements

Testing Strategy

Appendices

1. Project Overview

Introduction

Let-us-Connect was conceived as both an academic systems-programming exercise and a functional proof-of-concept for a production-capable web server. The guiding principle was to avoid third-party libraries entirely: no libevent, no libuv, no libcurl—only the POSIX standard, the C standard library, and pthreads.

The result is a server that demonstrates how the core internals of production servers such as Nginx or Apache are designed:

A non-blocking listening socket multiplexed with select(2).

A fixed-size thread pool that consumes work items from a doubly-linked FIFO queue.

A hand-written HTTP/1.1 parser that decomposes raw byte streams into structured dictionaries.

A pluggable routing table backed by a Binary Search Tree.

Keep-Alive connection tracking with idle-timeout management.

Zero-copy file delivery via Linux sendfile(2).

Key Features

Feature

Description

HTTP/1.1 Support

GET, HEAD, POST methods; proper status codes (200, 201, 304, 400, 403, 404, 405, 411, 500, 501, 505).

Keep-Alive

Persistent connections with 10-second idle timeout, multiplexed via select().

Thread Pool

Static-size pool (default 8 threads), semaphore-driven wake-up, three destroy modes.

Non-Blocking I/O

Both the server socket and all client sockets operate in O_NONBLOCK mode.

FIFO Work Queue

Doubly-linked list queue with $O(1)$ push-to-tail and pop-from-head.

Routing Table

BST-backed dictionary; wildcard "*" fallback to filesystem handler.

Zero-Copy Delivery

sendfile(2) transfers file data directly from kernel page-cache to socket.

MIME Detection

Extension-based MIME type resolution (HTML, CSS, JS, PNG, JPEG).

Path Traversal Guard

Blocks URIs containing ../ to prevent directory traversal attacks.

POST Persistence

Appends POST body data to public/post_data.txt on disk.

Graceful Shutdown

Three-mode destroy: Soft (drain), Hard (cancel), Drain-and-Drop.

2. Repository Structure

Let-us-Connect-HTTP-Server/
|-- DataStructures/                 # Generic C data-structure library
|   |-- Common/
|   |   |-- Node.h / Node.c         # Generic doubly-linked node
|   |-- Dictionary/
|   |   |-- Dictionary.h/.c         # BST-backed key-value map
|   |   |-- Entry.h/.c              # Key-value pair container
|   |-- Lists/
|   |   |-- LinkedList.h/.c         # Dynamic linked list
|   |   |-- Queue.h/.c              # FIFO queue built on LinkedList
|   |-- Trees/
|       |-- BinarySearchTree.h/.c   # Generic BST
|-- webserver_project/              # Core server application
|   |-- main.c                      # Entry point
|   |-- Makefile                    # GNU Make build script
|   |-- include/                    # Public header files
|   |   |-- network.h
|   |   |-- thread_pool.h
|   |   |-- fifo_queue.h
|   |   |-- http_parser.h / http_types.h
|   |   |-- http_handler.h
|   |-- src/
|   |   |-- network/
|   |   |   |-- network.c           # Socket layer + select loop
|   |   |   |-- thread_pool.c       # Thread pool implementation
|   |   |   |-- fifo_queue.c        # FIFO queue for tasks
|   |   |-- parser/
|   |   |   |-- http_parser.c       # HTTP/1.1 request parser
|   |   |-- handler/
|   |       |-- http_handler.c      # File-system request handler
|   |       |-- logger.c            # (Logging stub)
|   |-- public/                     # Default static content root
|       |-- index.html
|       |-- post_data.txt
|-- .github/
|-- test.http                       # VS Code REST Client test file


Module Dependency Graph

graph TD
    main[main.c] --> net[network.c]
    main --> tp[thread_pool.c]
    main --> dict[Dictionary]
    
    net --> tp
    net --> parser[http_parser.c]
    
    tp --> fq[fifo_queue.c]
    tp --> parser
    tp --> handler[http_handler.c]
    
    parser --> dict
    handler --> net
    
    dict --> bst[BST]
    dict --> ll[LinkedList]
    dict --> entry[Entry]


3. System Architecture

Let-us-Connect employs a Reactor + Thread-Pool hybrid pattern. The main thread acts as an event demultiplexer (the reactor), using select() to detect readability events on the server socket and all idle Keep-Alive client sockets. Once a socket becomes readable, the main thread wraps the file descriptor in a ClientTask and pushes it onto the thread pool's FIFO queue. One of the pre-spawned worker threads wakes up, pops the task, reads the HTTP request, parses it, routes it, and writes the HTTP response—all without blocking the main reactor loop.

Server Lifecycle

flowchart TD
    Start[Program Entry] --> mkdir[Create www/ directory]
    mkdir --> Sig[Setup Signals: SIGPIPE -> SIG_IGN]
    Sig --> Routes[Build Route Dictionary]
    Routes --> TP[Init Thread Pool]
    TP --> Net[Init Network]
    Net --> Loop[Enter Main select() Loop]
    Loop --> Event{Event?}
    
    Event -- server fd --> NewConn[Accept + alloc ClientTask]
    Event -- client fd --> ExistConn[Existing conn readable]
    
    NewConn --> Queue[add_work -> FIFO Queue]
    ExistConn --> Queue
    
    Queue --> KA[Check Keep-Alive Timeouts]
    KA -- loop back --> Loop
    KA -- shutdown --> Term[Shutdown + Pool Destroy]


Request Processing Pipeline

flowchart LR
    Client -->|TCP accept| Reactor[select() Demux]
    Reactor -->|push_last| Queue[FIFO Task Queue]
    Queue -->|pop_first + recv| Worker[Worker Thread]
    Worker -->|http_request_constructor| Parser[HTTP Parser]
    Parser -->|BST Search| Router[Route Lookup]
    Router -->|handle_filesystem_request| Handler[Handler Dispatch]
    Handler -->|send_response_head + sendfile| Resp[Response Assembly]


4. Networking Architecture

Socket Initialization

All socket setup is handled in network_init() inside src/network/network.c.

int server_fd = socket(AF_INET, SOCK_STREAM, 0); 

setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
make_nonblocking(server_fd); 

server_addr.sin_family      = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;  
server_addr.sin_port        = htons(port);

bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
listen(server_fd, SOMAXCONN);  


Non-Blocking I/O Model

Both the server socket and every client socket are set non-blocking immediately upon acceptance.

accept() returns EAGAIN/EWOULDBLOCK when no pending connections remain, allowing the drain loop to terminate cleanly.

recv() inside worker threads returns immediately if no data is ready, preventing a single slow client from monopolizing a worker thread.

⚠️ Blocking vs. Non-Blocking Tradeoff: > Non-blocking I/O introduces complexity (EAGAIN checks, partial read buffering) but is essential for scalability. The current design uses non-blocking reads with a per-connection read buffer (conn->read_buffer[8192]) to accumulate partial HTTP requests.

The select() Reactor Loop

The main thread runs a tight reactor loop built around the POSIX select() system call. It utilizes a 1-second timeout to ensure check_keepalive_timeouts() is called approximately every second.

Keep-Alive Tracking

A static array of 256 ClientConnection slots is maintained. A slot is claimed atomically under conns_mutex. The slot is released when the connection is closed.

Response Utilities

send_all(): A reliable send loop handling EINTR and EAGAIN.

send_file(): Zero-copy delivery using Linux sendfile(2). Transfers data directly from the kernel's file-descriptor buffer to the socket buffer, bypassing userspace.

5. Thread Pool Design

The thread pool is governed by include/thread_pool.h. It follows the classic Worker Pool pattern: a fixed number of threads block on a semaphore and wake up one-at-a-time as tasks are pushed onto the FIFO queue.

Worker Thread Lifecycle

stateDiagram-v2
    IDLE: IDLE (sem_wait)
    WAKE: Wake (sem_post received)
    LOCK: Acquire mutex
    CHECK: Check if TERMINATED
    POP: Pop task, release mutex
    WORK: Process request
    DONE: Acquire mutex, state=FREE
    EXIT: pthread_exit(0)

    IDLE --> WAKE
    WAKE --> LOCK
    LOCK --> CHECK
    CHECK --> EXIT: YES
    CHECK --> POP: NO
    POP --> WORK
    WORK --> DONE
    DONE --> IDLE


Destroy Modes

Mode

Behavior

THREAD_POOL_DESTROY_SOFT

Spins until available_work == 0, signals TERMINATED to each thread and joins all. Safest for graceful shutdown.

THREAD_POOL_DESTROY_DRAIN_AND_DROP

Immediately signals TERMINATED without waiting for the queue to empty.

THREAD_POOL_DESTROY_HARD

Issues pthread_cancel() to each thread. Fastest but leaves resources unfreed. (Currently used in main.c).

6. FIFO Queue Design

The FIFO queue is a doubly-linked list with $O(1)$ push-to-tail and $O(1)$ pop-from-head.

Thread Safety: The queue itself has no internal locks. Concurrent access safety is guaranteed by the thread pool's pool_lock mutex, which must be held by any caller before invoking push_last() or pop_first().

7. HTTP Parsing

The parser is a single-pass, allocation-aware, destructive parser.

Makes exactly one heap allocation for a mutable copy of the raw request string.

Uses strtok_r() for thread-safe tokenization.

Normalizes all header field names to lowercase before storage.

Parsing Pipeline

flowchart TD
    Raw[Raw byte buffer] --> Copy[Mutable heap copy]
    Copy --> Sep1[Split at \\r\\n\\r\\n]
    Sep1 --> Sep2[Split at first \\r\\n]
    
    Sep2 --> RL[Request Line: method, uri, version]
    Sep2 --> Hdr[Header Lines: key-value pairs]
    Sep1 --> Body[Body: raw + form fields]
    
    RL --> Dict[HTTPRequest: 3 Dictionaries]
    Hdr --> Dict
    Body --> Dict


8. Request Handler

handle_filesystem_request() is the default catch-all handler registered under the wildcard route "*".

Method

Action

GET

Resolve URI to public/{uri}, stat file, send headers + sendfile() body.

HEAD

Identical to GET but body transmission is skipped.

POST

Require Content-Length; log body to public/post_data.txt; return 201 Created.

Others

Return 405 Method Not Allowed.

Security: The handler explicitly rejects any URI containing ../, preventing directory traversal.

9. Data Structures

The DataStructures/ directory contains a self-contained, generic C library.

classDiagram
    Dictionary <|-- BinarySearchTree : inherits
    Dictionary *-- LinkedList : holds keys
    BinarySearchTree *-- Node
    LinkedList *-- Node
    BinarySearchTree *-- Entry
    
    class Dictionary {
        +BinarySearchTree bst
        +LinkedList keys
        +insert()
        +search()
    }


⚠️ Scalability Note: For routing tables with hundreds of routes, the current unbalanced BST could degrade to $O(n)$. A future version should use a Hash Table.

10. Concurrency Model

Primitive

Instance Name

Purpose

pthread_mutex_t

pool_lock

Protects thread pool internal state (queue, task counters, state transitions).

pthread_mutex_t

conns_mutex

Protects client_conns[] array from concurrent modification.

Named sem_t

pool_sem

Counts available tasks; worker threads block on it.

Named sem_t

INIT_SEM

Single-use barrier used during pool initialization to serialize thread spawning.

11. API Reference

Network API (network.h)

Signature

Description

int network_init(int port)

Creates, configures, binds, and listens on a TCP socket.

void network_run_server(...)

Enters the main event loop. Runs indefinitely.

int send_all(...)

Reliably sends data, retrying on EINTR and EAGAIN.

int send_file(...)

Sends a file via sendfile(2). Zero-copy.

int send_response_head(...)

Formats and sends a complete HTTP response header.

Thread Pool API (thread_pool.h)

Signature

Description

int thread_pool_init(...)

Allocates resources, creates workers, waits for readiness.

int add_work(...)

Enqueues a task and posts the semaphore. Lock-protected.

int thread_pool_destroy(...)

Shuts down all threads according to the destroy mode.

Parser & Handler API

Signature

Description

struct HTTPRequest http_request_constructor(char*)

Parses raw text into dictionaries. Caller owns result.

void http_request_destructor(struct HTTPRequest*)

Frees embedded dictionaries.

void handle_filesystem_request(...)

Default filesystem handler for GET/HEAD/POST.

12. Build System

Prerequisites:

GCC 9+ or Clang 10+ with C11 support

GNU Make 4.0+

Linux kernel 4.0+ (for sendfile offset support)

Compilation & Run:

# Clone the repository
git clone [https://github.com/](https://github.com/)<org>/Let-us-Connect-HTTP-Server.git
cd Let-us-Connect-HTTP-Server/webserver_project

# Build the server binary
make

# Start server (listens on port 8080)
./server


13. Memory Management

Allocation

Owner / Lifetime

Freed By

ClientTask (heap)

Main loop per connection

Worker thread

HTTPRequest fields

http_request_constructor

http_request_destructor

Queue node_t

push_last

pop_first

Pool arrays

thread_pool_init

thread_pool_destroy

14. Error Handling

Error handling follows a fail-fast with graceful degradation model.

Fatal: (socket fail, thread init) prints to stderr and exits.

Per-connection: Connection closed, slot freed, server continues.

HTTP-level: Sends appropriate HTTP error status code (400, 403, 404, 405, 411, 505) and HTML body.

15. Security Considerations

✅ Path Traversal Guard: Explicitly rejects any URI containing ../.

✅ SIGPIPE Suppression: SIGPIPE is ignored to prevent client disconnects from crashing the server.

⚠️ Known Gaps: > * No TLS/HTTPS: Traffic is plaintext.

No Request Size Limits: A malicious client could claim infinite Content-Length.

No Rate Limiting: Single IP could exhaust all 256 connection slots.

16. Performance Considerations

Zero-copy: sendfile(2) bypasses userspace memory for file delivery.

Thread Pre-allocation: Avoids runtime pthread_create() overhead.

Drain Loop: accept() loop processes the entire OS backlog in one iteration.

17. Limitations & Future Improvements

Current Limitations:

HTTP/1.1 and IPv4 only.

No chunked transfer encoding or compression (gzip/brotli).

No URL percent-decoding.

Short-Term Roadmap:

epoll migration: Replace select() for horizontal scalability beyond 1024 connections.

Configurable parameters: Expose port, threads, and roots via CLI args.

Content-Length validation: Enforce maximum body limits.

Long-Term Roadmap:

TLS layer: Integrate OpenSSL/mbedTLS.

Multi-process model: Use SO_REUSEPORT to scale across CPU cores.

Hash table routing: Move away from BST for $O(1)$ lookups.

18. Testing Strategy

The repository includes test.http for use with the VS Code REST Client.

To run a manual test suite using curl:

# GET test
curl -v http://localhost:8080/index.html

# POST test
curl -v -X POST http://localhost:8080/submit \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "name=Alice&age=30"


19. Appendices

Sequence Diagram: Full Request Lifecycle

sequenceDiagram
    participant C as Client
    participant M as Main (Reactor)
    participant Q as Queue
    participant W as Worker
    participant P as Parser
    participant H as Handler

    C->>M: TCP connect
    M-->>C: accept() -> fd
    M->>Q: add_work(task)
    W->>Q: pop_first(task)
    W->>W: recv(fd, buf)
    W->>P: http_request_constructor(buf)
    P-->>W: HTTPRequest{}
    W->>H: handle_filesystem_request()
    H->>H: stat() + open()
    H-->>W: file_fd
    W->>C: send_response_head()
    W->>C: sendfile(fd, file_fd)
    W->>P: http_request_destructor()
    W->>M: is_busy = 0 (keep-alive)
