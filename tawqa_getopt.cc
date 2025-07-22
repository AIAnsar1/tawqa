// TAWQA GetOpt Implementation
// Modern C++23 port without OOP
// Using TAWQA prefix to avoid naming conflicts

#include "tawqa_getopt.hh"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <string_view>
#include <string>

using namespace std::string_view_literals;

// Global variables for C compatibility
char* optarg = nullptr;
int optind = 1;
int opterr = 1;
int optopt = '?';

// Internal state
static const char* nextchar = nullptr;
static int first_nonopt = 1;
static int last_nonopt = 1;

enum class ordering_t {
    REQUIRE_ORDER,
    PERMUTE,
    RETURN_IN_ORDER
};

static ordering_t ordering = ordering_t::PERMUTE;

// Helper function to find character in string
static const char* find_char(const char* str, int chr) noexcept {
    while (*str) {
        if (*str == chr) {
            return str;
        }
        ++str;
    }
    return nullptr;
}

// Exchange two adjacent subsequences of ARGV
static void exchange_args(char** argv) noexcept {
    int bottom = first_nonopt;
    int middle = last_nonopt;
    int top = optind;

    while (top > middle && middle > bottom) {
        if (top - middle > middle - bottom) {
            // Bottom segment is shorter
            int len = middle - bottom;
            for (int i = 0; i < len; ++i) {
                std::swap(argv[bottom + i], argv[top - (middle - bottom) + i]);
            }
            top -= len;
        } else {
            // Top segment is shorter
            int len = top - middle;
            for (int i = 0; i < len; ++i) {
                std::swap(argv[bottom + i], argv[middle + i]);
            }
            bottom += len;
        }
    }

    first_nonopt += (optind - last_nonopt);
    last_nonopt = optind;
}

// Initialize getopt internal state
static const char* initialize_getopt(const char* optstring) noexcept {
    first_nonopt = last_nonopt = optind = 1;
    nextchar = nullptr;

    const char* posixly_correct = std::getenv("POSIXLY_CORRECT");

    if (optstring[0] == '-') {
        ordering = ordering_t::RETURN_IN_ORDER;
        ++optstring;
    } else if (optstring[0] == '+') {
        ordering = ordering_t::REQUIRE_ORDER;
        ++optstring;
    } else if (posixly_correct != nullptr) {
        ordering = ordering_t::REQUIRE_ORDER;
    } else {
        ordering = ordering_t::PERMUTE;
    }

    return optstring;
}

// Main getopt implementation
int tawqa_getopt_internal(int argc, char* const* argv, const char* optstring) {
    optarg = nullptr;

    if (optind == 0) {
        optstring = initialize_getopt(optstring);
    }

    if (nextchar == nullptr || *nextchar == '\0') {
        // Advance to next ARGV element
        if (ordering == ordering_t::PERMUTE) {
            if (first_nonopt != last_nonopt && last_nonopt != optind) {
                exchange_args(const_cast<char**>(argv));
            } else if (last_nonopt != optind) {
                first_nonopt = optind;
            }

            // Skip non-options
            while (optind < argc && 
                   (argv[optind][0] != '-' || argv[optind][1] == '\0')) {
                ++optind;
            }
            last_nonopt = optind;
        }

        // Handle "--" end of options
        if (optind != argc && std::strcmp(argv[optind], "--") == 0) {
            ++optind;

            if (first_nonopt != last_nonopt && last_nonopt != optind) {
                exchange_args(const_cast<char**>(argv));
            } else if (first_nonopt == last_nonopt) {
                first_nonopt = optind;
            }
            last_nonopt = argc;
            optind = argc;
        }

        // End of arguments
        if (optind == argc) {
            if (first_nonopt != last_nonopt) {
                optind = first_nonopt;
            }
            return -1;
        }

        // Handle non-option
        if (argv[optind][0] != '-' || argv[optind][1] == '\0') {
            if (ordering == ordering_t::REQUIRE_ORDER) {
                return -1;
            }
            optarg = argv[optind++];
            return 1;
        }

        nextchar = argv[optind] + 1;
    }

    // Process current option character
    char c = *nextchar++;
    const char* temp = find_char(optstring, c);

    if (*nextchar == '\0') {
        ++optind;
    }

    if (temp == nullptr || c == ':') {
        if (opterr) {
            std::fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
        }
        optopt = c;
        return '?';
    }

    if (temp[1] == ':') {
        if (temp[2] == ':') {
            // Optional argument
            if (*nextchar != '\0') {
                optarg = const_cast<char*>(nextchar);
                ++optind;
            } else {
                optarg = nullptr;
            }
            nextchar = nullptr;
        } else {
            // Required argument
            if (*nextchar != '\0') {
                optarg = const_cast<char*>(nextchar);
                ++optind;
            } else if (optind == argc) {
                if (opterr) {
                    std::fprintf(stderr, "%s: option requires an argument -- %c\n", 
                               argv[0], c);
                }
                optopt = c;
                return (optstring[0] == ':') ? ':' : '?';
            } else {
                optarg = argv[optind++];
            }
            nextchar = nullptr;
        }
    }

    return c;
}

// C-style interface
extern "C" int tawqa_getopt(int argc, char* const argv[], const char* optstring) {
    return tawqa_getopt_internal(argc, argv, optstring);
}

// Namespace implementation
namespace tawqa {

int GetOpt::getopt(int argc, char* const argv[], std::string_view optstring) {
    // Convert string_view to null-terminated string for internal use
    std::string opt_str{optstring};
    return tawqa_getopt_internal(argc, argv, opt_str.c_str());
}

std::string_view GetOpt::initialize_getopt(std::string_view optstring) {
    first_nonopt = last_nonopt = optind = 1;
    nextchar = {};

    const char* posixly_correct = std::getenv("POSIXLY_CORRECT");

    if (!optstring.empty() && optstring[0] == '-') {
        ordering = Ordering::RETURN_IN_ORDER;
        optstring.remove_prefix(1);
    } else if (!optstring.empty() && optstring[0] == '+') {
        ordering = Ordering::REQUIRE_ORDER;
        optstring.remove_prefix(1);
    } else if (posixly_correct != nullptr) {
        ordering = Ordering::REQUIRE_ORDER;
    } else {
        ordering = Ordering::PERMUTE;
    }

    return optstring;
}

std::optional<char> GetOpt::find_char_in_string(std::string_view str, char c) {
    auto pos = str.find(c);
    return (pos != std::string_view::npos) ? std::optional<char>{c} : std::nullopt;
}

} // namespace tawqa