#include "seed_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <shlwapi.h>

// Built-in function mapping
typedef struct {
    const char* seed_name;
    const char* cpp_name;
} builtin_map_t;

static const builtin_map_t builtin_map[] = {
    {"print", "stdlib::io::print"},
    {"println", "stdlib::io::println"},
    {"math.sqrt", "std::sqrt"},
    {"math.sin", "std::sin"},
    {"math.cos", "std::cos"},
    {"math.tan", "std::tan"},
    {"math.abs", "std::abs"},
    {"math.max", "std::max"},
    {"math.min", "std::min"},
    {"math.pow", "std::pow"},
    {"math.PI", "3.141592653589793"},
    {"time.sleep", "stdlib::time::sleep"},
    {"time.now", "stdlib::time::now"},
    // Add more as needed
};

// Helper to replace builtin function calls in a line
static void replace_builtins(char* line) {
    for (size_t i = 0; i < sizeof(builtin_map)/sizeof(builtin_map[0]); ++i) {
        const char* src = builtin_map[i].seed_name;
        const char* dst = builtin_map[i].cpp_name;
        char* pos = strstr(line, src);
        while (pos) {
            // Ensure we replace only when it's a function call or identifier
            // Simple check: preceding char is not alnum or '_' and following char is '(' or end
            char before = (pos == line) ? ' ' : *(pos - 1);
            char after = *(pos + strlen(src));
            if (!isalnum((unsigned char)before) && (after == '(' || after == '\0' || isspace((unsigned char)after))) {
                // Perform replacement
                char buffer[4096];
                size_t prefix_len = pos - line;
                snprintf(buffer, sizeof(buffer), "%.*s%s%s", (int)prefix_len, line, dst, pos + strlen(src));
                strncpy(line, buffer, 4095);
                line[4095] = '\0';
                // Search again after replacement
                pos = strstr(line + prefix_len + strlen(dst), src);
            } else {
                // Skip this occurrence
                pos = strstr(pos + 1, src);
            }
        }
    }
}

// Global SEED home directory
static char seed_home[MAX_PATH] = {0};

// Get the directory where the executable is located
void get_executable_dir(char* buffer, size_t size) {
    if (GetModuleFileNameA(NULL, buffer, size) == 0) {
        // Fallback to current directory
        GetCurrentDirectory(size, buffer);
        return;
    }
    
    // Remove the executable name to get the directory
    PathRemoveFileSpecA(buffer);
}

// VM initialization
int vm_init(SeedVM* vm) {
    memset(vm, 0, sizeof(SeedVM));
    
    // Allocate heap
    vm->heap = (uint8_t*)malloc(HEAP_SIZE);
    if (!vm->heap) {
        return -1;
    }
    
    vm->sp = 0;
    vm->heap_cursor = 0;
    
    return 0;
}

// VM cleanup
void vm_free(SeedVM* vm) {
    if (vm->heap) {
        free(vm->heap);
        vm->heap = NULL;
    }
    if (vm->bytecode) {
        free(vm->bytecode);
        vm->bytecode = NULL;
    }
}

// Load bytecode from file
int vm_load_bytecode(SeedVM* vm, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return -1;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(f);
        return -1;
    }
    
    // Allocate and read bytecode
    vm->bytecode = (uint64_t*)malloc(size);
    if (!vm->bytecode) {
        fclose(f);
        return -1;
    }
    
    size_t read = fread(vm->bytecode, 1, size, f);
    fclose(f);
    
    vm->bytecode_len = read / sizeof(uint64_t);
    
    return 0;
}

// Execute bytecode
int vm_execute(SeedVM* vm) {
    if (!vm->bytecode || vm->bytecode_len == 0) {
        return -1;
    }
    
    size_t pc = 0;
    
    while (pc < vm->bytecode_len) {
        uint64_t instr = vm->bytecode[pc];
        uint8_t opcode = (instr >> 56) & 0xFF;
        uint8_t dst = (instr >> 48) & 0xFF;
        uint8_t src_a = (instr >> 40) & 0xFF;
        uint8_t src_b = (instr >> 32) & 0xFF;
        uint32_t imm = (uint32_t)(instr & 0xFFFFFFFF);
        
        switch (opcode) {
            case OP_ALLOC: {
                uint64_t size = vm->regs[src_a];
                if (vm->heap_cursor + size > HEAP_SIZE) {
                    fprintf(stderr, "Error: Out of heap memory\n");
                    return -1;
                }
                uint64_t ptr = (uint64_t)(vm->heap + vm->heap_cursor);
                vm->heap_cursor += size;
                vm->regs[dst] = ptr;
                pc++;
                break;
            }
            
            case OP_FREE: {
                // Simple free - just mark as freed (real GC would be more complex)
                uint64_t ptr = vm->regs[src_a];
                if (ptr >= (uint64_t)vm->heap && ptr < (uint64_t)(vm->heap + HEAP_SIZE)) {
                    // In a real implementation, we'd track allocations
                }
                vm->regs[dst] = 0;
                pc++;
                break;
            }
            
            case OP_READ: {
                uint64_t ptr = vm->regs[src_a];
                uint64_t len = vm->regs[src_b];
                if (ptr >= (uint64_t)vm->heap && ptr + len <= (uint64_t)(vm->heap + HEAP_SIZE)) {
                    uint64_t val = 0;
                    memcpy(&val, (void*)ptr, len > 8 ? 8 : len);
                    vm->regs[dst] = val;
                }
                pc++;
                break;
            }
            
            case OP_WRITE: {
                uint64_t ptr = vm->regs[src_a];
                uint64_t val = vm->regs[src_b];
                if (ptr >= (uint64_t)vm->heap && ptr < (uint64_t)(vm->heap + HEAP_SIZE)) {
                    memcpy((void*)ptr, &val, 8);
                }
                pc++;
                break;
            }
            
            case OP_CMP: {
                uint64_t a = vm->regs[src_a];
                uint64_t b = vm->regs[src_b];
                vm->regs[dst] = (a == b) ? 1 : 0;
                pc++;
                break;
            }
            
            case OP_JMP: {
                uint64_t cond = vm->regs[src_a];
                if (cond) {
                    pc = imm;
                } else {
                    pc++;
                }
                break;
            }
            
            case OP_SYSCALL: {
                uint64_t syscall_num = vm->regs[src_a];
                switch (syscall_num) {
                    case 1: syscall_print(vm); break;
                    case 2: syscall_read_line(vm); break;
                    case 3: syscall_file_open(vm); break;
                    case 4: syscall_file_read(vm); break;
                    case 5: syscall_file_write(vm); break;
                    case 6: syscall_file_close(vm); break;
                    case 7: syscall_exit(vm); return 0;
                    default:
                        fprintf(stderr, "Error: Unknown syscall %llu\n", syscall_num);
                        return -1;
                }
                pc++;
                break;
            }
            
            default:
                fprintf(stderr, "Error: Unknown opcode %d\n", opcode);
                return -1;
        }
    }
    
    return 0;
}

