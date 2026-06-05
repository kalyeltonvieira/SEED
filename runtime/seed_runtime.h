#ifndef SEED_RUNTIME_H
#define SEED_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

// Bytecode opcodes
#define OP_ALLOC  0x01
#define OP_FREE   0x02
#define OP_READ   0x03
#define OP_WRITE  0x04
#define OP_CMP    0x05
#define OP_JMP    0x06
#define OP_SYSCALL 0x07

// VM configuration
#define REG_COUNT 16
#define STACK_SIZE 4096
#define HEAP_SIZE (1024 * 1024)  // 1MB

// VM state
typedef struct {
    uint64_t regs[REG_COUNT];
    uint64_t stack[STACK_SIZE];
    uint8_t* heap;
    uint64_t* bytecode;
    size_t bytecode_len;
    size_t sp;
    size_t heap_cursor;
} SeedVM;

// Native function signatures
typedef void (*NativeFunc)(SeedVM* vm);

// VM functions
int vm_init(SeedVM* vm);
void vm_free(SeedVM* vm);
int vm_load_bytecode(SeedVM* vm, const char* filename);
int vm_execute(SeedVM* vm);

// Native functions
void native_print(SeedVM* vm);
void native_read_line(SeedVM* vm);
void native_file_open(SeedVM* vm);
void native_file_read(SeedVM* vm);
void native_file_write(SeedVM* vm);
void native_file_close(SeedVM* vm);
void native_malloc(SeedVM* vm);
void native_free(SeedVM* vm);

// Syscall handlers
void syscall_print(SeedVM* vm);
void syscall_read_line(SeedVM* vm);
void syscall_file_open(SeedVM* vm);
void syscall_file_read(SeedVM* vm);
void syscall_file_write(SeedVM* vm);
void syscall_file_close(SeedVM* vm);
void syscall_exit(SeedVM* vm);

#endif // SEED_RUNTIME_H
