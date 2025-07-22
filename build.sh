#!/bin/bash

# TAWQA Build Script
# Unified build system for all versions

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
RUST_BINARY="target/release/tawqa"
CPP_BINARY="tawqa_cpp"
HYBRID_BINARY="tawqa_hybrid"

# Functions
print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}  TAWQA Unified Build System${NC}"
    echo -e "${BLUE}================================${NC}"
    echo
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

install_rust() {
    print_info "Installing Rust..."
    if command -v curl &> /dev/null; then
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
        source ~/.cargo/env
        print_success "Rust installed successfully"
    else
        print_error "curl not found. Please install curl first"
        exit 1
    fi
}

check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check for Rust
    if command -v cargo &> /dev/null; then
        print_success "Rust/Cargo found"
    else
        print_warning "Rust/Cargo not found"
        read -p "Do you want to install Rust automatically? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_rust
        else
            print_error "Rust is required. Please install from https://rustup.rs/"
            exit 1
        fi
    fi
    
    # Update Rust dependencies
    print_info "Updating Rust dependencies..."
    if cargo update; then
        print_success "Rust dependencies updated"
    else
        print_warning "Failed to update Rust dependencies"
    fi
    
    # Check for C++ compiler
    if command -v g++ &> /dev/null; then
        print_success "G++ compiler found"
    else
        print_error "G++ compiler not found. Please install build-essential"
        exit 1
    fi
    
    # Check for Make
    if command -v make &> /dev/null; then
        print_success "Make found"
    else
        print_error "Make not found. Please install build-essential"
        exit 1
    fi
    
    echo
}

build_rust() {
    print_info "Building Rust version..."
    if cargo build --release; then
        print_success "Rust version built successfully"
        return 0
    else
        print_error "Failed to build Rust version"
        return 1
    fi
}

build_cpp() {
    print_info "Building C++ classic version..."
    if make -f Makefile.unified cpp; then
        print_success "C++ classic version built successfully"
        return 0
    else
        print_error "Failed to build C++ classic version"
        return 1
    fi
}

# C++20 extended version removed

build_hybrid() {
    print_info "Building hybrid version..."
    if make -f Makefile.unified hybrid; then
        print_success "Hybrid version built successfully"
        return 0
    else
        print_error "Failed to build hybrid version"
        return 1
    fi
}

test_versions() {
    print_info "Testing built versions..."
    local failed=0
    
    # Test Rust version
    if [ -f "$RUST_BINARY" ]; then
        if ./$RUST_BINARY --help &> /dev/null; then
            print_success "Rust version test passed"
        else
            print_error "Rust version test failed"
            failed=1
        fi
    else
        print_warning "Rust version not found, skipping test"
    fi
    
    # Test C++ classic version
    if [ -f "$CPP_BINARY" ]; then
        if ./$CPP_BINARY -h &> /dev/null; then
            print_success "C++ classic version test passed"
        else
            print_error "C++ classic version test failed"
            failed=1
        fi
    else
        print_warning "C++ classic version not found, skipping test"
    fi
    
    # C++20 extended version removed
    
    # Test hybrid version
    if [ -f "$HYBRID_BINARY" ]; then
        if ./$HYBRID_BINARY --help &> /dev/null; then
            print_success "Hybrid version test passed"
        else
            print_error "Hybrid version test failed"
            failed=1
        fi
    else
        print_warning "Hybrid version not found, skipping test"
    fi
    
    return $failed
}

show_status() {
    print_info "Build status:"
    echo
    
    echo "Available executables:"
    [ -f "$RUST_BINARY" ] && echo -e "  ${GREEN}✓${NC} $RUST_BINARY (Rust version)" || echo -e "  ${RED}✗${NC} $RUST_BINARY (Rust version)"
    [ -f "$CPP_BINARY" ] && echo -e "  ${GREEN}✓${NC} $CPP_BINARY (C++ classic)" || echo -e "  ${RED}✗${NC} $CPP_BINARY (C++ classic)"
    [ -f "$HYBRID_BINARY" ] && echo -e "  ${GREEN}✓${NC} $HYBRID_BINARY (Hybrid C++/Rust)" || echo -e "  ${RED}✗${NC} $HYBRID_BINARY (Hybrid C++/Rust)"
    
    echo
}

clean_all() {
    print_info "Cleaning all build artifacts..."
    make -f Makefile.unified clean
    print_success "All build artifacts cleaned"
}

show_help() {
    echo "TAWQA Build Script"
    echo
    echo "Usage: $0 [COMMAND]"
    echo
    echo "Commands:"
    echo "  all      Build all versions (default)"
    echo "  rust     Build Rust version only"
    echo "  cpp      Build C++ classic version only"
    echo "  hybrid   Build hybrid version only"
    echo "  test     Test all built versions"
    echo "  clean    Clean all build artifacts"
    echo "  status   Show build status"
    echo "  deps     Check dependencies"
    echo "  help     Show this help"
    echo
    echo "Examples:"
    echo "  $0           # Build all versions"
    echo "  $0 rust      # Build only Rust version"
    echo "  $0 clean all # Clean and rebuild everything"
    echo "  $0 test      # Test all versions"
}

# Main script logic
main() {
    print_header
    
    case "${1:-all}" in
        "all")
            check_dependencies
            build_rust
            build_cpp
            build_hybrid
            echo
            show_status
            echo
            print_info "Running tests..."
            test_versions
            ;;
        "rust")
            check_dependencies
            build_rust
            ;;
        "cpp")
            check_dependencies
            build_cpp
            ;;
        "hybrid")
            check_dependencies
            build_rust  # Hybrid needs Rust backend
            build_hybrid
            ;;
        "test")
            test_versions
            ;;
        "clean")
            clean_all
            ;;
        "status")
            show_status
            ;;
        "deps")
            check_dependencies
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown command: $1"
            echo
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"