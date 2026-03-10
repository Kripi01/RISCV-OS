# A from-scratch OS for RISC-V 64-bit

**Note:** The source code and comments are currently in French and are being progressively translated.

## Implemented Features

### 0. RAM Structure

* **Structure:** Linker script `kernel.lds`

### 1. Kernel & Bootstrapping

* **Boot:** Initialization via `crt0.S` and `enter_smode.S` then calling `kernel_start` (C code)
* **Privilege Modes:** Transition from Machine mode to Supervisor mode; execution happens in User mode (and S-mode for idle).

### 2. Memory Management

* **Paging (Virtual Memory):** Implementation of the *Sv39* standard with a 3-level page table (page walk).
* **Isolation:** Separation of kernel and user address spaces (Segfault errors are triggered).
* **Physical Memory Allocation:** Frame allocator using a *free list*.
* **Heaps (Buddy System Algorithm):**
  * **kheap:** Dynamic heap for the kernel.
  * **uheap:** Dynamic heap for user processes.

### 3. Processes

* **Scheduler:** Preemptive *Round-Robin* scheduler.
* **Process Management:**
  * States: `RUNNING`, `READY`, `SLEEPING`, `ZOMBIE`, `WAITING`.
  * Primitives: `create_process`, `waitpid`, `sleep`, `ps`.
  * `idle` process: Executed when no other process is ready (uses `wfi`).
* **Context Switching:** Saving and restoring registers via `ctx_sw`.

### 4. Interrupts & Exceptions

* **Trap Handler:**
  * Centralized management of exceptions (syscalls and page faults) and interrupts (Timer, UART interrupts raised by the PLIC).
  * Switching stacks (`mscratch`, `sscratch`, `sp`)
* **System Calls:** `ecall` interface allowing user processes to interact with the kernel (I/O, processes, FS).

### 5. File System (RAMFS)

* A simple file system residing entirely in RAM.
* Support for directories and files (WIP).
* Commands: `mkdir`, `ls`, `pwd`, `cd`, `rm`.

### 6. Drivers & I/O

* **UART:** Serial communication for logs and text-mode keyboard input.
* **Screen (Framebuffer):** Graphic support via `bochs-display` and PCIe with custom font rendering (`font.c`).

### 7. User Interface

* **Shell (bash custom):** CLI to navigate the FS and launch processes.
  * Commands: `echo`, `clear`, `bash`, `... &` (background process), etc

---

## Getting Started

### Prerequisites

* RISC-V Toolchain: `gcc-riscv64-unknown-elf`
* Emulator: `qemu-system-riscv64`
* Make: `make`
* GDB (optional): `gdb-multiarch`

### Start the kernel

* **Console mode (nographic):**

  ```bash
  make run
  ```

* **Graphic mode:**

  ```bash
  make rung
  ```

### Debugging

```bash
make debug  # console
make debugg # graphic
```
