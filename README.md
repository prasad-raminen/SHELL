C Shell (Custom POSIX Shell)
A modular, POSIX-compliant shell implementation in C. This project handles command parsing via a Context-Free Grammar (CFG), process management, I/O redirection, and complex job control.

🛠 Compilation Requirements
The shell must be compiled using the following strict flags to ensure POSIX compliance:

Bash

gcc -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
To build:

Navigate to the shell directory.

Run make all.

Execution: ./shell.out.

📦 Core Modules
1. Shell Input & Grammar
The shell implements a parser based on the following grammar:

Sequencing: ; for sequential and & for background execution.

Piping: | to connect stdout of one process to stdin of the next.

Redirection: < (input), > (output truncate), and >> (output append).

Validation: Any syntax violation (e.g., empty pipe | ;) triggers an Invalid Syntax! error.

2. Built-in Functions (Intrinsics)
No exec* system calls are used for these commands. | Command | Usage | Description | | :--- | :--- | :--- | | hop | hop [dir] | Changes CWD. Supports ~, ., .., and -. | | reveal | reveal -[al] [dir] | Lists directory contents in ASCII order. | | log | log [purge\|execute] | Manages a history of the last 15 commands. |

3. Process & Job Control
The shell manages multiple processes using fork(), execvp(), and waitpid().

Foreground: The shell waits until the process group finishes.

Background (&): Commands run asynchronously. The shell notifies upon completion with exit status.

Activities: Monitors all active processes and their states (Running vs Stopped).

4. I/O Redirection & Pipelines
The shell utilizes pipe() and dup2() for complex data routing:

Pipes: Multi-stage pipelines (e.g., cmd1 | cmd2 | cmd3).

Redirection: Integrated with pipes (e.g., cmd1 < in.txt | cmd2 > out.txt).

5. Signal Handling & Shortcuts
Ctrl-C (SIGINT): Interrupts the current foreground process.

Ctrl-Z (SIGTSTP): Suspends the foreground process and moves it to the background.

Ctrl-D (EOF): Cleanly logs out and terminates all child processes.

ping: Sends signals to specific PIDs via signal % 32.

📂 Project Structure
/src: Function implementations.

/include: Header files defining structures and POSIX macros.

Makefile: Build script to generate shell.out
