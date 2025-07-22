// TAWQA Bridge - C++ wrapper that can call Rust version
// Modern C++20 implementation that bridges to Rust

#include "tawqa_generic.hh"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>

namespace fs = std::filesystem;

class TawqaBridge {
private:
    std::string rust_binary_path;
    std::string cpp_binary_path;
    
    // Find the Rust binary
    std::string find_rust_binary() {
        std::vector<std::string> possible_paths = {
            "./target/release/tawqa",
            "../target/release/tawqa",
            "/usr/local/bin/tawqa_rust",
            "tawqa_rust"
        };
        
        for (const auto& path : possible_paths) {
            if (fs::exists(path) && fs::is_regular_file(path)) {
                return fs::absolute(path);
            }
        }
        return "";
    }
    
    // Find the C++ binary
    std::string find_cpp_binary() {
        std::vector<std::string> possible_paths = {
            "./tawqa_cpp",
            "./tawqa",
            "/usr/local/bin/tawqa_cpp",
            "tawqa_cpp"
        };
        
        for (const auto& path : possible_paths) {
            if (fs::exists(path) && fs::is_regular_file(path)) {
                return fs::absolute(path);
            }
        }
        return "";
    }
    
    // Check if arguments suggest modern interface (Rust)
    bool should_use_rust(const std::vector<std::string>& args) {
        if (args.empty()) return false;
        
        // Modern commands that suggest Rust version
        std::vector<std::string> rust_commands = {
            "connect", "c", "listen", "l"
        };
        
        for (const auto& cmd : rust_commands) {
            if (args[0] == cmd) return true;
        }
        
        // Check for modern flags
        for (const auto& arg : args) {
            if (arg == "--interactive" || arg == "--local-interactive" || 
                arg == "--block-signals" || arg.starts_with("--shell=")) {
                return true;
            }
        }
        
        return false;
    }
    
    // Execute external program
    int execute_program(const std::string& program, const std::vector<std::string>& args) {
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(program.c_str()));
        
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execv(program.c_str(), argv.data());
            std::cerr << "Failed to execute " << program << std::endl;
            _exit(127);
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            std::cerr << "Failed to fork process" << std::endl;
            return 1;
        }
    }
    
public:
    TawqaBridge() {
        rust_binary_path = find_rust_binary();
        cpp_binary_path = find_cpp_binary();
    }
    
    void print_help() {
        std::cout << R"(TAWQA Bridge - Unified C++/Rust Interface

This bridge automatically selects between C++ and Rust implementations
based on the command syntax used.

Usage:
  tawqa [classic netcat options] host port     # Uses C++ version
  tawqa connect [options] host port            # Uses Rust version  
  tawqa listen [options] [host] port           # Uses Rust version

Classic C++ Interface (netcat-compatible):
  tawqa [-l] [-p port] [-u] [-v] [-z] [-n] host port
  
  Options:
    -l          Listen mode
    -p port     Local port number
    -u          UDP mode
    -v          Verbose output
    -z          Zero-I/O mode (scanning)
    -n          Numeric-only IP addresses
    -h          Show help

Modern Rust Interface:
  tawqa connect --shell <shell> host port
  tawqa listen [--interactive] [--local-interactive] [--exec <cmd>] [host] port
  
  Connect options:
    --shell, -s <shell>    Shell to execute
    
  Listen options:
    --interactive, -i         Interactive mode
    --local-interactive, -l   Local interactive mode
    --block-signals, -b       Block signals
    --exec, -e <command>      Execute command on connection

Examples:
  # Classic interface (uses C++)
  tawqa google.com 80
  tawqa -l -p 4444
  tawqa -v -z 192.168.1.1 22
  
  # Modern interface (uses Rust)
  tawqa connect --shell /bin/bash 192.168.1.100 4444
  tawqa listen --interactive 4444
  tawqa listen --exec "whoami" 0.0.0.0 4444

Available backends:
)";
        
        if (!cpp_binary_path.empty()) {
            std::cout << "  ✓ C++ version: " << cpp_binary_path << std::endl;
        } else {
            std::cout << "  ✗ C++ version: not found" << std::endl;
        }
        
        if (!rust_binary_path.empty()) {
            std::cout << "  ✓ Rust version: " << rust_binary_path << std::endl;
        } else {
            std::cout << "  ✗ Rust version: not found" << std::endl;
        }
    }
    
    int run(const std::vector<std::string>& args) {
        // Handle help
        if (args.empty() || args[0] == "-h" || args[0] == "--help") {
            print_help();
            return 0;
        }
        
        // Determine which version to use
        bool use_rust = should_use_rust(args);
        
        if (use_rust && !rust_binary_path.empty()) {
            std::cout << "Using Rust version..." << std::endl;
            return execute_program(rust_binary_path, args);
        } else if (!use_rust && !cpp_binary_path.empty()) {
            std::cout << "Using C++ version..." << std::endl;
            return execute_program(cpp_binary_path, args);
        } else {
            // Fallback logic
            if (!cpp_binary_path.empty()) {
                std::cout << "Falling back to C++ version..." << std::endl;
                return execute_program(cpp_binary_path, args);
            } else if (!rust_binary_path.empty()) {
                std::cout << "Falling back to Rust version..." << std::endl;
                return execute_program(rust_binary_path, args);
            } else {
                std::cerr << "Error: No TAWQA backends found!" << std::endl;
                std::cerr << "Please build at least one version:" << std::endl;
                std::cerr << "  make cpp   # for C++ version" << std::endl;
                std::cerr << "  make rust  # for Rust version" << std::endl;
                return 1;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    TawqaBridge bridge;
    return bridge.run(args);
}