// TAWQA Hybrid Implementation
// C++ frontend that can use Rust backend
// Modern C++20 without OOP

#include "tawqa_generic.hh"
#include "tawqa_getopt.hh"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>

using namespace std::string_literals;

// Global settings
static bool g_use_rust_backend = false;
static bool g_verbose = false;
static std::string g_rust_binary_path = "./target/release/tawqa";

// Function declarations
void tawqa_hybrid_help();
int tawqa_execute_rust_backend(const std::vector<std::string>& args);
int tawqa_execute_cpp_backend(int argc, char* argv[]);
bool tawqa_rust_binary_exists();
std::vector<std::string> tawqa_convert_args_to_rust_format(int argc, char* argv[]);

// Check if Rust binary exists
bool tawqa_rust_binary_exists() {
    return std::filesystem::exists(g_rust_binary_path);
}

// Convert C++ style arguments to Rust style
std::vector<std::string> tawqa_convert_args_to_rust_format(int argc, char* argv[]) {
    std::vector<std::string> rust_args;
    rust_args.push_back(g_rust_binary_path);
    
    bool listen_mode = false;
    bool connect_mode = false;
    std::string host, port;
    
    // Parse arguments and convert to Rust format
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-l") {
            listen_mode = true;
        } else if (arg == "-p" && i + 1 < argc) {
            port = argv[++i];
        } else if (arg == "-v") {
            // Rust version doesn't have -v, we'll handle verbosity in C++
            g_verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            rust_args.clear();
            rust_args.push_back(g_rust_binary_path);
            rust_args.push_back("--help");
            return rust_args;
        } else if (arg == "-e" && i + 1 < argc) {
            rust_args.push_back("--exec");
            rust_args.push_back(argv[++i]);
        } else if (arg == "-i") {
            rust_args.push_back("--interactive");
        } else if (arg == "-b") {
            rust_args.push_back("--block-signals");
        } else if (arg == "--local-interactive") {
            rust_args.push_back("--local-interactive");
        } else if (arg == "-s" && i + 1 < argc) {
            rust_args.push_back("--shell");
            rust_args.push_back(argv[++i]);
        } else if (arg[0] != '-') {
            // This is likely host or port
            if (host.empty()) {
                host = arg;
            } else if (port.empty()) {
                port = arg;
            }
        }
    }
    
    // Determine command based on parsed arguments
    if (listen_mode) {
        rust_args.push_back("listen");
        if (!port.empty()) {
            rust_args.push_back(port);
        }
    } else if (!host.empty()) {
        rust_args.push_back("connect");
        rust_args.push_back(host);
        if (!port.empty()) {
            rust_args.push_back(port);
        }
    }
    
    return rust_args;
}

// Execute Rust backend
int tawqa_execute_rust_backend(const std::vector<std::string>& args) {
    if (g_verbose) {
        std::printf("Using Rust backend: %s\n", g_rust_binary_path.c_str());
        std::printf("Command: ");
        for (const auto& arg : args) {
            std::printf("%s ", arg.c_str());
        }
        std::printf("\n");
    }
    
    // Convert vector to char* array for execv
    std::vector<char*> c_args;
    for (const auto& arg : args) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - execute Rust binary
        execv(g_rust_binary_path.c_str(), c_args.data());
        std::perror("Failed to execute Rust backend");
        _exit(127);
    } else if (pid > 0) {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        std::perror("Failed to fork");
        return 1;
    }
}

// Execute C++ backend (fallback)
int tawqa_execute_cpp_backend(int argc, char* argv[]) {
    if (g_verbose) {
        std::printf("Using C++ backend\n");
    }
    
    // Check if C++ binary exists
    std::string cpp_binary = "./tawqa_cpp";
    if (!std::filesystem::exists(cpp_binary)) {
        std::fprintf(stderr, "Error: C++ backend not found at %s\n", cpp_binary.c_str());
        std::fprintf(stderr, "Please build it with: make cpp\n");
        return 1;
    }
    
    // Prepare arguments for C++ version
    std::vector<std::string> cpp_args;
    cpp_args.push_back(cpp_binary);
    
    // Copy all arguments except the program name
    for (int i = 1; i < argc; ++i) {
        cpp_args.push_back(argv[i]);
    }
    
    if (g_verbose) {
        std::printf("Command: ");
        for (const auto& arg : cpp_args) {
            std::printf("%s ", arg.c_str());
        }
        std::printf("\n");
    }
    
    // Convert vector to char* array for execv
    std::vector<char*> c_args;
    for (const auto& arg : cpp_args) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - execute C++ binary
        execv(cpp_binary.c_str(), c_args.data());
        std::perror("Failed to execute C++ backend");
        _exit(127);
    } else if (pid > 0) {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        std::perror("Failed to fork");
        return 1;
    }
}