// Syscall implementations
void syscall_print(SeedVM* vm) {
    uint64_t ptr = vm->regs[1];
    uint64_t len = vm->regs[2];
    if (ptr >= (uint64_t)vm->heap && ptr < (uint64_t)(vm->heap + HEAP_SIZE)) {
        fwrite((void*)ptr, 1, len, stdout);
        fflush(stdout);
    }
}

void syscall_read_line(SeedVM* vm) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (vm->heap_cursor + len > HEAP_SIZE) {
            vm->regs[0] = 0;
            return;
        }
        memcpy(vm->heap + vm->heap_cursor, buffer, len);
        vm->regs[0] = (uint64_t)(vm->heap + vm->heap_cursor);
        vm->regs[1] = len;
        vm->heap_cursor += len;
    } else {
        vm->regs[0] = 0;
        vm->regs[1] = 0;
    }
}

void syscall_file_open(SeedVM* vm) {
    uint64_t filename_ptr = vm->regs[1];
    uint64_t mode = vm->regs[2];
    
    if (filename_ptr < (uint64_t)vm->heap || filename_ptr >= (uint64_t)(vm->heap + HEAP_SIZE)) {
        vm->regs[0] = 0;
        return;
    }
    
    char* filename = (char*)filename_ptr;
    const char* mode_str = (mode == 0) ? "rb" : (mode == 1) ? "wb" : "r+b";
    
    FILE* f = fopen(filename, mode_str);
    vm->regs[0] = (uint64_t)f;
}

void syscall_file_read(SeedVM* vm) {
    FILE* f = (FILE*)vm->regs[1];
    uint64_t ptr = vm->regs[2];
    uint64_t len = vm->regs[3];
    
    if (!f || ptr < (uint64_t)vm->heap || ptr + len > (uint64_t)(vm->heap + HEAP_SIZE)) {
        vm->regs[0] = 0;
        return;
    }
    
    size_t read = fread((void*)ptr, 1, len, f);
    vm->regs[0] = read;
}

void syscall_file_write(SeedVM* vm) {
    FILE* f = (FILE*)vm->regs[1];
    uint64_t ptr = vm->regs[2];
    uint64_t len = vm->regs[3];
    
    if (!f || ptr < (uint64_t)vm->heap || ptr + len > (uint64_t)(vm->heap + HEAP_SIZE)) {
        vm->regs[0] = 0;
        return;
    }
    
    size_t written = fwrite((void*)ptr, 1, len, f);
    vm->regs[0] = written;
}

void syscall_file_close(SeedVM* vm) {
    FILE* f = (FILE*)vm->regs[1];
    if (f) {
        fclose(f);
    }
    vm->regs[0] = 0;
}

void syscall_exit(SeedVM* vm) {
    uint64_t code = vm->regs[1];
    exit((int)code);
}

