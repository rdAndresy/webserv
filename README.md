# minishell
Build a non-blocking HTTP server in C++98

# Usage
make ; ./webserv [config file.conf]

# Usefull information
- Default configuration files are in conf/ folder
- We chose the epoll() I/O multiplexer function for this project

# Authorized functions
1. `execve`: Executes a file by replacing the current process image with a new one.
2. `dup`: Duplicates a file descriptor, making it refer to the same open file description.
3. `dup2`: Duplicates a file descriptor to a specified value.
4. `pipe`: Creates a unidirectional data channel that can be used for interprocess communication.
5. `strerror`: Returns a string describing the error number.
6. `gai_strerror`: Returns a string describing an error code returned by <code>getaddrinfo</code> or <code>getnameinfo</code>.
7. `errno`: An external variable that stores the last error code set by a system call or library function.
8. `fork`: Creates a new process by duplicating the calling process.
9. `socketpair`: Creates a pair of connected sockets.
10. `htons`: Converts a 16-bit number from host byte order to network byte order.
11. `htonl`: Converts a 32-bit number from host byte order to network byte order.
12. `ntohs`: Converts a 16-bit number from network byte order to host byte order.
13. `ntohl`: Converts a 32-bit number from network byte order to host byte order.
16. `epoll_create`: Creates an epoll instance for event monitoring.
17. `epoll_ctl`: Controls the behavior of an epoll instance.
18. `epoll_wait`: Waits for events on an epoll instance.
19. `kqueue`: Creates a kernel event queue.
20. `kevent`: Registers or modifies events in a kqueue.
21. `socket`: Creates an endpoint for communication and returns a socket descriptor.
22. `accept`: Accepts a connection on a socket.
23. `listen`: Marks a socket as a passive socket for accepting connections.
24. `send`: Sends data on a socket.
25. `recv`: Receives data from a socket.
26. `chdir`: Changes the current working directory.
27. `bind`: Assigns a local address to a socket.
28. `connect`: Establishes a connection to a specified socket.
29. `getaddrinfo`: Translates a host name or address and service name into a socket address.
30. `freeaddrinfo`: Frees memory allocated by <code>getaddrinfo</code>.
31. `setsockopt`: Configures socket options.
32. `getsockname`: Retrieves the local address of a socket.
33. `getprotobyname`: Retrieves protocol information for a given protocol name.
34. `fcntl`: Performs various control operations on a file descriptor.
35. `close`: Closes a file descriptor.
36. `read`: Reads data from a file descriptor.
37. `write`: Writes data to a file descriptor.
38. `waitpid`: Waits for a specific child process to stop or terminate.
39. `kill`: Sends a signal to a process.
40. `signal`: Installs a signal handler for a specific signal.
41. `access`: Checks the accessibility of a file.
42. `stat`: Retrieves information about a file based on its path.
43. `open`: Opens a file and returns a file descriptor.
44. `opendir`: Opens a directory stream.
45. `readdir`: Reads the next entry in a directory stream.
46. `closedir`: Closes a directory stream.
