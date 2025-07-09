# ChthonOS - AArch64 Operating System

**Version 1.0 "Tehom"**

A minimal operating system for AArch64 (ARM64) architecture. Microkernel design with Plan 9 inspired message-passing IPC and 9p, virtual filesystem, and namespace support.

## Features

- **AArch64 Architecture**: Native 64-bit ARM support
- **Message-Passing IPC**: Process communication via structured messages
- **Virtual Filesystem (VFS)**: Pluggable filesystem architecture
- **Multiple Filesystems**: RAM-based and AbyssFS implementations
- **Memory Management**: MMU setup with identity mapping and kernel heap
- **Process Management**: Multi-process support with scheduling
- **Exception Handling**: Comprehensive exception and interrupt handling
- **User Mode Support**: EL0 user programs with system call interface
- **Shell Interface**: Interactive command-line environment 

## Project Structure

```
OS-public/
├── src/                      # Source code
│   ├── arch/                 # Architecture-specific code (AArch64)
│   │   ├── boot.S           # Boot assembly and startup
│   │   ├── exceptions.S     # Exception vector table
│   │   ├── exceptions.c     # Exception handlers
│   │   ├── context_switch.S # Process context switching
│   │   ├── enter_usermode.S # EL1 -> EL0 transition
│   │   ├── mmu.c           # Memory Management Unit setup
│   │   └── user_shell.S    # User mode assembly
│   │
│   ├── kernel/              # Core kernel functionality
│   │   ├── kernel.c         # Main kernel initialization
│   │   ├── process.c        # Process management
│   │   ├── message.c        # Inter-process communication
│   │   └── namespace.c      # Namespace management
│   │
│   ├── drivers/             # Device drivers
│   │   ├── uart.c           # Serial UART driver
│   │   ├── uart_debug.c     # UART debugging utilities
│   │   ├── timer.c          # System timer driver
│   │   └── gic.c            # Generic Interrupt Controller
│   │
│   ├── fs/                  # Filesystem implementations
│   │   ├── vfs.c            # Virtual filesystem layer
│   │   ├── ramfs.c          # RAM-based filesystem
│   │   └── abyssfs.c        # AbyssFS implementation
│   │
│   ├── mm/                  # Memory management
│   │   ├── kmalloc.c        # Kernel memory allocator
│   │   └── string.c         # String manipulation utilities
│   │
│   └── user/                # User programs
│       ├── shell.c          # Interactive shell
│       └── process_test.c   # Process testing utilities
│
├── include/                 # Header files
│   └── *.h                 # System headers
│
├── Makefile                # Build system
├── linker.ld              # Kernel linker script
├── user_linker.ld         # User program linker script
└── README.md              # This file
```

## Build Requirements

- **LLVM Toolchain**: Version 20.1.7 or compatible
  - `clang` (C/assembly compiler)
  - `lld` (LLVM linker)
- **QEMU**: For emulation and testing
- **Make**: Build automation

### macOS Installation
```bash
# Install LLVM toolchain via Homebrew
brew install llvm

# Install LLD linker (required for linking)
brew install lld

# Install QEMU for emulation
brew install qemu
```

### Verify Installation
```bash
# Check LLVM tools
clang --version
which lld

# Check QEMU
qemu-system-aarch64 --version
```

**Note**: The build system uses `clang` for compilation and `lld` for linking with the `-fuse-ld=lld` flag. Both packages are required for successful builds.

## Building

### Quick Build
```bash
make
```

### Available Targets
```bash
make all        # Build the kernel (default)
make clean      # Remove all build artifacts  
make rebuild    # Clean and build
make help       # Show available targets
```

### Build Output
- **kernel.elf**: Main bootable kernel image
- **\*.o**: Object files (intermediate build artifacts)

## Running

### QEMU Emulation
```bash
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 2048 -nographic -kernel kernel.elf
```

#### QEMU Parameters Explained
- `-machine virt`: Use ARM Versatile Platform Board
- `-cpu cortex-a72`: Emulate ARM Cortex-A72 processor
- `-m 2048`: Allocate 2GB of RAM
- `-nographic`: Disable graphical output (console only)
- `-kernel kernel.elf`: Boot the specified kernel image

### Alternative QEMU Command (with serial monitoring)
```bash
qemu-system-aarch64 -M virt -cpu cortex-a72 -m 2048 -nographic -serial mon:stdio -kernel kernel.elf
```

## Boot Sequence

1. **Boot Assembly** (`src/arch/boot.S`): Initial setup and jump to C code
2. **Kernel Main** (`src/kernel/kernel.c`): Core initialization
3. **MMU Setup** (`src/arch/mmu.c`): Memory management configuration
4. **Driver Initialization**: UART, timer, GIC setup
5. **Filesystem Mount**: VFS and filesystem initialization
6. **Process Creation**: User mode processes and shell
7. **Scheduler**: Multi-process execution begins

## System Architecture

### Memory Layout
- **0x00000000 - 0x3FFFFFFF**: Device memory (1GB)
- **0x40000000 - 0x7FFFFFFF**: Kernel space (1GB) 
- **0x80000000 - 0xBFFFFFFF**: User space (1GB)

### Exception Levels
- **EL1**: Kernel/supervisor mode
- **EL0**: User mode applications

### IPC Mechanism
Processes communicate via structured messages supporting:
- File operations (open, read, write, create)
- Process control (fork, exec, wait)
- Directory operations (mkdir, chdir, getcwd)
- File management (copy, move, remove)

## Development

### Adding New Features
1. **Drivers**: Add to `src/drivers/` and update Makefile
2. **Filesystems**: Implement VFS interface in `src/fs/`
3. **System Calls**: Extend message types in `include/message.h`
4. **User Programs**: Add to `src/user/`

### Debugging
- Use UART output for kernel debugging
- QEMU monitor commands for system inspection
- GDB integration supported via QEMU

## License

This project is licensed under the BSD 2-Clause License - see below for details.

```
BSD 2-Clause License

Copyright (c) 2025, ChthonOS Project
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Contributing

ChthonOS v1.0 (codename: **Tehom**) is the stable educational operating system designed for learning purposes and inclusion in an upcoming book on OS development. 

### Current Status
- **Tehom (v1.0)**: Complete, stable educational OS - this branch/release
- **Melinoe (v2.0)**: Active development with VirtIO, enhanced features - separate branches

This codebase represents the **stable v1.0 release** that serves as the foundation for understanding operating system concepts. Tehom has fulfilled its primary purpose as a teaching tool.

### Repository Organization
```
Branches:
├── tehom-*          # v1.0 stable branches (educational)
├── melinoe-*        # v2.0 development branches (advanced features)  
└── main            # current stable release
```

### Types of Contributions 
- **Bug fixes**: Corrections to existing functionality
- **Documentation improvements**: Better explanations, typo fixes, clarity enhancements  
- **Educational enhancements**: Improved comments, clearer examples
- **Build system fixes**: Platform compatibility, dependency issues

### Future Development
Active collaborative development continues in **Melinoe v2.0** branches, which build upon the concepts established in this Tehom v1.0 release. Melinoe includes VirtIO support, enhanced filesystems, and extended functionality designed for broader community contributions.

### How to Contribute
1. Fork the repository
2. Create a feature branch for your fix/improvement
3. Make your changes with clear, educational comments
4. Test thoroughly on the target platform
5. Submit a pull request with a detailed description

Please keep in mind this project's educational nature - contributions should maintain code clarity and learning value over optimization.

## References

- ARM Architecture Reference Manual ARMv8
- QEMU Documentation
- LLVM/Clang Documentation