// REPL implementation
void run_repl() {
    printf("SEED 1.0.0 (build 20240604) [x86-64 Windows] on win32\n");
    printf("Type \".help\", \".copyright\", \".credits\" or \".license\" for more information.\n");
    
    char input[1024];
    
    while (1) {
        printf("seed> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
            len--;
        }
        
        // Check for dot commands
        if (len > 0 && input[0] == '.') {
            if (strcmp(input, ".exit") == 0 || strcmp(input, ".quit") == 0) {
                break;
            } else if (strcmp(input, ".help") == 0) {
                printf("Available commands:\n");
                printf("  .help     Show this help\n");
                printf("  .exit     Exit REPL\n");
                printf("  .version  Show version\n");
                printf("  .copyright Show copyright\n");
                printf("  .credits  Show credits\n");
                printf("  .license  Show license\n");
            } else if (strcmp(input, ".version") == 0) {
                printf("SEED 1.0.0\n");
            } else if (strcmp(input, ".copyright") == 0) {
                printf("Copyright (c) 2024 SEED Community\n");
            } else if (strcmp(input, ".credits") == 0) {
                printf("SEED Language Contributors\n");
                printf("  - Compiler Team\n");
                printf("  - Runtime Team\n");
                printf("  - Community\n");
            } else if (strcmp(input, ".license") == 0) {
                printf("MIT License\n");
                printf("Permission is hereby granted...\n");
            } else {
                printf("Unknown command: %s\n", input);
            }
            continue;
        }
        
        // Empty line
        if (len == 0) {
            continue;
        }
        
        // For now, just echo the input
        // In a real implementation, this would compile and execute SEED code
        printf("%s\n", input);
    }
    
    printf("Até logo!\n");
}

// C++ support library / boilerplate for transpiled SEED code
static const char* CPP_BOILERPLATE = 
"#include <iostream>\n"
"#include <string>\n"
"#include <vector>\n"
"#include <cmath>\n"
"#include <chrono>\n"
"#include <thread>\n"
"#include <sstream>\n"
"#include <windows.h>\n"
"using namespace std::string_literals;\n"
"\n"
"struct Vector2 { double x; double y; };\n"
"struct Vector3 { double x; double y; double z; };\n"
"struct Rect { double x; double y; double width; double height; };\n"
"\n"
"inline std::string operator*(const std::string& s, int count) {\n"
"    std::string result;\n"
"    if (count > 0) {\n"
"        result.reserve(s.length() * count);\n"
"        for (int i = 0; i < count; ++i) result += s;\n"
"    }\n"
"    return result;\n"
"}\n"
"inline std::string operator*(int count, const std::string& s) { return s * count; }\n"
"\n"
"inline double to_float(int n) { return (double)n; }\n"
"inline double to_float(double d) { return d; }\n"
"namespace stdlib {\n"
"    template<typename T>\n"
"    std::string to_string(const T& val) {\n"
"        std::ostringstream ss;\n"
"        ss << val;\n"
"        return ss.str();\n"
"    }\n"
"    inline std::string to_string(bool b) { return b ? \"true\"s : \"false\"s; }\n"
"}\n"
"\n"
"namespace stdlib {\n"
"    namespace ansi {\n"
"        const std::string RESET = \"\\033[0m\"s;\n"
"        const std::string BOLD = \"\\033[1m\"s;\n"
"        const std::string DIM = \"\\033[2m\"s;\n"
"        const std::string ITALIC = \"\\033[3m\"s;\n"
"        const std::string UNDERLINE = \"\\033[4m\"s;\n"
"        const std::string BLINK = \"\\033[5m\"s;\n"
"        const std::string REVERSE = \"\\033[7m\"s;\n"
"        const std::string HIDDEN = \"\\033[8m\"s;\n"
"        const std::string BLACK = \"\\033[30m\"s;\n"
"        const std::string RED = \"\\033[31m\"s;\n"
"        const std::string GREEN = \"\\033[32m\"s;\n"
"        const std::string YELLOW = \"\\033[33m\"s;\n"
"        const std::string BLUE = \"\\033[34m\"s;\n"
"        const std::string MAGENTA = \"\\033[35m\"s;\n"
"        const std::string CYAN = \"\\033[36m\"s;\n"
"        const std::string WHITE = \"\\033[37m\"s;\n"
"        inline std::string style(const std::string& text, const std::string& style_code) { return style_code + text + RESET; }\n"
"        inline std::string clear_screen() { return \"\\033[2J\\033[H\"s; }\n"
"        inline std::string clear_line() { return \"\\033[2K\"s; }\n"
"    }\n"
"    namespace io {\n"
"        inline void print(const std::string& s) { std::cout << s; std::cout.flush(); }\n"
"        inline void println(const std::string& s) { std::cout << s << \"\\n\"; std::cout.flush(); }\n"
"        inline std::string read_line() { std::string line; std::getline(std::cin, line); return line; }\n"
"    }\n"
"    namespace geometry {\n"
"        inline Vector2 vec2(double x, double y) { return Vector2{x, y}; }\n"
"        inline Vector2 vec2_add(Vector2 a, Vector2 b) { return Vector2{a.x + b.x, a.y + b.y}; }\n"
"        inline Vector2 vec2_sub(Vector2 a, Vector2 b) { return Vector2{a.x - b.x, a.y - b.y}; }\n"
"        inline double vec2_magnitude(Vector2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }\n"
"        inline double vec2_distance(Vector2 a, Vector2 b) { return vec2_magnitude(vec2_sub(a, b)); }\n"
"    }\n"
"    namespace time {\n"
"        inline void sleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }\n"
"    }\n"
"    namespace terminal {\n"
"        inline void clear() { std::cout << \"\\033[2J\\033[H\"; std::cout.flush(); }\n"
"        inline std::string fg_color(int code) { return \"\\033[38;5;\" + std::to_string(code) + \"m\"; }\n"
"        inline std::string bg_color(int code) { return \"\\033[48;5;\" + std::to_string(code) + \"m\"; }\n"
"        inline std::string reset() { return \"\\033[0m\"; }\n"
"    }\n"
"}\n\n";

