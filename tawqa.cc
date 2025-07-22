// TAWQA Main Implementation
// Modern C++23 port of netcat without OOP
// Using TAWQA prefix to avoid naming conflicts

#include "tawqa_generic.hh"
#include "tawqa_getopt.hh"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <array>
#include <string_view>
#include <span>
#include <iostream>
#include <iomanip>

using namespace std::string_view_literals;

// Constants
constexpr std::uint16_t TAWQA_SLEAZE_PORT = 31337;
constexpr std::size_t TAWQA_BIGSIZ = 8192;
constexpr std::size_t TAWQA_SMALLSIZ = 256;
constexpr std::size_t TAWQA_MAXHOSTNAMELEN = 256;

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

// Type aliases for better readability
using tawqa_socket_t = int;
using tawqa_port_t = std::uint16_t;

// Host information structure
struct tawqa_host_info {
    std::array<char, TAWQA_MAXHOSTNAMELEN> name;
    std::array<std::array<char, 24>, 8> addrs;
    std::array<struct in_addr, 8> iaddrs;
};

// Port information structure  
struct tawqa_port_info {
    std::array<char, 64> name;
    std::array<char, 8> anum;
    tawqa_port_t num;
};

// Global state variables
static tawqa_socket_t g_netfd = -1;
static int g_ofd = 0;
static std::array<char, 20> g_unknown = {"(UNKNOWN)"};
static std::array<char, 4> g_tcp = {"tcp"};
static std::array<char, 4> g_udp = {"udp"};

// Command line flags
static bool g_verbose = false;
static bool g_listen = false;
static bool g_numeric = false;
static bool g_udp_mode = false;
static bool g_zero_io = false;
static std::uint32_t g_wait_time = 0;
static std::uint32_t g_interval = 0;

// Buffer management
static std::array<char, TAWQA_BIGSIZ> g_bigbuf_in;
static std::array<char, TAWQA_BIGSIZ> g_bigbuf_net;

// Statistics
static std::uint32_t g_wrote_out = 0;
static std::uint32_t g_wrote_net = 0;

// Forward declarations (implementations below)

// Error reporting function
void tawqa_holler(const char* str, const char* p1, const char* p2, const char* p3) {
    if (g_verbose) {
        std::fprintf(stderr, str, p1, p2, p3);
        if (errno) {
            perror(" ");
        } else {
            std::fprintf(stderr, "\n");
        }
        std::fflush(stderr);
    }
}

// Fatal error function
void tawqa_bail(const char* str, const char* p1, const char* p2, const char* p3) {
    g_verbose = true;
    tawqa_holler(str, p1, p2, p3);
    if (g_netfd >= 0) {
        close(g_netfd);
    }
    std::exit(1);
}

// Signal handler
static void tawqa_catch_signal(int sig) {
    static char sig_str[16], net_str[16], out_str[16];
    if (g_verbose > 1) {
        snprintf(sig_str, sizeof(sig_str), "%d", sig);
        snprintf(net_str, sizeof(net_str), "%u", g_wrote_net);
        snprintf(out_str, sizeof(out_str), "%u", g_wrote_out);
        tawqa_bail("Caught signal %s, sent %s, rcvd %s", sig_str, net_str, out_str);
    }
    tawqa_bail("Interrupted!");
}

// Memory allocation wrapper
static void* tawqa_malloc(std::size_t size) {
    std::size_t aligned_size = (size + 3) & ~3; // Align to 4 bytes
    void* ptr = std::calloc(1, aligned_size);
    if (!ptr) {
        static char size_str[32];
        snprintf(size_str, sizeof(size_str), "%zu", aligned_size);
        tawqa_bail("malloc %s failed", size_str);
    }
    return ptr;
}

// Find newline in buffer
static std::size_t tawqa_findline(const char* buf, std::size_t size) {
    if (!buf || size > TAWQA_BIGSIZ) {
        return 0;
    }
    
    for (std::size_t i = 0; i < size; ++i) {
        if (buf[i] == '\n') {
            return i + 1;
        }
    }
    return size;
}