// Help function
void tawqa_hybrid_help() {
    std::printf(R"(TAWQA Hybrid Version - Intelligent C++/Rust netcat

This version automatically selects the best backend for your needs:
- Rust backend for advanced features (interactive modes, modern CLI)
- C++ backend for classic netcat compatibility

Usage: tawqa_hybrid [OPTIONS] [HOST] [PORT]

Classic netcat options (C++ backend):
  -l              Listen mode
  -p PORT         Local port number
  -u              UDP mode
  -v              Verbose output
  -z              Zero-I/O mode (port scanning)
  -n              Numeric-only IP addresses
  -w SECS         Timeout for connections

Advanced options (Rust backend):
  -i              Interactive mode
  -b              Block signals
  --local-interactive  Local interactive mode
  -e COMMAND      Execute command on connection
  -s SHELL        Shell to use for connections

Backend control:
  --rust          Force use of Rust backend
  --cpp           Force use of C++ backend
  --version       Show version information
  -h, --help      Show this help

Backend Status:
  - Rust backend: %s (%s)
  - C++ backend: Always available

Auto-selection rules:
  1. Advanced features (-i, -b, --local-interactive) → Rust
  2. Classic features (-u, -z, -n) → C++
  3. Default: Rust if available, otherwise C++

Examples:
  tawqa_hybrid -l -p 4444                    # Listen (auto-select)
  tawqa_hybrid 192.168.1.100 4444            # Connect (auto-select)
  tawqa_hybrid -i -l 4444                     # Interactive listen (Rust)
  tawqa_hybrid -z 192.168.1.100 80           # Port scan (C++)
  tawqa_hybrid --rust listen 4444             # Force Rust syntax
  tawqa_hybrid --cpp -l -p 4444               # Force C++ syntax

)", g_rust_binary_path.c_str(), 
    tawqa_rust_binary_exists() ? "Available" : "Not found");
}

// Main function
int main(int argc, char* argv[]) {
    // Check for backend selection flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--rust") {
            g_use_rust_backend = true;
            // Remove this flag from arguments
            for (int j = i; j < argc - 1; ++j) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        } else if (arg == "--cpp") {
            g_use_rust_backend = false;
            // Remove this flag from arguments
            for (int j = i; j < argc - 1; ++j) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        } else if (arg == "-h" || arg == "--help") {
            tawqa_hybrid_help();
            return 0;
        } else if (arg == "-v") {
            g_verbose = true;
        }
    }
    
    // Auto-detect backend if not forced
    if (!g_use_rust_backend && argc > 1) {
        // Check if we should use Rust backend
        bool has_rust_specific_features = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-i" || arg == "--interactive" || 
                arg == "--local-interactive" || arg == "-b") {
                has_rust_specific_features = true;
                break;
            }
        }
        
        if (has_rust_specific_features && tawqa_rust_binary_exists()) {
            g_use_rust_backend = true;
            if (g_verbose) {
                std::printf("Auto-selecting Rust backend for advanced features\n");
            }
        }
    }
    
    // Default to Rust if available and not explicitly disabled
    if (!g_use_rust_backend && tawqa_rust_binary_exists()) {
        g_use_rust_backend = true;
    }
    
    if (argc < 2) {
        tawqa_hybrid_help();
        return 1;
    }
    
    // Execute appropriate backend
    if (g_use_rust_backend) {
        if (!tawqa_rust_binary_exists()) {
            std::fprintf(stderr, "Error: Rust backend not found at %s\n", 
                        g_rust_binary_path.c_str());
            std::fprintf(stderr, "Please build it with: cargo build --release\n");
            return 1;
        }
        
        auto rust_args = tawqa_convert_args_to_rust_format(argc, argv);
        return tawqa_execute_rust_backend(rust_args);
    } else {
        return tawqa_execute_cpp_backend(argc, argv);
    }
}