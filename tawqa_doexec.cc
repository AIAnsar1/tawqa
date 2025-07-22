// TAWQA DoExec Implementation
// Modern C++23 port without OOP - Unix/Linux version
// Using TAWQA prefix to avoid naming conflicts

#include "tawqa_generic.hh"
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <cstdio>

#ifdef TAWQA_GAPING_SECURITY_HOLE

// Global variables
static char* tawqa_program_path = nullptr;

// Function declarations
void tawqa_holler(const char* str, const char* p1 = nullptr, 
                  const char* p2 = nullptr, const char* p3 = nullptr);

// Session data structure (C-style, no OOP)
struct tawqa_session_data {
    int read_pipe_fd;
    int write_pipe_fd;
    pid_t process_id;
    int client_socket;
    bool is_connected;
};

// Create pipes for shell communication
static bool tawqa_create_pipes(int* read_pipe, int* write_pipe, 
                               int* shell_stdin, int* shell_stdout) {
    int pipe_read[2], pipe_write[2];
    
    if (pipe(pipe_read) == -1) {
        tawqa_holler("Failed to create read pipe");
        return false;
    }
    
    if (pipe(pipe_write) == -1) {
        close(pipe_read[0]);
        close(pipe_read[1]);
        tawqa_holler("Failed to create write pipe");
        return false;
    }
    
    *read_pipe = pipe_read[0];      // Parent reads from here
    *shell_stdout = pipe_read[1];   // Child writes to here
    *shell_stdin = pipe_write[0];   // Child reads from here
    *write_pipe = pipe_write[1];    // Parent writes to here
    
    return true;
}

// Start shell process
static pid_t tawqa_start_shell(int shell_stdin, int shell_stdout) {
    pid_t pid = fork();
    
    if (pid == -1) {
        tawqa_holler("Failed to fork shell process");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        dup2(shell_stdin, STDIN_FILENO);
        dup2(shell_stdout, STDOUT_FILENO);
        dup2(shell_stdout, STDERR_FILENO);
        
        close(shell_stdin);
        close(shell_stdout);
        
        // Execute shell
        const char* shell_name = strrchr(tawqa_program_path, '/');
        if (shell_name) {
            ++shell_name;
        } else {
            shell_name = tawqa_program_path;
        }
        
        execl(tawqa_program_path, shell_name, nullptr);
        _exit(127); // exec failed
    }
    
    // Parent process
    close(shell_stdin);
    close(shell_stdout);
    
    return pid;
}

// Handle data transfer from shell to client
static void tawqa_shell_to_client(int read_pipe, int client_socket) {
    constexpr size_t buffer_size = TAWQA_BUFFER_SIZE;
    char buffer[buffer_size];
    char output_buffer[buffer_size * 2];
    
    while (true) {
        ssize_t bytes_read = read(read_pipe, buffer, buffer_size);
        if (bytes_read <= 0) {
            break;
        }
        
        // Process data: convert LF to CRLF
        size_t output_pos = 0;
        char prev_char = 0;
        
        for (ssize_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n' && prev_char != '\r') {
                output_buffer[output_pos++] = '\r';
            }
            prev_char = output_buffer[output_pos++] = buffer[i];
        }
        
        if (send(client_socket, output_buffer, output_pos, 0) <= 0) {
            break;
        }
    }
}

// Handle data transfer from client to shell
static void tawqa_client_to_shell(int client_socket, int write_pipe) {
    char recv_buffer[1];
    char line_buffer[TAWQA_BUFFER_SIZE];
    size_t buffer_pos = 0;
    
    while (recv(client_socket, recv_buffer, 1, 0) > 0) {
        line_buffer[buffer_pos++] = recv_buffer[0];
        
        if (recv_buffer[0] == '\r') {
            line_buffer[buffer_pos++] = '\n';
        }
        
        // Check for exit command
        if (buffer_pos >= 6 && 
            strncasecmp(line_buffer, "exit\r\n", 6) == 0) {
            break;
        }
        
        // Send line when complete or buffer full
        if (recv_buffer[0] == '\n' || recv_buffer[0] == '\r' || 
            buffer_pos >= TAWQA_BUFFER_SIZE - 1) {
            
            if (write(write_pipe, line_buffer, buffer_pos) == -1) {
                break;
            }
            buffer_pos = 0;
        }
    }
}

// Create session
static tawqa_session_data* tawqa_create_session() {
    auto* session = static_cast<tawqa_session_data*>(
        calloc(1, sizeof(tawqa_session_data)));
    
    if (!session) {
        return nullptr;
    }
    
    int shell_stdin, shell_stdout;
    
    if (!tawqa_create_pipes(&session->read_pipe_fd, &session->write_pipe_fd,
                           &shell_stdin, &shell_stdout)) {
        free(session);
        return nullptr;
    }
    
    session->process_id = tawqa_start_shell(shell_stdin, shell_stdout);
    if (session->process_id == -1) {
        close(session->read_pipe_fd);
        close(session->write_pipe_fd);
        free(session);
        return nullptr;
    }
    
    session->client_socket = -1;
    session->is_connected = false;
    
    return session;
}

// Main doexec function
bool tawqa_doexec(int client_socket) {
    tawqa_session_data* session = tawqa_create_session();
    if (!session) {
        return false;
    }
    
    session->client_socket = client_socket;
    session->is_connected = true;
    
    // Fork for handling bidirectional communication
    pid_t comm_pid = fork();
    
    if (comm_pid == -1) {
        tawqa_holler("Failed to fork communication process");
        close(session->read_pipe_fd);
        close(session->write_pipe_fd);
        kill(session->process_id, SIGTERM);
        free(session);
        return false;
    }
    
    if (comm_pid == 0) {
        // Child: handle client to shell
        close(session->read_pipe_fd);
        tawqa_client_to_shell(client_socket, session->write_pipe_fd);
        close(session->write_pipe_fd);
        _exit(0);
    } else {
        // Parent: handle shell to client
        close(session->write_pipe_fd);
        tawqa_shell_to_client(session->read_pipe_fd, client_socket);
        close(session->read_pipe_fd);
        
        // Clean up
        kill(comm_pid, SIGTERM);
        kill(session->process_id, SIGTERM);
        
        int status;
        waitpid(comm_pid, &status, 0);
        waitpid(session->process_id, &status, 0);
    }
    
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    free(session);
    
    return true;
}

// Set program path for execution
void tawqa_set_program_path(const char* path) {
    if (tawqa_program_path) {
        free(tawqa_program_path);
    }
    tawqa_program_path = strdup(path);
}

// Cleanup function
void tawqa_doexec_cleanup() {
    if (tawqa_program_path) {
        free(tawqa_program_path);
        tawqa_program_path = nullptr;
    }
}

#else

// Stub implementation when security hole is disabled
bool tawqa_doexec([[maybe_unused]] int client_socket) {
    tawqa_holler("doexec support not compiled in");
    return false;
}

void tawqa_set_program_path([[maybe_unused]] const char* path) {
    // No-op
}

void tawqa_doexec_cleanup() {
    // No-op
}

#endif // TAWQA_GAPING_SECURITY_HOLE