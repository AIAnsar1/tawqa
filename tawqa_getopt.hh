#pragma once

#ifndef TAWQA_GETOPT_HH_INCLUDED
#define TAWQA_GETOPT_HH_INCLUDED

// TAWQA GetOpt Header
// Modern C++23 port of getopt functionality
// Using TAWQA prefix to avoid naming conflicts

#include <string_view>
#include <span>
#include <optional>

// Modern C++23 getopt implementation
namespace tawqa {

struct Option {
    std::string_view name;
    bool has_arg;
    int* flag;
    int val;
};

enum class ArgumentType {
    NO_ARGUMENT = 0,
    REQUIRED_ARGUMENT = 1,
    OPTIONAL_ARGUMENT = 2
};

class GetOpt {
public:
    // Global state variables (for compatibility)
    static inline char* optarg = nullptr;
    static inline int optind = 1;
    static inline int opterr = 1;
    static inline int optopt = '?';

    // Main getopt function
    static int getopt(int argc, char* const argv[], std::string_view optstring);
    
    // Long option support
    static int getopt_long(int argc, char* const argv[], 
                          std::string_view optstring,
                          std::span<const Option> longopts, 
                          int* longindex = nullptr);

private:
    static inline std::string_view nextchar;
    static inline int first_nonopt = 1;
    static inline int last_nonopt = 1;
    
    enum class Ordering {
        REQUIRE_ORDER,
        PERMUTE,
        RETURN_IN_ORDER
    };
    
    static inline Ordering ordering = Ordering::PERMUTE;
    
    static std::string_view initialize_getopt(std::string_view optstring);
    static void exchange_args(char** argv);
    static std::optional<char> find_char_in_string(std::string_view str, char c);
};

} // namespace tawqa

// C-style interface for compatibility
extern "C" {
    extern char* optarg;
    extern int optind;
    extern int opterr;
    extern int optopt;
    
    int tawqa_getopt(int argc, char* const argv[], const char* optstring);
}

#endif // TAWQA_GETOPT_HH_INCLUDED