// Resolve hostname
static tawqa_host_info* tawqa_gethostpoop(const char* name, bool numeric_only) {
    auto* poop = static_cast<tawqa_host_info*>(tawqa_malloc(sizeof(tawqa_host_info)));
    
    std::strncpy(poop->name.data(), g_unknown.data(), poop->name.size() - 1);
    
    struct in_addr iaddr;
    iaddr.s_addr = inet_addr(name);
    
    if (iaddr.s_addr == INADDR_NONE) {
        // Name resolution
        if (numeric_only) {
            tawqa_bail("Can't parse %s as an IP address", name);
        }
        
        struct hostent* hostent = gethostbyname(name);
        if (!hostent) {
            tawqa_bail("%s: forward host lookup failed", name);
        }
        
        std::strncpy(poop->name.data(), hostent->h_name, poop->name.size() - 1);
        
        for (int x = 0; hostent->h_addr_list[x] && x < 8; ++x) {
            std::memcpy(&poop->iaddrs[x], hostent->h_addr_list[x], sizeof(struct in_addr));
            std::strncpy(poop->addrs[x].data(), inet_ntoa(poop->iaddrs[x]), 
                        poop->addrs[x].size() - 1);
        }
    } else {
        // Numeric address
        poop->iaddrs[0] = iaddr;
        std::strncpy(poop->addrs[0].data(), inet_ntoa(iaddr), 
                    poop->addrs[0].size() - 1);
        
        if (!numeric_only && g_verbose) {
            struct hostent* hostent = gethostbyaddr(
                reinterpret_cast<const char*>(&iaddr), 
                sizeof(struct in_addr), AF_INET);
            if (hostent) {
                std::strncpy(poop->name.data(), hostent->h_name, 
                            poop->name.size() - 1);
            }
        }
    }
    
    return poop;
}

// Resolve port number
static tawqa_port_t tawqa_getportpoop(const char* pstring, std::uint32_t pnum) {
    static tawqa_port_info portpoop;
    
    portpoop.name[0] = '?';
    portpoop.name[1] = '\0';
    
    const char* protocol = g_udp_mode ? g_udp.data() : g_tcp.data();
    
    if (pnum) {
        auto port = static_cast<tawqa_port_t>(pnum);
        if (!g_numeric) {
            struct servent* servent = getservbyport(htons(port), protocol);
            if (servent) {
                std::strncpy(portpoop.name.data(), servent->s_name, 
                            portpoop.name.size() - 1);
            }
        }
        std::snprintf(portpoop.anum.data(), portpoop.anum.size(), "%u", port);
        portpoop.num = port;
        return port;
    }
    
    if (pstring) {
        auto port = static_cast<tawqa_port_t>(std::atoi(pstring));
        if (port) {
            return tawqa_getportpoop(nullptr, port);
        }
        
        if (!g_numeric) {
            struct servent* servent = getservbyname(pstring, protocol);
            if (servent) {
                std::strncpy(portpoop.name.data(), servent->s_name, 
                            portpoop.name.size() - 1);
                port = ntohs(servent->s_port);
                std::snprintf(portpoop.anum.data(), portpoop.anum.size(), "%u", port);
                portpoop.num = port;
                return port;
            }
        }
    }
    
    return 0;
}