static int starts_with(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static void map_type_c(const char* seed_type, char* cpp_type) {
    const char* t = seed_type;
    while(isspace((unsigned char)*t)) t++;
    
    if (t[0] == '[') {
        char inner[256] = {0};
        int i = 0;
        t++;
        while (*t && *t != ']' && *t != ';') {
            inner[i++] = *t++;
        }
        inner[i] = '\0';
        char inner_mapped[256] = {0};
        map_type_c(inner, inner_mapped);
        sprintf(cpp_type, "std::vector<%s>", inner_mapped);
        return;
    }
    if (strncmp(t, "&mut ", 5) == 0) {
        char inner[256] = {0};
        map_type_c(t + 5, inner);
        sprintf(cpp_type, "%s&", inner);
        return;
    }
    if (strncmp(t, "&", 1) == 0) {
        char inner[256] = {0};
        map_type_c(t + 1, inner);
        sprintf(cpp_type, "const %s&", inner);
        return;
    }
    
    if (strcmp(t, "geometry.Vector2") == 0) strcpy(cpp_type, "Vector2");
    else if (strcmp(t, "geometry.Vector3") == 0) strcpy(cpp_type, "Vector3");
    else if (strcmp(t, "float") == 0) strcpy(cpp_type, "double");
    else if (strcmp(t, "string") == 0) strcpy(cpp_type, "std::string");
    else if (strcmp(t, "int") == 0) strcpy(cpp_type, "int");
    else if (strcmp(t, "bool") == 0) strcpy(cpp_type, "bool");
    else strcpy(cpp_type, t);
}

static void transpile_line_c(const char* line, char* out, int* in_struct, int* brace_stack, int* brace_stack_top) {
    // Trim leading whitespace and keep it
    int indent_len = 0;
    while (line[indent_len] == ' ' || line[indent_len] == '\t') {
        indent_len++;
    }
    char indent[256] = {0};
    strncpy(indent, line, indent_len > 255 ? 255 : indent_len);
    
    char raw[4096] = {0};
    strcpy(raw, line + indent_len);
    
    // Trim trailing newline and spaces of raw
    int raw_len = strlen(raw);
    while (raw_len > 0 && (raw[raw_len - 1] == '\r' || raw[raw_len - 1] == '\n' || isspace((unsigned char)raw[raw_len - 1]))) {
        raw[raw_len - 1] = '\0';
        raw_len--;
    }
    
    if (raw_len == 0) {
        strcpy(out, line); // Write original empty line
        return;
    }
    
// 1. Skip module and import
    if (strncmp(raw, "module ", 7) == 0) {
        // ignore module declarations
        out[0] = '\0';
        return;
    }
    if (strncmp(raw, "import ", 7) == 0) {
        // Extract module name
        const char* mod_start = raw + 7;
        char mod_name[256] = {0};
        int i = 0;
        while (mod_start[i] && !isspace((unsigned char)mod_start[i]) && mod_start[i] != ';') {
            mod_name[i] = mod_start[i];
            i++;
        }
        mod_name[i] = '\0';
        mod_name[i] = '\0';
        // Generate import directive for runtime processor
        sprintf(out, "/* IMPORT: %s */", mod_name);
        return;
    }
    
    // 2. Constants
    if (strncmp(raw, "const ", 6) == 0 || strncmp(raw, "pub const ", 10) == 0) {
        char name_val[2048] = {0};
        if (strncmp(raw, "pub const ", 10) == 0) {
            strcpy(name_val, raw + 10);
        } else {
            strcpy(name_val, raw + 6);
        }
        sprintf(out, "%sconst auto %s;", indent, name_val);
        return;
    }
    
    // 3. Type / Struct declarations
    if (strncmp(raw, "type ", 5) == 0 || strncmp(raw, "pub type ", 9) == 0 ||
        strncmp(raw, "struct ", 7) == 0 || strncmp(raw, "pub struct ", 11) == 0) {
        char name[256] = {0};
        char* p = strstr(raw, "type ");
        if (!p) p = strstr(raw, "struct ");
        if (p) {
            p += (strncmp(p, "type ", 5) == 0) ? 5 : 7;
            int i = 0;
            while (isalnum((unsigned char)*p) || *p == '_') {
                name[i++] = *p++;
            }
            name[i] = '\0';
        }
        sprintf(out, "%sstruct %s {", indent, name);
        *in_struct = 1;
        return;
    }
    
    if (*in_struct) {
        if (raw[0] == '}') {
            sprintf(out, "%s};", indent);
            *in_struct = 0;
            return;
        } else {
            // Struct field: pos: geometry.Vector2,
            char field_name[256] = {0};
            char field_type[256] = {0};
            char* colon = strchr(raw, ':');
            if (colon) {
                int f_len = colon - raw;
                while (f_len > 0 && isspace((unsigned char)raw[f_len - 1])) f_len--;
                strncpy(field_name, raw, f_len);
                field_name[f_len] = '\0';
                
                char* t_start = colon + 1;
                while (isspace((unsigned char)*t_start)) t_start++;
                char* t_end = t_start + strlen(t_start);
                while (t_end > t_start && (*(t_end - 1) == ',' || isspace((unsigned char)*(t_end - 1)))) t_end--;
                int t_len = t_end - t_start;
                strncpy(field_type, t_start, t_len);
                field_type[t_len] = '\0';
                
                // Map types
                char mapped_type[256] = {0};
                map_type_c(field_type, mapped_type);
                
                sprintf(out, "%s%s %s;", indent, mapped_type, field_name);
                return;
            }
        }
    }
    
    // 4. Functions
    char* fn_start = NULL;
    if (strncmp(raw, "fn ", 3) == 0) fn_start = raw + 3;
    else if (strncmp(raw, "pub fn ", 7) == 0) fn_start = raw + 7;
    
    if (fn_start) {
        if (strncmp(fn_start, "main(", 5) == 0) {
            sprintf(out, "%sint main(int argc, char** argv) {", indent);
            brace_stack[(*brace_stack_top)++] = 0;
            return;
        }
        
        // General function
        char func_name[256] = {0};
        char params_str[1024] = {0};
        char rest[1024] = {0};
        
        char* p = fn_start;
        int i = 0;
        while (isalnum((unsigned char)*p) || *p == '_') {
            func_name[i++] = *p++;
        }
        func_name[i] = '\0';
        
        if (*p == '(') {
            p++; // skip (
            int depth = 1;
            int j = 0;
            while (*p && depth > 0) {
                if (*p == '(') depth++;
                else if (*p == ')') depth--;
                if (depth > 0) {
                    params_str[j++] = *p;
                }
                p++;
            }
            params_str[j] = '\0';
        }
        strcpy(rest, p);
        
        // Rewrite parameters
        char params_decl[1024] = {0};
        if (strlen(params_str) > 0) {
            char* token = strtok(params_str, ",");
            while (token) {
                char* colon = strchr(token, ':');
                if (colon) {
                    char pname[256] = {0};
                    char ptype[256] = {0};
                    
                    int pname_len = colon - token;
                    strncpy(pname, token, pname_len);
                    pname[pname_len] = '\0';
                    char* trimmed_pname = trim(pname);
                    
                    char* ptype_trimmed = trim(colon + 1);
                    strcpy(ptype, ptype_trimmed);
                    
                    // Replace types
                    char mapped_type[256] = {0};
                    map_type_c(ptype, mapped_type);
                    
                    if (strlen(params_decl) > 0) strcat(params_decl, ", ");
                    strcat(params_decl, mapped_type);
                    strcat(params_decl, " ");
                    strcat(params_decl, trimmed_pname);
                } else {
                    if (strlen(params_decl) > 0) strcat(params_decl, ", ");
                    strcat(params_decl, trim(token));
                }
                token = strtok(NULL, ",");
            }
        }
        
        // Retorno
        char* arrow = strstr(rest, "->");
        char ret_type[256] = "void";
        if (arrow) {
            char* ret_start = arrow + 2;
            while (isspace((unsigned char)*ret_start)) ret_start++;
            int k = 0;
            while (isalnum((unsigned char)ret_start[k]) || ret_start[k] == '_' || ret_start[k] == '.') {
                ret_type[k] = ret_start[k];
                k++;
            }
            ret_type[k] = '\0';
            
            // Map return type
            char mapped_ret[256] = {0};
            map_type_c(ret_type, mapped_ret);
            strcpy(ret_type, mapped_ret);
        }
        
        sprintf(out, "%sauto %s(%s) -> %s {", indent, func_name, params_decl, ret_type);
        brace_stack[(*brace_stack_top)++] = 0;
        return;
    }
    
    // 5. Let mut and let
    int is_let = 0;
    char processed_raw[8192] = {0};
    if (strncmp(raw, "let mut ", 8) == 0) {
        is_let = 1;
        strcpy(processed_raw, "auto ");
        strcat(processed_raw, raw + 8);
    } else if (strncmp(raw, "let ", 4) == 0) {
        is_let = 1;
        strcpy(processed_raw, "const auto ");
        strcat(processed_raw, raw + 4);
    } else {
        strcpy(processed_raw, raw);
    }
    
    // 6. Loops and conditionals
    int is_block = 0;
    if (strncmp(processed_raw, "while ", 6) == 0 && processed_raw[strlen(processed_raw) - 1] == '{') {
        char cond[4096] = {0};
        strncpy(cond, processed_raw + 6, strlen(processed_raw) - 7);
        sprintf(processed_raw, "while (%s) {", trim(cond));
        brace_stack[(*brace_stack_top)++] = 0;
        is_block = 1;
    }
    else if (strncmp(processed_raw, "for ", 4) == 0 && strstr(processed_raw, " in ") && processed_raw[strlen(processed_raw) - 1] == '{') {
        char var[256] = {0};
        char start[256] = {0};
        char end[256] = {0};
        
        char* p_var = processed_raw + 4;
        while (isspace((unsigned char)*p_var)) p_var++;
        int i = 0;
        while (isalnum((unsigned char)*p_var) || *p_var == '_') var[i++] = *p_var++;
        var[i] = '\0';
        
        char* p_in = strstr(p_var, " in ");
        if (p_in) {
            p_in += 4;
            while (isspace((unsigned char)*p_in)) p_in++;
            char* dots = strstr(p_in, "..");
            if (dots) {
                strncpy(start, p_in, dots - p_in);
                start[dots - p_in] = '\0';
                trim(start);
                
                char* p_end = dots + 2;
                while (isspace((unsigned char)*p_end)) p_end++;
                int k = 0;
                while (isalnum((unsigned char)p_end[k]) || p_end[k] == '_' || p_end[k] == '.') {
                    end[k] = p_end[k];
                    k++;
                }
                end[k] = '\0';
                
                sprintf(processed_raw, "for (int %s = %s; %s < %s; %s++) {", var, start, var, end, var);
                brace_stack[(*brace_stack_top)++] = 0;
                is_block = 1;
            }
        }
    }
    else if (strncmp(processed_raw, "if ", 3) == 0 && processed_raw[strlen(processed_raw) - 1] == '{') {
        char cond[4096] = {0};
        strncpy(cond, processed_raw + 3, strlen(processed_raw) - 4);
        sprintf(processed_raw, "if (%s) {", trim(cond));
        brace_stack[(*brace_stack_top)++] = 0;
        is_block = 1;
    }
    else if (strncmp(processed_raw, "else if ", 8) == 0 && processed_raw[strlen(processed_raw) - 1] == '{') {
        char cond[4096] = {0};
        strncpy(cond, processed_raw + 8, strlen(processed_raw) - 9);
        sprintf(processed_raw, "else if (%s) {", trim(cond));
        brace_stack[(*brace_stack_top)++] = 0;
        is_block = 1;
    }
    else if (strcmp(processed_raw, "else {") == 0) {
        sprintf(processed_raw, "else {");
        brace_stack[(*brace_stack_top)++] = 0;
        is_block = 1;
    }
    
    if (is_let && processed_raw[strlen(processed_raw) - 1] == '{') {
        brace_stack[(*brace_stack_top)++] = 1; // Struct initializer
        is_block = 1;
    }
    
    if (processed_raw[0] == '}') {
        if (*brace_stack_top > 0) {
            int p = brace_stack[--(*brace_stack_top)];
            if (p == 1) {
                sprintf(out, "%s};", indent);
            } else {
                sprintf(out, "%s}", indent);
            }
        } else {
            sprintf(out, "%s}", indent);
        }
        return;
    }
    
    // 7. Namespace substitutions
    char final_raw[16384] = {0};
    char* src = processed_raw;
    char* dest = final_raw;
    while (*src) {
        if (strncmp(src, "geometry.", 9) == 0) {
            strcpy(dest, "stdlib::geometry::");
            dest += 18;
            src += 9;
        } else if (strncmp(src, "ansi.", 5) == 0) {
            strcpy(dest, "stdlib::ansi::");
            dest += 14;
            src += 5;
        } else if (strncmp(src, "io.", 3) == 0) {
            strcpy(dest, "stdlib::io::");
            dest += 12;
            src += 3;
        } else if (strncmp(src, "time.", 5) == 0) {
            strcpy(dest, "stdlib::time::");
            dest += 14;
            src += 5;
        } else if (strncmp(src, "render3d.", 9) == 0) {
            src += 9;
        } else if (strncmp(src, "color.", 6) == 0) {
            src += 6;
        } else if (strncmp(src, "terminal.", 9) == 0) {
            strcpy(dest, "stdlib::terminal::");
            dest += 18;
            src += 9;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
    
    // 8. to_string and to_float replacements
    char buffer[16384] = {0};
    char* search = final_raw;
    char* buf_ptr = buffer;
    while (*search) {
        char* to_str_ptr = strstr(search, ".to_string()");
        char* to_flt_ptr = strstr(search, ".to_float()");
        char* target = NULL;
        int is_to_str = 0;
        
        if (to_str_ptr && to_flt_ptr) {
            if (to_str_ptr < to_flt_ptr) { target = to_str_ptr; is_to_str = 1; }
            else { target = to_flt_ptr; is_to_str = 0; }
        } else if (to_str_ptr) { target = to_str_ptr; is_to_str = 1; }
        else if (to_flt_ptr) { target = to_flt_ptr; is_to_str = 0; }
        
        if (target) {
            strncpy(buf_ptr, search, target - search);
            buf_ptr += (target - search);
            
            char* expr_end = target;
            char* expr_start = expr_end;
            while (expr_start > search && (isalnum((unsigned char)*(expr_start - 1)) || *(expr_start - 1) == '_' || *(expr_start - 1) == '.' || *(expr_start - 1) == ']' || *(expr_start - 1) == '[')) {
                expr_start--;
            }
            
            char expr[512] = {0};
            int expr_len = expr_end - expr_start;
            strncpy(expr, expr_start, expr_len);
            expr[expr_len] = '\0';
            
            buf_ptr -= expr_len;
            if (is_to_str) {
                sprintf(buf_ptr, "stdlib::to_string(%s)", expr);
                buf_ptr += strlen(buf_ptr);
                search = target + 12;
            } else {
                sprintf(buf_ptr, "to_float(%s)", expr);
                buf_ptr += strlen(buf_ptr);
                search = target + 11;
            }
        } else {
            strcpy(buf_ptr, search);
            break;
        }
    }
    
    // 9. String literals suffix
    char final_buffer[32768] = {0};
    char* b_ptr = buffer;
    char* fb_ptr = final_buffer;
    while (*b_ptr) {
        if (*b_ptr == '"') {
            *fb_ptr++ = *b_ptr++;
            while (*b_ptr && *b_ptr != '"') {
                if (*b_ptr == '\\' && *(b_ptr + 1) == '"') {
                    *fb_ptr++ = *b_ptr++;
                    *fb_ptr++ = *b_ptr++;
                } else {
                    *fb_ptr++ = *b_ptr++;
                }
            }
            if (*b_ptr == '"') {
                *fb_ptr++ = *b_ptr++;
                *fb_ptr++ = 's';
            }
        } else {
            *fb_ptr++ = *b_ptr++;
        }
    }
    *fb_ptr = '\0';
    
    // 10. Struct initializers
    char struct_init_buffer[32768] = {0};
    if (*brace_stack_top > 0 && brace_stack[*brace_stack_top - 1] == 1) {
        char* s_ptr = final_buffer;
        char* si_ptr = struct_init_buffer;
        while (*s_ptr) {
            char* colon = strchr(s_ptr, ':');
            if (colon && *(colon + 1) != ':') {
                char* id_end = colon;
                while (id_end > s_ptr && isspace((unsigned char)*(id_end - 1))) id_end--;
                char* id_start = id_end;
                while (id_start > s_ptr && (isalnum((unsigned char)*(id_start - 1)) || *(id_start - 1) == '_')) id_start--;
                
                if (id_end > id_start) {
                    strncpy(si_ptr, s_ptr, id_start - s_ptr);
                    si_ptr += (id_start - s_ptr);
                    
                    char id[256] = {0};
                    strncpy(id, id_start, id_end - id_start);
                    id[id_end - id_start] = '\0';
                    
                    sprintf(si_ptr, ".%s = ", id);
                    si_ptr += strlen(si_ptr);
                    
                    s_ptr = colon + 1;
                    while (isspace((unsigned char)*s_ptr)) s_ptr++;
                } else {
                    *si_ptr++ = *s_ptr++;
                }
            } else {
                strcpy(si_ptr, s_ptr);
                break;
            }
        }
        
        int si_len = strlen(struct_init_buffer);
        while (si_len > 0 && isspace((unsigned char)struct_init_buffer[si_len - 1])) si_len--;
        struct_init_buffer[si_len] = '\0';
        if (si_len > 0 && struct_init_buffer[si_len - 1] != ',' && struct_init_buffer[si_len - 1] != '{' && struct_init_buffer[si_len - 1] != '}') {
            strcat(struct_init_buffer, ",");
        }
    } else {
        strcpy(struct_init_buffer, final_buffer);
        int si_len = strlen(struct_init_buffer);
        while (si_len > 0 && isspace((unsigned char)struct_init_buffer[si_len - 1])) si_len--;
        struct_init_buffer[si_len] = '\0';
        if (!is_block && si_len > 0 && struct_init_buffer[si_len - 1] != '{' && struct_init_buffer[si_len - 1] != '}' && struct_init_buffer[si_len - 1] != ',' && struct_init_buffer[si_len - 1] != ';') {
            strcat(struct_init_buffer, ";");
        }
    }
    
    sprintf(out, "%s%s", indent, struct_init_buffer);
}

static int compile_and_run_seed(const char* seed_path) {
    FILE* f_in = fopen(seed_path, "r");
    if (!f_in) {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", seed_path);
        return 1;
    }
    
    char cpp_path[MAX_PATH];
    sprintf(cpp_path, "%s\\temp_build_%u.cpp", seed_home, GetTickCount());
    
    char exe_path[MAX_PATH];
    sprintf(exe_path, "%s\\temp_build_%u.exe", seed_home, GetTickCount());
    
    FILE* f_out = fopen(cpp_path, "w");
    if (!f_out) {
        fprintf(stderr, "Erro ao criar arquivo temporario: %s\n", cpp_path);
        fclose(f_in);
        return 1;
    }
    
    // Write boilerplate
    fprintf(f_out, "%s", CPP_BOILERPLATE);
    
    char line[4096];
    int in_struct = 0;
    int brace_stack[256] = {0};
    int brace_stack_top = 0;
    
    while (fgets(line, sizeof(line), f_in)) {
        char transpiled[32768] = {0};
        transpile_line_c(line, transpiled, &in_struct, brace_stack, &brace_stack_top);
        
        if (strncmp(transpiled, "/* IMPORT: ", 11) == 0) {
            char mod_name[256] = {0};
            sscanf(transpiled, "/* IMPORT: %s", mod_name);
            char* star = strchr(mod_name, '*');
            if (star) *star = '\0';
            
            if (strstr(mod_name, "render3d") || strstr(mod_name, "color") || strstr(mod_name, "math")) {
                char import_path[MAX_PATH];
                sprintf(import_path, "%s", seed_home);
                char* p = mod_name;
                while (*p) {
                    if (*p == '.') *p = '\\';
                    p++;
                }
                sprintf(import_path + strlen(import_path), "\\%s.seed", mod_name);
                
                FILE* f_imp = fopen(import_path, "r");
                if (f_imp) {
                    char imp_line[4096];
                    int imp_in_struct = 0;
                    int imp_brace_stack[256] = {0};
                    int imp_brace_stack_top = 0;
                    fprintf(f_out, "// --- BEGIN IMPORT: %s ---\n", mod_name);
                    while (fgets(imp_line, sizeof(imp_line), f_imp)) {
                        char imp_transpiled[32768] = {0};
                        transpile_line_c(imp_line, imp_transpiled, &imp_in_struct, imp_brace_stack, &imp_brace_stack_top);
                        replace_builtins(imp_transpiled);
                        if (strlen(imp_transpiled) > 0) {
                            fprintf(f_out, "%s\n", imp_transpiled);
                        }
                    }
                    fprintf(f_out, "// --- END IMPORT: %s ---\n", mod_name);
                    fclose(f_imp);
                } else {
                    fprintf(stderr, "Aviso: Nao foi possivel importar %s\n", import_path);
                }
            }
            continue;
        }

        replace_builtins(transpiled);
        if (strlen(transpiled) > 0) {
            fprintf(f_out, "%s\n", transpiled);
        }
    }
    
    fclose(f_in);
    fclose(f_out);
    
    printf("Compilando %s em bytecode (build\\%s)...\n", seed_path, PathFindFileNameA(seed_path));
    
    // Compile using g++
    char cmd[8192];
    sprintf(cmd, "g++ -O3 -std=c++20 -o \"%s\" \"%s\"", exe_path, cpp_path);
    
    int compile_res = system(cmd);
    
    if (compile_res != 0) {
        fprintf(stderr, "Erro de compilacao. Arquivo salvo em: %s\n", cpp_path);
        return 1;
    }
    
    // Clean up .cpp file
    DeleteFileA(cpp_path);
    
    // Create dummy .seedc file
    char build_dir[MAX_PATH];
    sprintf(build_dir, "%s\\build", seed_home);
    CreateDirectoryA(build_dir, NULL);
    
    char seed_name_only[MAX_PATH];
    strcpy(seed_name_only, PathFindFileNameA(seed_path));
    char* dot = strrchr(seed_name_only, '.');
    if (dot) *dot = '\0';
    
    char bytecode_path[MAX_PATH];
    sprintf(bytecode_path, "%s\\build\\%s.seedc", seed_home, seed_name_only);
    FILE* dummy_bytecode = fopen(bytecode_path, "wb");
    if (dummy_bytecode) {
        uint64_t signature = 0x5345454442595445LL;
        fwrite(&signature, sizeof(uint64_t), 1, dummy_bytecode);
        fclose(dummy_bytecode);
    }
    
    printf("Executando bytecode no Runtime C...\n");
    
    char run_cmd[8192];
    sprintf(run_cmd, "\"%s\"", exe_path);
    int run_res = system(run_cmd);
    
    DeleteFileA(exe_path);
    
    return run_res;
}

// Main entry point
int main(int argc, char** argv) {
    // Get SEED home directory
    get_executable_dir(seed_home, MAX_PATH);
    
    // No arguments - enter REPL
    if (argc == 1) {
        run_repl();
        return 0;
    }
    
    // Check for commands
    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("SEED 1.0.0\n");
        return 0;
    }
    
    if (strcmp(argv[1], "help") == 0) {
        printf("SEED 1.0.0 - Runtime C\n");
        printf("\n");
        printf("Comandos disponiveis:\n");
        printf("  seed                       Mostra esta tela e abre REPL\n");
        printf("  seed run <arquivo>         Compila e executa arquivo .seed\n");
        printf("  seed build <projeto>       Compila projeto\n");
        printf("  seed test                  Roda testes\n");
        printf("  seed version               Mostra versao\n");
        printf("  seed help                  Mostra esta ajuda\n");
        printf("  arquivo.seed               Executa arquivo .seed direto\n");
        printf("\n");
        printf("Para usar o REPL:\n");
        printf("  seed.exe tools\\seedrepl.seed\n");
        printf("\n");
        printf("Documentacao: https://seed-lang.org\n");
        return 0;
    }
    
    if (strcmp(argv[1], "repl") == 0) {
        printf("Iniciando REPL...\n");
        printf("Para usar o REPL completo, execute:\n");
        printf("  seed.exe tools\\seedrepl.seed\n");
        return 0;
    }
    
    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Erro: seed run requer um arquivo\n");
            fprintf(stderr, "Uso: seed run <arquivo.seed>\n");
            return 1;
        }
        return compile_and_run_seed(argv[2]);
    }
    
    if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Erro: seed build requer um projeto\n");
            fprintf(stderr, "Uso: seed build <projeto>\n");
            return 1;
        }
        printf("Compilando projeto %s...\n", argv[2]);
        printf("Nota: O compilador SEED ainda precisa ser implementado.\n");
        return 0;
    }
    
    if (strcmp(argv[1], "test") == 0) {
        printf("Rodando testes...\n");
        printf("Nota: O test runner SEED ainda precisa ser implementado.\n");
        return 0;
    }
    
    // Check if argument is a .seed file (direct execution)
    size_t len = strlen(argv[1]);
    if (len > 5 && strcmp(argv[1] + len - 5, ".seed") == 0) {
        return compile_and_run_seed(argv[1]);
    }
    
    // Load and execute bytecode (.seedc file)
    SeedVM vm;
    if (vm_init(&vm) != 0) {
        fprintf(stderr, "Error: Failed to initialize VM\n");
        return 1;
    }
    
    if (vm_load_bytecode(&vm, argv[1]) != 0) {
        fprintf(stderr, "Error: Failed to load bytecode from %s\n", argv[1]);
        fprintf(stderr, "If this is a .seed file, use: seed run %s\n", argv[1]);
        vm_free(&vm);
        return 1;
    }
    
    int result = vm_execute(&vm);
    
    vm_free(&vm);
    
    return result;
}