// Create and configure socket
static tawqa_socket_t tawqa_doconnect(struct in_addr* raddr, tawqa_port_t rport,
                                      struct in_addr* laddr, tawqa_port_t lport) {
    tawqa_socket_t nnetfd;
    
    if (g_udp_mode) {
        nnetfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    } else {
        nnetfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    
    if (nnetfd < 0) {
        tawqa_bail("Can't get socket");
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(nnetfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        tawqa_holler("setsockopt reuseaddr failed");
    }
    
    // Local binding
    struct sockaddr_in lclend = {};
    lclend.sin_family = AF_INET;
    
    if (laddr) {
        lclend.sin_addr = *laddr;
    }
    if (lport) {
        lclend.sin_port = htons(lport);
    }
    
    if (laddr || lport) {
        if (bind(nnetfd, reinterpret_cast<struct sockaddr*>(&lclend), 
                sizeof(lclend)) < 0) {
            static char port_str[16];
            snprintf(port_str, sizeof(port_str), "%u", lport);
            tawqa_bail("Can't bind to %s:%s", 
                      laddr ? inet_ntoa(*laddr) : "0.0.0.0", port_str);
        }
    }
    
    if (g_listen) {
        return nnetfd;
    }
    
    // Remote connection
    struct sockaddr_in remend = {};
    remend.sin_family = AF_INET;
    remend.sin_addr = *raddr;
    remend.sin_port = htons(rport);
    
    if (connect(nnetfd, reinterpret_cast<struct sockaddr*>(&remend), 
               sizeof(remend)) < 0) {
        static char port_str[16];
        snprintf(port_str, sizeof(port_str), "%u", rport);
        tawqa_bail("Can't connect to %s:%s", inet_ntoa(*raddr), port_str);
    }
    
    return nnetfd;
}

// Main network loop
static void tawqa_readwrite(tawqa_socket_t netfd) {
    fd_set readfds;
    int maxfd = std::max(netfd, STDIN_FILENO) + 1;
    
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(netfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        struct timeval timeout = {1, 0}; // 1 second timeout
        int ready = select(maxfd, &readfds, nullptr, nullptr, &timeout);
        
        if (ready < 0) {
            if (errno == EINTR) continue;
            tawqa_bail("select failed");
        }
        
        if (ready == 0) continue; // timeout
        
        // Handle network -> stdout
        if (FD_ISSET(netfd, &readfds)) {
            ssize_t bytes = recv(netfd, g_bigbuf_net.data(), g_bigbuf_net.size(), 0);
            if (bytes <= 0) {
                if (g_verbose) {
                    tawqa_holler("Network connection closed");
                }
                break;
            }
            
            ssize_t written = write(STDOUT_FILENO, g_bigbuf_net.data(), bytes);
            if (written > 0) {
                g_wrote_out += written;
            }
        }
        
        // Handle stdin -> network
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            ssize_t bytes = read(STDIN_FILENO, g_bigbuf_in.data(), g_bigbuf_in.size());
            if (bytes <= 0) {
                if (g_verbose) {
                    tawqa_holler("stdin closed");
                }
                break;
            }
            
            ssize_t sent = send(netfd, g_bigbuf_in.data(), bytes, 0);
            if (sent > 0) {
                g_wrote_net += sent;
            }
        }
    }
}

// Help text
static void tawqa_help() {
    printf("TAWQA (The Almighty Wonderful Quite Adequate) netcat\n");
    printf("Usage: tawqa [options] hostname port[s]\n");
    printf("       tawqa -l -p port [options] [hostname] [port]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -l          Listen mode, for inbound connects\n");
    printf("  -p port     Local port number\n");
    printf("  -u          UDP mode\n");
    printf("  -v          Verbose [use twice to be more verbose]\n");
    printf("  -w secs     Timeout for connects and final net reads\n");
    printf("  -z          Zero-I/O mode [used for scanning]\n");
    printf("  -n          Numeric-only IP addresses, no DNS\n");
    printf("  -h          This help text\n");
    printf("\n");
    printf("Port numbers can be individual or ranges: lo-hi [inclusive]\n");
}

// Main function
int main(int argc, char* argv[]) {
    std::signal(SIGINT, tawqa_catch_signal);
    std::signal(SIGTERM, tawqa_catch_signal);
    
    // Parse command line options
    int opt;
    tawqa_port_t local_port = 0;
    const char* program_path = nullptr;
    
    while ((opt = tawqa_getopt(argc, argv, "lp:uvw:znhe:")) != -1) {
        switch (opt) {
            case 'l':
                g_listen = true;
                break;
            case 'p':
                local_port = tawqa_getportpoop(optarg, 0);
                break;
            case 'u':
                g_udp_mode = true;
                break;
            case 'v':
                g_verbose = true;
                break;
            case 'w':
                g_wait_time = std::atoi(optarg);
                break;
            case 'z':
                g_zero_io = true;
                break;
            case 'n':
                g_numeric = true;
                break;
            case 'h':
                tawqa_help();
                return 0;
            case 'e':
                program_path = optarg;
                tawqa_set_program_path(program_path);
                break;
            default:
                tawqa_help();
                return 1;
        }
    }
    
    // Parse remaining arguments
    if (optind >= argc && !g_listen) {
        tawqa_help();
        return 1;
    }
    
    const char* hostname = nullptr;
    tawqa_port_t remote_port = 0;
    
    if (optind < argc) {
        hostname = argv[optind++];
    }
    
    if (optind < argc) {
        remote_port = tawqa_getportpoop(argv[optind], 0);
    }
    
    // Resolve addresses
    tawqa_host_info* remote_host = nullptr;
    if (hostname) {
        remote_host = tawqa_gethostpoop(hostname, g_numeric);
    }
    
    // Create connection
    g_netfd = tawqa_doconnect(
        remote_host ? &remote_host->iaddrs[0] : nullptr,
        remote_port,
        nullptr,
        local_port
    );
    
    if (g_listen) {
        if (listen(g_netfd, 1) < 0) {
            tawqa_bail("listen failed");
        }
        
        if (g_verbose) {
            static char port_str[16];
            snprintf(port_str, sizeof(port_str), "%u", local_port);
            tawqa_holler("Listening on port %s", port_str);
        }
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(g_netfd, 
                              reinterpret_cast<struct sockaddr*>(&client_addr), 
                              &client_len);
        if (client_fd < 0) {
            tawqa_bail("accept failed");
        }
        
        if (g_verbose) {
            static char port_str[16];
            snprintf(port_str, sizeof(port_str), "%u", ntohs(client_addr.sin_port));
            tawqa_holler("Connection from %s:%s", 
                        inet_ntoa(client_addr.sin_addr), port_str);
        }
        
        close(g_netfd);
        g_netfd = client_fd;
        
        if (program_path) {
            tawqa_doexec(g_netfd);
            return 0;
        }
    }
    
    if (g_zero_io) {
        close(g_netfd);
        return 0;
    }
    
    // Main I/O loop
    tawqa_readwrite(g_netfd);
    
    if (g_verbose) {
        static char net_str[16], out_str[16];
        snprintf(net_str, sizeof(net_str), "%u", g_wrote_net);
        snprintf(out_str, sizeof(out_str), "%u", g_wrote_out);
        tawqa_holler("Total: sent %s, received %s", net_str, out_str);
    }
    
    close(g_netfd);
    if (remote_host) {
        std::free(remote_host);
    }
    
    return 0;
}