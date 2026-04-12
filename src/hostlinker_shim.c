/*
 * hostlinker_shim.c - DustLink Host Runtime Shim (C implementation)
 *
 * Rust-independent replacement for host_runtime_shim.rs.
 * Provides the C ABI functions called by Dust linker modules (.ds files).
 *
 * Layer 0: OS-level FFI boundary (file I/O, paths, strings, CLI args)
 * Layer 1: Linker state management (objects, symbols, relocations, sections)
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* ========================================================================
 * Constants
 * ======================================================================== */

#define ERR_OK                  0
#define ERR_FILE_NOT_FOUND      1
#define ERR_INVALID_FORMAT      2
#define ERR_INVALID_SECTION     3
#define ERR_UNDEFINED_SYMBOL    4
#define ERR_MULTIPLE_DEFINITION 5
#define ERR_INVALID_RELOCATION  6
#define ERR_INVALID_ADDRESS     8
#define ERR_INVALID_ENTRY       9
#define ERR_WRITE_FAILED        10
#define ERR_INVALID_IMAGE       11
#define ERR_UNSUPPORTED_FLAG    12
#define ERR_MISSING_FLAG_VALUE  13
#define ERR_UNSUPPORTED_TARGET  14
#define ERR_EMPTY_INPUT         15
#define ERR_CONFLICTING_OPTIONS 16
#define ERR_NOT_IMPLEMENTED_YET 17

#define DEFAULT_OUTPUT_HANDLE   1
#define DEFAULT_OUTPUT_NAME     "a.out"

#define FORMAT_ELF64    1
#define FORMAT_BINARY   2
#define FORMAT_MBR      3
#define FORMAT_PE64     4
#define FORMAT_MACHO64  5

#define TARGET_NONE             0
#define TARGET_X86_64_LINUX     1
#define TARGET_X86_64_WINDOWS   2
#define TARGET_X86_64_MACOS     3
#define TARGET_AARCH64_LINUX    4
#define TARGET_AARCH64_WINDOWS  5
#define TARGET_AARCH64_MACOS    6

/* ELF constants */
#define SHN_UNDEF   0
#define SHN_ABS     65521

#define COFF_MACHINE_X86_64  0x8664
#define COFF_MACHINE_AARCH64 0xaa64

#define MACH_CPU_X86_64  0x01000007
#define MACH_CPU_ARM64   0x0100000c
#define MH_DYLIB         0x6

/* Object format probe results */
#define OBJECT_FORMAT_UNKNOWN   0
#define OBJECT_FORMAT_ELF64     1
#define OBJECT_FORMAT_COFF64    2
#define OBJECT_FORMAT_MACHO64   3

/* Capacity limits (match Rust MAXSECTIONS/MAXSYMBOLS/etc) */
#define MAX_OBJECTS        4096
#define MAX_SECTIONS       4096
#define MAX_SYMBOLS        131072
#define MAX_RELOCATIONS    131072
#define MAX_GLOSYMB        131072
#define MAX_SEARCH_PATHS   256
#define MAX_RPATHS         64
#define MAX_RPATH_LINKS    64
#define MAX_ARCHIVES       256
#define MAX_OUTPUT_SECS    256
#define MAX_TRACE_SYMS     256
#define MAX_STRINGS        65536
#define MAX_MAP_ROWS       131072
#define MAX_NEEDED_LIBS    256
#define MAX_DEFERRED       1024
#define MAX_OPT_STATES     32
#define MAX_TLS_SLOTS      4096
#define MAX_GOT_SLOTS      4096

/* ========================================================================
 * String pool
 * ======================================================================== */

static char **g_strings = NULL;
static uint32_t g_strings_count = 0;
static uint32_t g_strings_cap = 0;

/* CLI argument cache */
static char **g_argv = NULL;
static int g_argc = 0;

static void init_args(void) {
    if (g_argv != NULL) return;
    /* Read from environment as "ARGV0\0ARGV1\0..." or fall back to defaults */
    /* For dustlink, the dust compiler passes argc > 0 before main() calls
       the linker entry. We assume argc >= 1 with g_argv[0] being the program. */
    g_argc = 1;
    g_argv = (char **)malloc(sizeof(char *));
    g_argv[0] = strdup("dustlink");
}

/* ========================================================================
 * Hash function (FNV-1a 64-bit, matches Rust fnv1a64)
 * ======================================================================== */

static uint64_t fnv1a64(const char *s) {
    uint64_t h = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        h ^= *p;
        h *= 1099511628211ULL;
    }
    return h;
}

/* ========================================================================
 * Small dynamic arrays (no external dependencies)
 * ======================================================================== */

typedef struct {
    uint64_t *items;
    uint32_t  count;
    uint32_t  cap;
} u64_vec;

typedef struct {
    uint16_t *items;
    uint32_t  count;
    uint32_t  cap;
} u16_vec;

typedef struct {
    char **items;
    uint32_t count;
    uint32_t cap;
} str_vec;

typedef struct {
    uint32_t *items;
    uint32_t  count;
    uint32_t  cap;
} u32_vec;

static void u64_vec_init(u64_vec *v) { v->items = NULL; v->count = 0; v->cap = 0; }
static void str_vec_init(str_vec *v) { v->items = NULL; v->count = 0; v->cap = 0; }
static void u32_vec_init(u32_vec *v) { v->items = NULL; v->count = 0; v->cap = 0; }
static void u16_vec_init(u16_vec *v) { v->items = NULL; v->count = 0; v->cap = 0; }

static int u64_vec_push(u64_vec *v, uint64_t val) {
    if (v->count >= v->cap) {
        uint32_t nc = v->cap ? v->cap * 2 : 64;
        v->items = (uint64_t *)realloc(v->items, nc * sizeof(uint64_t));
        if (!v->items) return -1;
        v->cap = nc;
    }
    v->items[v->count++] = val;
    return 0;
}

static int str_vec_push(str_vec *v, const char *s) {
    if (v->count >= v->cap) {
        uint32_t nc = v->cap ? v->cap * 2 : 16;
        v->items = (char **)realloc(v->items, nc * sizeof(char *));
        if (!v->items) return -1;
        v->cap = nc;
    }
    v->items[v->count++] = s ? strdup(s) : NULL;
    return 0;
}

static int u32_vec_push(u32_vec *v, uint32_t val) {
    if (v->count >= v->cap) {
        uint32_t nc = v->cap ? v->cap * 2 : 16;
        v->items = (uint32_t *)realloc(v->items, nc * sizeof(uint32_t));
        if (!v->items) return -1;
        v->cap = nc;
    }
    v->items[v->count++] = val;
    return 0;
}

static void u64_vec_free(u64_vec *v) { free(v->items); v->items = NULL; v->count = 0; v->cap = 0; }
static void str_vec_free(str_vec *v) {
    for (uint32_t i = 0; i < v->count; i++) free(v->items[i]);
    free(v->items); v->items = NULL; v->count = 0; v->cap = 0;
}
static void u32_vec_free(u32_vec *v) { free(v->items); v->items = NULL; v->count = 0; v->cap = 0; }
static void u16_vec_free(u16_vec *v) { free(v->items); v->items = NULL; v->count = 0; v->cap = 0; }

/* ========================================================================
 * Object Record (matches Rust ObjectRecord / ObjectSection /
 * ObjectSymbol / ObjectRelocation)
 * ======================================================================== */

typedef struct {
    uint32_t  index;
    uint32_t  sec_type;
    uint64_t  flags;
    uint64_t  offset;
    uint64_t  size;
    uint32_t  link;
    uint32_t  info;
    uint64_t  align;
    uint64_t  entsize;
    uint8_t  *data;       /* owned section data when needed */
    uint64_t  data_len;
} ObjectSection;

typedef struct {
    uint64_t  name_hash;
    uint8_t   bind;
    uint8_t   sym_type;
    uint16_t  shndx;
    uint64_t  value;
    uint64_t  size;
    uint32_t  strtab_section;
    uint8_t   defined;    /* 0 = undefined, 1 = defined */
    /* Additional fields for global symbol table */
    uint32_t  object_index;
    uint32_t  symbol_index;
    uint64_t  address;
} ObjectSymbol;

typedef struct {
    uint32_t  section;
    uint64_t  offset;
    uint32_t  reloc_type;
    uint32_t  symbol;
    uint64_t  addend;
} ObjectRelocation;

typedef struct {
    uint64_t  path;           /* CString handle */
    uint64_t  file_size;
    uint16_t  elf_type;
    uint16_t  machine;
    uint32_t  object_kind;    /* 0=unknown, 1=ELF, 2=COFF, 3=Mach-O */

    ObjectSection  *sections;
    uint32_t   sec_count;
    uint32_t   sec_cap;

    ObjectSymbol  *symbols;
    uint32_t   sym_count;
    uint32_t   sym_cap;

    ObjectRelocation *relocs;
    uint32_t   reloc_count;
    uint32_t   reloc_cap;

    /* For image patching */
    uint8_t    *image_data;
    uint64_t   image_size;
} ObjectRecord;

typedef struct {
    uint32_t  object_index;
    uint32_t  symbol_index;
    uint8_t   bind;
    uint32_t  defined;
    uint64_t  address;
} GlobalSymbol;

typedef struct {
    uint32_t  model;
    uint64_t  canonical_name_hash;
    uint32_t  canonical_object_index;
    uint32_t  canonical_symbol_index;
    uint32_t  slot_index;
    uint64_t  reloc_addend;
} TlsSynthSlot;

typedef struct {
    uint32_t  z_flags;       /* bitmask */
} ZFlags;

typedef struct {
    uint32_t  link_static;
    uint32_t  link_as_needed;
    uint32_t  symbolic_bindings;
    uint32_t  symbolic_functions;
    uint32_t  symbolic_group;
    uint32_t  z_origin;
    uint32_t  z_interpose;
    uint32_t  z_initfirst;
    uint32_t  z_nodelete;
    uint32_t  use_default_search_paths;
    uint32_t  whole_archive;
    uint32_t  link_new_dtags;
    uint32_t  link_copy_dt_needed_entries;
    uint32_t  no_undefined;
    uint32_t  allow_shlib_undefined;
    uint32_t  unresolved_policy;
    uint32_t  warn_unresolved;
    uint32_t  trace_enabled;
} LinkerOptionState;

typedef struct {
    /* Output section data for image building */
    uint8_t  *data;
    uint64_t  size;
    uint64_t  cap;
} OutputSection;

/* ========================================================================
 * Global Linker State (replaces Rust LinkerState)
 * ======================================================================== */

static struct LinkerState {
    uint64_t  last_error;
    uint64_t  output_path;
    uint32_t  output_format;
    uint32_t  target;
    uint64_t  entry;
    uint64_t  image_base;
    uint32_t  arch;
    uint32_t  os;

    u64_vec      search_paths;
    str_vec      search_path_strs;   /* actual string values */
    u64_vec      archives;
    u64_vec      inputs;

    ObjectRecord *objects;
    uint32_t     object_count;
    uint32_t     object_cap;

    ObjectSymbol *globals_sym;
    uint64_t     *globals_hash;  /* parallel hash array */
    uint32_t     globals_count;
    uint32_t     globals_cap;

    u64_vec      required_symbols;
    OutputSection *output_sections;
    uint32_t     output_section_count;
    uint32_t     output_section_cap;
    uint32_t     active_patch_object;      /* 0 = none */

    uint32_t  strip_debug;
    uint32_t  gc_sections;
    uint32_t  allow_multiple_definition;

    uint32_t  build_id_mode;
    uint8_t   build_id_data[64];
    uint32_t  build_id_len;

    uint32_t  z_relro;
    uint32_t  z_now;
    uint32_t  z_execstack;
    uint32_t  z_origin;
    uint32_t  z_interpose;
    uint32_t  z_initfirst;
    uint32_t  z_nodelete;
    uint32_t  z_nodlopen;

    uint64_t  memory_origin;
    uint64_t  memory_length;
    uint64_t  pending_elf_entry;
    uint64_t  pending_elf_image_base;

    u64_vec      start_lib_members;
    u32_vec      loaded_start_lib_members;
    u32_vec      group_start_lib_starts;
    u32_vec      group_archive_starts;
    uint32_t     archive_progress;
    uint32_t     start_lib_progress;

    /* Loaded archive member tracking: simple linear list */
    u64_vec      loaded_archive_keys;   /* path_handle#index encoded */

    LinkerOptionState opt_stack[MAX_OPT_STATES];
    uint32_t          opt_stack_top;

    uint32_t  link_shared;
    uint32_t  link_pie;
    uint32_t  link_static;
    uint32_t  link_as_needed;
    uint32_t  symbolic_bindings;
    uint32_t  symbolic_functions;
    uint32_t  symbolic_group;
    uint32_t  whole_archive;
    uint32_t  link_new_dtags;
    uint32_t  link_copy_dt_needed_entries;
    uint32_t  no_undefined;
    uint32_t  allow_shlib_undefined;
    uint32_t  unresolved_policy;
    uint32_t  warn_unresolved;
    uint32_t  trace_enabled;

    u64_vec     trace_symbol_hashes;

    uint64_t  shared_undefined_hashes[MAX_DEFERRED];
    uint32_t  shared_undefined_count;

    uint64_t  sysroot_path;

    u64_vec   rpaths;
    str_vec   rpaths_str;

    u64_vec   rpath_links;
    str_vec   rpath_links_str;

    uint64_t  dynamic_linker_path;
    uint64_t  soname;

    u64_vec   needed_shared_libs;
    str_vec   needed_shared_libs_str;
    uint32_t  last_shared_object_retained;
    uint32_t  use_default_search_paths;

    /* Blocked default library hashes */
    uint64_t  blocked_lib_hashes[MAX_DEFERRED];
    uint32_t  blocked_lib_count;

    uint32_t  hash_style;
    uint32_t  thread_count;
    uint32_t  emit_eh_frame_hdr;
    uint32_t  fatal_warnings;
    uint32_t  color_diagnostics;
    uint32_t  print_gc_sections;
    uint32_t  icf_mode;

    uint32_t  pe_no_entry;
    uint32_t  pe_dynamic_base;
    uint32_t  pe_nx_compat;
    uint32_t  pe_large_address_aware;

    uint64_t  dependency_file;
    uint32_t  emit_relocs;

    /* TLS synthetic slots */
    uint64_t  aarch64_tls_synth_base;
    TlsSynthSlot *tls_synth_slots;
    uint32_t  tls_synth_count;
    uint32_t  tls_synth_cap;

    /* TLS GOT slots */
    uint64_t  tls_got_base;
    struct { uint32_t obj; uint32_t sym; uint64_t addr; } *tls_got_slots;
    uint32_t  tls_got_count;
    uint32_t  tls_got_cap;

    /* Map file rows */
    char     **map_rows;
    uint32_t  map_row_count;
    uint32_t  map_row_cap;

    /* Image buffer for patching */
    uint8_t  *image_buf;
    uint64_t  image_buf_size;
} g_state;


/* ========================================================================
 * Helpers
 * ======================================================================== */

static int string_store(uint64_t *out, const char *s) {
    if (!s) return -1;
    if (g_strings_count >= g_strings_cap) {
        uint32_t nc = g_strings_cap ? g_strings_cap * 2 : 1024;
        char **ns = (char **)realloc(g_strings, nc * sizeof(char *));
        if (!ns) return -1;
        g_strings = ns;
        g_strings_cap = nc;
    }
    char *dup = strdup(s);
    if (!dup) return -1;
    g_strings[g_strings_count++] = dup;
    *out = (uint64_t)(uintptr_t)dup;
    return 0;
}

static const char *cstring_from_handle(uint64_t h) {
    if (h == DEFAULT_OUTPUT_HANDLE) return DEFAULT_OUTPUT_NAME;
    if (h < 0x10000) return NULL;
    return (const char *)(uintptr_t)h;
}

static char *cstring_dup_handle(uint64_t h) {
    if (h == DEFAULT_OUTPUT_HANDLE) return strdup(DEFAULT_OUTPUT_NAME);
    if (h == 0) return NULL;
    if (h < 0x10000) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%" PRIu64, h);
        /* Actually return handle directly, not as string */
        return strdup((const char *)(uintptr_t)h);
    }
    return strdup((const char *)(uintptr_t)h);
}

static int path_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0 ? 1 : 0;
}

static int path_is_file(const char *p) {
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

static uint64_t file_size_by_path(const char *p) {
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    return (uint64_t)st.st_size;
}

static uint8_t read_u8_at_path(const char *p, uint64_t off) {
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    if (fseek(f, (long)off, SEEK_SET) != 0) { fclose(f); return 0; }
    uint8_t b;
    fread(&b, 1, 1, f);
    fclose(f);
    return b;
}

static int write_u8_at_path(const char *p, uint64_t off, uint8_t v) {
    FILE *f = fopen(p, "r+b");
    if (!f) { f = fopen(p, "wb"); }
    if (!f) return -1;
    if (fseek(f, (long)off, SEEK_SET) != 0) { fclose(f); return -1; }
    fwrite(&v, 1, 1, f);
    fclose(f);
    return 0;
}

static int truncate_file(const char *p, uint64_t size) {
    FILE *f = fopen(p, "wb");
    if (!f) return -1;
    if (size == 0) { fclose(f); return 0; }
    if (fseek(f, size - 1, SEEK_SET) != 0) { fclose(f); return -1; }
    /* Write null byte to set file size to size */
    char zero = 0;
    fwrite(&zero, 1, 1, f);
    fclose(f);
    return 0;
}

static int append_line_to_path(const char *p, const char *line) {
    FILE *f = fopen(p, "a");
    if (!f) return -1;
    fprintf(f, "%s\n", line);
    fclose(f);
    return 0;
}

/* ELF byte-read helpers (little-endian) */
static uint16_t read_u16_le_path(const char *p, uint64_t off) {
    uint8_t b0 = read_u8_at_path(p, off);
    uint8_t b1 = read_u8_at_path(p, off + 1);
    return (uint16_t)b0 | ((uint16_t)b1 << 8);
}

static uint32_t read_u32_le_path(const char *p, uint64_t off) {
    uint16_t lo = read_u16_le_path(p, off);
    uint16_t hi = read_u16_le_path(p, off + 2);
    return (uint32_t)lo | ((uint32_t)hi << 16);
}

static uint64_t read_u64_le_path(const char *p, uint64_t off) {
    uint32_t lo = read_u32_le_path(p, off);
    uint32_t hi = read_u32_le_path(p, off + 4);
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

/* FNV-1a hash of null-terminated C string */
static uint64_t str_hash(const char *s) {
    return fnv1a64(s);
}

/* Simple linear search in globals */
static int find_global(uint64_t name_hash) {
    for (uint32_t i = 0; i < g_state.globals_count; i++) {
        if (g_state.globals_hash[i] == name_hash) return (int)i;
    }
    return -1;
}

/* ========================================================================
 * LAYER 0: CLI Args
 * ======================================================================== */

uint32_t host_args_count(void) {
    init_args();
    return (uint32_t)g_argc;
}

uint64_t host_arg_at(uint32_t index) {
    init_args();
    if ((int)index >= g_argc) return 0;
    uint64_t h = 0;
    if (string_store(&h, g_argv[index]) < 0) return 0;
    return h;
}

uint64_t host_args(void) {
    init_args();
    return (uint64_t)(uintptr_t)g_argv;
}

/* ========================================================================
 * LAYER 0: String operations
 * ======================================================================== */

uint64_t host_str_after_prefix(uint64_t text, uint64_t prefix) {
    const char *t = cstring_from_handle(text);
    const char *p = cstring_from_handle(prefix);
    if (!t || !p) return 0;
    size_t plen = strlen(p);
    if (strncmp(t, p, plen) == 0) {
        uint64_t h = 0;
        string_store(&h, t + plen);
        return h;
    }
    return 0;
}

uint32_t host_print_line(uint64_t text) {
    const char *t = cstring_from_handle(text);
    if (!t) return ERR_INVALID_FORMAT;
    printf("%s\n", t);
    return ERR_OK;
}

uint64_t host_parse_u64(uint64_t text, uint32_t base) {
    const char *t = cstring_from_handle(text);
    if (!t) return 0;
    if (base == 0 || base > 36) base = 10;
    char *end;
    errno = 0;
    uint64_t v = strtoull(t, &end, (int)base);
    if (errno != 0 || end == t) return 0;
    return v;
}

uint32_t host_path_exists(uint64_t path) {
    return path_exists((const char *)(uintptr_t)path) ? 1 : 0;
}

uint32_t host_path_is_file(uint64_t path) {
    return path_is_file((const char *)(uintptr_t)path) ? 1 : 0;
}

uint64_t host_path_join(uint64_t base, uint64_t leaf) {
    const char *b = cstring_from_handle(base);
    const char *l = cstring_from_handle(leaf);
    if (!b) { b = cstring_from_handle(base); }
    if (!l) return base;
    if (!b) return leaf;

    size_t bl = strlen(b);
    size_t ll = strlen(l);
    char *joined = malloc(bl + 1 + ll + 1);
    if (!joined) return 0;
    memcpy(joined, b, bl);
    if (bl > 0 && b[bl - 1] != '/') {
        joined[bl] = '/';
        bl++;
    }
    memcpy(joined + bl, l, ll);
    joined[bl + ll] = 0;
    uint64_t h = 0;
    string_store(&h, joined);
    free(joined);
    return h;
}

uint64_t host_str_concat(uint64_t lhs, uint64_t rhs) {
    const char *a = cstring_from_handle(lhs);
    const char *b = cstring_from_handle(rhs);
    if (!a) return rhs;
    if (!b) return lhs;
    size_t al = strlen(a), bl = strlen(b);
    char *s = malloc(al + bl + 1);
    if (!s) return 0;
    memcpy(s, a, al);
    memcpy(s + al, b, bl);
    s[al + bl] = 0;
    uint64_t h = 0;
    string_store(&h, s);
    free(s);
    return h;
}

uint32_t host_str_eq(uint64_t lhs, uint64_t rhs) {
    const char *a = cstring_from_handle(lhs);
    const char *b = cstring_from_handle(rhs);
    if (!a || !b) return 0;
    return strcmp(a, b) == 0 ? 1 : 0;
}

uint32_t host_str_ends_with(uint64_t text, uint64_t suffix) {
    const char *t = cstring_from_handle(text);
    const char *s = cstring_from_handle(suffix);
    if (!t || !s) return 0;
    size_t tl = strlen(t), sl = strlen(s);
    if (sl > tl) return 0;
    return strcmp(t + tl - sl, s) == 0 ? 1 : 0;
}

uint32_t host_str_starts_with(uint64_t text, uint64_t prefix) {
    const char *t = cstring_from_handle(text);
    const char *p = cstring_from_handle(prefix);
    if (!t || !p) return 0;
    size_t pl = strlen(p);
    return strncmp(t, p, pl) == 0 ? 1 : 0;
}

uint64_t host_str_hash64(uint64_t text) {
    const char *t = cstring_from_handle(text);
    if (!t) return 0;
    return str_hash(t);
}

/* ========================================================================
 * LAYER 0: Filesystem operations
 * ======================================================================== */

uint64_t host_fs_file_size(uint64_t path) {
    const char *p = cstring_from_handle(path);
    if (!p) return 0;
    return file_size_by_path(p);
}

uint8_t host_fs_read_u8(uint64_t path, uint64_t offset) {
    const char *p = cstring_from_handle(path);
    if (!p) return 0;
    return read_u8_at_path(p, offset);
}

uint32_t host_fs_write_u8(uint64_t path, uint64_t offset, uint8_t value) {
    const char *p = cstring_from_handle(path);
    if (!p) return ERR_WRITE_FAILED;
    return write_u8_at_path(p, offset, value) == 0 ? ERR_OK : ERR_WRITE_FAILED;
}

uint32_t host_fs_truncate(uint64_t path, uint64_t size) {
    const char *p = cstring_from_handle(path);
    if (!p) return ERR_WRITE_FAILED;
    return truncate_file(p, size) == 0 ? ERR_OK : ERR_WRITE_FAILED;
}

uint32_t host_fs_append_line(uint64_t path, uint64_t line) {
    const char *p = cstring_from_handle(path);
    const char *l = cstring_from_handle(line);
    if (!p || !l) return ERR_WRITE_FAILED;
    return append_line_to_path(p, l) == 0 ? ERR_OK : ERR_WRITE_FAILED;
}

/* ========================================================================
 * Archive helpers (minimal implementations)
 * ======================================================================== */

uint32_t host_archive_validate_magic(uint64_t path) {
    const char *p = cstring_from_handle(path);
    if (!p || !path_exists(p)) return ERR_FILE_NOT_FOUND;
    unsigned char magic[8];
    FILE *f = fopen(p, "rb");
    if (!f) return ERR_FILE_NOT_FOUND;
    size_t rd = fread(magic, 1, 8, f);
    fclose(f);
    if (rd != 8) return ERR_INVALID_FORMAT;
    if (memcmp(magic, "!<arch>\n", 8) != 0) return ERR_INVALID_FORMAT;
    return ERR_OK;
}

/* ========================================================================
 * LAYER 1: Linker State - Setters
 * ======================================================================== */

static void reset_linker_state(void) {
    memset(&g_state, 0, sizeof(g_state));
    g_state.output_format = FORMAT_ELF64;
    g_state.target = TARGET_NONE;
    g_state.image_base = 0x100000;  /* 1MB default */
    g_state.entry = 0x100000;
    g_state.build_id_mode = 0;  /* none */
    g_state.hash_style = 0;     /* none */
    g_state.unresolved_policy = 0; /* report all */
    g_state.use_default_search_paths = 1;
    g_state.z_relro = 1;
    g_state.z_now = 0;
    g_state.link_copy_dt_needed_entries = 1;  /* default: on */
}

uint32_t host_linker_set_output_path(uint64_t path) {
    g_state.output_path = path;
    return ERR_OK;
}

uint32_t host_linker_set_output_format(uint32_t format) {
    g_state.output_format = format;
    return ERR_OK;
}

uint32_t host_linker_set_target(uint32_t target) {
    g_state.target = target;
    return ERR_OK;
}

uint32_t host_linker_set_entry(uint64_t entry) {
    g_state.entry = entry;
    return ERR_OK;
}

uint32_t host_linker_set_image_base(uint64_t base) {
    g_state.image_base = base;
    return ERR_OK;
}

uint32_t host_linker_set_gc_sections(uint32_t enabled) {
    g_state.gc_sections = enabled ? 1 : 0;
    return ERR_OK;
}

uint32_t host_linker_set_allow_multiple_definition(uint32_t enabled) {
    g_state.allow_multiple_definition = enabled ? 1 : 0;
    return ERR_OK;
}

uint32_t host_linker_enable_build_id_default(void) {
    g_state.build_id_mode = 1;  /* fast */
    g_state.z_relro = 1;
    return ERR_OK;
}

uint32_t host_linker_set_build_id(uint64_t spec) {
    const char *s = cstring_from_handle(spec);
    if (!s) {
        g_state.build_id_mode = 1;  /* fast default */
        g_state.build_id_len = 0;
        return ERR_OK;
    }
    if (strcmp(s, "none") == 0 || strcmp(s, "0") == 0) { g_state.build_id_mode = 0; }
    else if (strcmp(s, "fast") == 0) { g_state.build_id_mode = 1; }
    else if (strcmp(s, "md5") == 0) { g_state.build_id_mode = 2; }
    else if (strcmp(s, "sha1") == 0) { g_state.build_id_mode = 3; }
    else if (strcmp(s, "uuid") == 0) { g_state.build_id_mode = 4; }
    else {
        /* hex byte string */
        g_state.build_id_mode = 5;  /* hex */
        g_state.build_id_len = (strlen(s) + 1) / 2;
        if (g_state.build_id_len > 64) g_state.build_id_len = 64;
        for (uint32_t i = 0; i < g_state.build_id_len; i++) {
            unsigned v;
            sscanf(s + 2 * i, "%02x", &v);
            g_state.build_id_data[i] = (uint8_t)v;
        }
    }
    return ERR_OK;
}

uint32_t host_linker_set_z_option(uint64_t option) {
    const char *s = cstring_from_handle(option);
    if (!s) return ERR_OK;
    if (strcmp(s, "relro") == 0) { g_state.z_relro = 1; }
    else if (strcmp(s, "norelro") == 0) { g_state.z_relro = 0; }
    else if (strcmp(s, "now") == 0) { g_state.z_now = 1; }
    else if (strcmp(s, "lazy") == 0) { g_state.z_now = 0; }
    else if (strcmp(s, "execstack") == 0) { g_state.z_execstack = 1; }
    else if (strcmp(s, "noexecstack") == 0) { g_state.z_execstack = 0; }
    else if (strcmp(s, "origin") == 0) { g_state.z_origin = 1; }
    else if (strcmp(s, "interpose") == 0) { g_state.z_interpose = 1; }
    else if (strcmp(s, "initfirst") == 0) { g_state.z_initfirst = 1; }
    else if (strcmp(s, "nodelete") == 0) { g_state.z_nodelete = 1; }
    else { /* accepted but ignored: text, notext, defs, undefs */ }
    return ERR_OK;
}

static int z_flag_bitmask(void) {
    int bits = 0;
    if (g_state.z_relro)  bits |= 1 << 0;
    if (g_state.z_now)    bits |= 1 << 1;
    if (g_state.z_execstack) bits |= 1 << 2;
    if (g_state.z_origin) bits |= 1 << 3;
    if (g_state.z_interpose) bits |= 1 << 4;
    if (g_state.z_initfirst) bits |= 1 << 5;
    return bits;
}


/* ========================================================================
 * LAYER 1: Linker State - Getters (bulk of simple setters/getters)
 * ======================================================================== */

uint64_t host_linker_get_output_path(void) { return g_state.output_path; }
uint32_t host_linker_get_output_format(void) { return g_state.output_format; }
uint32_t host_linker_get_target(void) { return g_state.target; }
uint64_t host_linker_get_entry(void) { return g_state.entry; }
uint64_t host_linker_get_image_base(void) { return g_state.image_base; }
uint32_t host_linker_get_gc_sections(void) { return g_state.gc_sections; }
uint32_t host_linker_get_allow_multiple_definition(void) { return g_state.allow_multiple_definition; }
uint32_t host_linker_get_build_id_mode(void) { return g_state.build_id_mode; }
uint32_t host_linker_get_z_flags(void) { return z_flag_bitmask(); }
uint64_t host_linker_get_dynamic_dt_flags(void) {
    uint64_t flags = 0;
    if (g_state.z_now) flags |= (1ULL << 1);  /* DF_1_NOW via z_now */
    if (g_state.z_relro) flags |= (1ULL << 0); /* relro bit */
    return flags;
}
uint64_t host_linker_get_dynamic_dt_flags_1(void) {
    uint64_t flags = 0;
    if (g_state.z_now) flags |= 1;      /* DF_1_NOW */
    if (g_state.z_origin) flags |= 0x80; /* DF_1_ORIGIN */
    if (g_state.z_interpose) flags |= 0x400; /* DF_1_INTERPOSE */
    if (g_state.z_initfirst) flags |= 0x20; /* DF_1_INITFIRST */
    if (g_state.z_nodelete) flags |= 8; /* DF_1_NODELETE */
    return flags;
}

uint32_t host_linker_set_arch(uint32_t arch) { g_state.arch = arch; return ERR_OK; }
uint32_t host_linker_set_os(uint32_t os) { g_state.os = os; return ERR_OK; }

uint32_t host_linker_set_static(uint32_t enabled) { g_state.link_static = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_static(void) { return g_state.link_static; }
uint32_t host_linker_set_shared(uint32_t enabled) { g_state.link_shared = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_shared(void) { return g_state.link_shared; }
uint32_t host_linker_set_pie(uint32_t enabled) { g_state.link_pie = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_pie(void) { return g_state.link_pie; }
uint32_t host_linker_set_as_needed(uint32_t enabled) { g_state.link_as_needed = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_as_needed(void) { return g_state.link_as_needed; }
uint32_t host_linker_set_symbolic_bindings(uint32_t enabled) { g_state.symbolic_bindings = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_symbolic_bindings(void) { return g_state.symbolic_bindings; }
uint32_t host_linker_set_symbolic_functions(uint32_t enabled) { g_state.symbolic_functions = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_symbolic_functions(void) { return g_state.symbolic_functions; }
uint32_t host_linker_set_symbolic_group(uint32_t enabled) { g_state.symbolic_group = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_symbolic_group(void) { return g_state.symbolic_group; }
uint32_t host_linker_set_new_dtags(uint32_t enabled) { g_state.link_new_dtags = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_new_dtags(void) { return g_state.link_new_dtags; }
uint32_t host_linker_set_copy_dt_needed_entries(uint32_t enabled) { g_state.link_copy_dt_needed_entries = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_copy_dt_needed_entries(void) { return g_state.link_copy_dt_needed_entries; }

uint32_t host_linker_set_no_undefined(uint32_t enabled) { g_state.no_undefined = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_no_undefined(void) { return g_state.no_undefined; }
uint32_t host_linker_set_allow_shlib_undefined(uint32_t enabled) { g_state.allow_shlib_undefined = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_allow_shlib_undefined(void) { return g_state.allow_shlib_undefined; }
uint32_t host_linker_set_unresolved_policy(uint32_t policy) { g_state.unresolved_policy = policy; return ERR_OK; }
uint32_t host_linker_get_unresolved_policy(void) { return g_state.unresolved_policy; }
uint32_t host_linker_set_warn_unresolved(uint32_t enabled) { g_state.warn_unresolved = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_warn_unresolved(void) { return g_state.warn_unresolved; }

uint32_t host_linker_set_trace(uint32_t enabled) { g_state.trace_enabled = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_trace(void) { return g_state.trace_enabled; }
uint32_t host_linker_add_trace_symbol(uint64_t name) {
    return u64_vec_push(&g_state.trace_symbol_hashes, name);
}

uint32_t host_linker_set_whole_archive(uint32_t enabled) { g_state.whole_archive = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_whole_archive(void) { return g_state.whole_archive; }

uint32_t host_linker_set_hash_style(uint64_t style) {
    const char *s = cstring_from_handle(style);
    if (!s) {
        g_state.hash_style = 0;
        return ERR_OK;
    }
    if (strcmp(s, "sysv") == 0) g_state.hash_style = 1;
    else if (strcmp(s, "gnu") == 0) g_state.hash_style = 2;
    else if (strcmp(s, "both") == 0) g_state.hash_style = 3;
    else g_state.hash_style = 0;
    return ERR_OK;
}
uint32_t host_linker_get_hash_style(void) { return g_state.hash_style; }
uint32_t host_linker_set_thread_count(uint32_t count) { g_state.thread_count = count; return ERR_OK; }
uint32_t host_linker_get_thread_count(void) { return g_state.thread_count; }
uint32_t host_linker_set_eh_frame_hdr(uint32_t enabled) { g_state.emit_eh_frame_hdr = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_set_fatal_warnings(uint32_t enabled) { g_state.fatal_warnings = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_fatal_warnings(void) { return g_state.fatal_warnings; }
uint32_t host_linker_set_color_diagnostics(uint32_t enabled) { g_state.color_diagnostics = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_color_diagnostics(void) { return g_state.color_diagnostics; }
uint32_t host_linker_set_print_gc_sections(uint32_t enabled) { g_state.print_gc_sections = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_print_gc_sections(void) { return g_state.print_gc_sections; }
uint32_t host_linker_set_emit_relocs(uint32_t enabled) { g_state.emit_relocs = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_emit_relocs(void) { return g_state.emit_relocs; }
uint32_t host_linker_set_icf_mode(uint32_t mode) { g_state.icf_mode = mode; return ERR_OK; }
uint32_t host_linker_get_icf_mode(void) { return g_state.icf_mode; }

uint32_t host_linker_set_pe_no_entry(uint32_t enabled) { g_state.pe_no_entry = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_pe_no_entry(void) { return g_state.pe_no_entry; }
uint32_t host_linker_set_pe_dynamic_base(uint32_t enabled) { g_state.pe_dynamic_base = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_pe_dynamic_base(void) { return g_state.pe_dynamic_base; }
uint32_t host_linker_set_pe_nx_compat(uint32_t enabled) { g_state.pe_nx_compat = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_pe_nx_compat(void) { return g_state.pe_nx_compat; }
uint32_t host_linker_set_pe_large_address_aware(uint32_t enabled) { g_state.pe_large_address_aware = enabled ? 1 : 0; return ERR_OK; }
uint32_t host_linker_get_pe_large_address_aware(void) { return g_state.pe_large_address_aware; }

uint32_t host_linker_strip_debug(void) { g_state.strip_debug = 1; return ERR_OK; }

uint32_t host_linker_set_use_default_search_paths(uint32_t enabled) {
    g_state.use_default_search_paths = enabled ? 1 : 0;
    return ERR_OK;
}
uint32_t host_linker_get_use_default_search_paths(void) {
    return g_state.use_default_search_paths;
}

uint32_t host_linker_set_sysroot(uint64_t path) {
    g_state.sysroot_path = path;
    return ERR_OK;
}
uint64_t host_linker_get_sysroot(void) { return g_state.sysroot_path; }

uint32_t host_linker_set_dependency_file(uint64_t path) {
    g_state.dependency_file = path;
    return ERR_OK;
}
uint64_t host_linker_get_dependency_file(void) { return g_state.dependency_file; }

uint32_t host_linker_set_dynamic_linker(uint64_t path) {
    g_state.dynamic_linker_path = path;
    return ERR_OK;
}
uint64_t host_linker_get_dynamic_linker(void) { return g_state.dynamic_linker_path; }

uint32_t host_linker_set_soname(uint64_t name) {
    g_state.soname = name;
    return ERR_OK;
}
uint64_t host_linker_get_soname(void) { return g_state.soname; }

/* ========================================================================
 * LAYER 1: Search Paths
 * ======================================================================== */

uint32_t host_linker_add_search_path(uint64_t path) {
    uint64_t h = 0;
    if (string_store(&h, cstring_from_handle(path)) == 0) {
        return u64_vec_push(&g_state.search_paths, h);
    }
    return ERR_NOT_IMPLEMENTED_YET;
}

uint32_t host_linker_search_path_count(void) {
    return g_state.search_paths.count;
}

uint64_t host_linker_get_search_path(uint32_t index) {
    if (index >= g_state.search_paths.count) return 0;
    return g_state.search_paths.items[index];
}

uint32_t host_linker_add_rpath(uint64_t path) {
    uint64_t h = 0;
    if (string_store(&h, cstring_from_handle(path)) == 0) {
        return u64_vec_push(&g_state.rpaths, h);
    }
    return ERR_NOT_IMPLEMENTED_YET;
}

uint32_t host_linker_rpath_count(void) { return g_state.rpaths.count; }
uint64_t host_linker_get_rpath(uint32_t index) {
    if (index >= g_state.rpaths.count) return 0;
    return g_state.rpaths.items[index];
}

uint32_t host_linker_add_rpath_link(uint64_t path) {
    uint64_t h = 0;
    if (string_store(&h, cstring_from_handle(path)) == 0) {
        return u64_vec_push(&g_state.rpath_links, h);
    }
    return ERR_NOT_IMPLEMENTED_YET;
}

uint32_t host_linker_rpath_link_count(void) { return g_state.rpath_links.count; }
uint64_t host_linker_get_rpath_link(uint32_t index) {
    if (index >= g_state.rpath_links.count) return 0;
    return g_state.rpath_links.items[index];
}

uint32_t host_linker_default_search_path_count(void) {
    (void)g_state;
    return 0; /* No default paths in standalone mode */
}
uint64_t host_linker_get_default_search_path(uint32_t index) { (void)index; return 0; }

uint32_t host_linker_block_default_library(uint64_t name) {
    const char *n = cstring_from_handle(name);
    if (!n) return ERR_INVALID_FORMAT;
    uint64_t h = str_hash(n);
    if (g_state.blocked_lib_count < MAX_DEFERRED) {
        g_state.blocked_lib_hashes[g_state.blocked_lib_count++] = h;
    }
    return ERR_OK;
}

uint32_t host_linker_is_default_library_blocked(uint64_t name) {
    const char *n = cstring_from_handle(name);
    if (!n) return 0;
    uint64_t h = str_hash(n);
    for (uint32_t i = 0; i < g_state.blocked_lib_count; i++) {
        if (g_state.blocked_lib_hashes[i] == h) return 1;
    }
    return 0;
}

/* ========================================================================
 * LAYER 1: Input registration & archives
 * ======================================================================== */

uint32_t host_linker_register_input(uint64_t path) {
    return u64_vec_push(&g_state.inputs, path);
}

uint32_t host_linker_register_archive(uint64_t path) {
    return u64_vec_push(&g_state.archives, path);
}

uint32_t host_linker_group_push_start(void) {
    return u32_vec_push(&g_state.group_archive_starts, g_state.archive_progress);
}

uint32_t host_linker_group_pop_start(void) {
    if (g_state.group_archive_starts.count == 0) return ERR_OK;
    g_state.group_archive_starts.count--;
    return ERR_OK;
}

uint32_t host_linker_group_depth(void) { return g_state.group_archive_starts.count; }

uint32_t host_linker_start_lib_push_start(void) {
    return u32_vec_push(&g_state.group_start_lib_starts, g_state.start_lib_progress);
}
uint32_t host_linker_start_lib_pop_start(void) {
    if (g_state.group_start_lib_starts.count == 0) return ERR_OK;
    g_state.group_start_lib_starts.count--;
    return ERR_OK;
}
uint32_t host_linker_start_lib_depth(void) { return g_state.group_start_lib_starts.count; }

uint32_t host_linker_start_lib_register_member(uint64_t path) {
    return u64_vec_push(&g_state.start_lib_members, path);
}
uint32_t host_linker_start_lib_member_count(void) { return g_state.start_lib_members.count; }
uint64_t host_linker_get_start_lib_member(uint32_t index) {
    if (index >= g_state.start_lib_members.count) return 0;
    return g_state.start_lib_members.items[index];
}
uint32_t host_linker_start_lib_member_is_loaded(uint32_t index) {
    for (uint32_t i = 0; i < g_state.loaded_start_lib_members.count; i++) {
        if (g_state.loaded_start_lib_members.items[i] == index) return 1;
    }
    return 0;
}
uint32_t host_linker_start_lib_member_mark_loaded(uint32_t index) {
    return u32_vec_push(&g_state.loaded_start_lib_members, index);
}
uint32_t host_linker_start_lib_member_matches_unresolved(uint32_t index) {
    /* Check if this member provides any unresolved symbols */
    (void)index;
    return 0;
}
uint32_t host_linker_start_lib_progress_reset(void) {
    g_state.start_lib_progress = 0;
    return ERR_OK;
}
uint32_t host_linker_start_lib_progress_add(uint32_t delta) {
    g_state.start_lib_progress += delta;
    return ERR_OK;
}
uint32_t host_linker_start_lib_progress_get(void) {
    return g_state.start_lib_progress;
}

uint32_t host_linker_archive_count(void) { return g_state.archives.count; }
uint64_t host_linker_get_archive(uint32_t index) {
    if (index >= g_state.archives.count) return 0;
    return g_state.archives.items[index];
}
uint32_t host_linker_archive_progress_reset(void) {
    g_state.archive_progress = 0;
    return ERR_OK;
}
uint32_t host_linker_archive_progress_add(uint32_t delta) {
    g_state.archive_progress += delta;
    return ERR_OK;
}
uint32_t host_linker_archive_progress_get(void) {
    return g_state.archive_progress;
}

/* ========================================================================
 * LAYER 1: Needed shared libraries
 * ======================================================================== */

uint32_t host_linker_note_needed_library(uint64_t name, uint32_t is_shared) {
    uint64_t h = 0;
    if (string_store(&h, cstring_from_handle(name)) == 0) {
        return u64_vec_push(&g_state.needed_shared_libs, h);
    }
    (void)is_shared;
    return ERR_OK;
}

uint32_t host_linker_needed_library_count(void) {
    return g_state.needed_shared_libs.count;
}
uint64_t host_linker_get_needed_library(uint32_t index) {
    if (index >= g_state.needed_shared_libs.count) return 0;
    return g_state.needed_shared_libs.items[index];
}

uint32_t host_linker_last_shared_object_retained(void) {
    return g_state.last_shared_object_retained;
}

/* ========================================================================
 * LAYER 1: Apply defsym & require symbol
 * ======================================================================== */

uint32_t host_linker_apply_defsym(uint64_t spec) {
    const char *s = cstring_from_handle(spec);
    if (!s) return ERR_INVALID_FORMAT;
    /* Format: "name=value" or "name=0x1234" */
    const char *eq = strchr(s, '=');
    if (!eq) return ERR_INVALID_FORMAT;
    /* Store the name hash as a required symbol; full handling done later */
    char name[256];
    size_t nlen = (size_t)(eq - s);
    if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
    memcpy(name, s, nlen);
    name[nlen] = 0;
    uint64_t h = str_hash(name);
    return u64_vec_push(&g_state.required_symbols, h);
}

uint32_t host_linker_require_symbol(uint64_t name) {
    const char *n = cstring_from_handle(name);
    if (!n) return ERR_INVALID_FORMAT;
    uint64_t h = str_hash(n);
    return u64_vec_push(&g_state.required_symbols, h);
}

uint32_t host_linker_check_required_symbols(void) {
    for (uint32_t i = 0; i < g_state.required_symbols.count; i++) {
        uint64_t h = g_state.required_symbols.items[i];
        if (h == 0) continue;
        if (find_global(h) < 0) return ERR_UNDEFINED_SYMBOL;
    }
    return ERR_OK;
}

uint32_t host_linker_required_symbol_count(void) {
    uint32_t c = 0;
    for (uint32_t i = 0; i < g_state.required_symbols.count; i++) {
        if (g_state.required_symbols.items[i] != 0) c++;
    }
    return c;
}


/* ========================================================================
 * LAYER 1: Reset state & push/pop state
 * ======================================================================== */

uint32_t host_linker_reset_state(void) {
    reset_linker_state();
    return ERR_OK;
}

uint32_t host_linker_push_state(void) {
    LinkerOptionState *st = &g_state.opt_stack[g_state.opt_stack_top];
    st->link_static = g_state.link_static;
    st->link_as_needed = g_state.link_as_needed;
    st->symbolic_bindings = g_state.symbolic_bindings;
    st->symbolic_functions = g_state.symbolic_functions;
    st->symbolic_group = g_state.symbolic_group;
    st->z_origin = g_state.z_origin;
    st->z_interpose = g_state.z_interpose;
    st->z_initfirst = g_state.z_initfirst;
    st->z_nodelete = g_state.z_nodelete;
    st->use_default_search_paths = g_state.use_default_search_paths;
    st->whole_archive = g_state.whole_archive;
    st->link_new_dtags = g_state.link_new_dtags;
    st->link_copy_dt_needed_entries = g_state.link_copy_dt_needed_entries;
    st->no_undefined = g_state.no_undefined;
    st->allow_shlib_undefined = g_state.allow_shlib_undefined;
    st->unresolved_policy = g_state.unresolved_policy;
    st->warn_unresolved = g_state.warn_unresolved;
    st->trace_enabled = g_state.trace_enabled;
    g_state.opt_stack_top++;
    return ERR_OK;
}

uint32_t host_linker_pop_state(void) {
    if (g_state.opt_stack_top == 0) return ERR_OK;
    g_state.opt_stack_top--;
    LinkerOptionState *st = &g_state.opt_stack[g_state.opt_stack_top];
    g_state.link_static = st->link_static;
    g_state.link_as_needed = st->link_as_needed;
    g_state.symbolic_bindings = st->symbolic_bindings;
    g_state.symbolic_functions = st->symbolic_functions;
    g_state.symbolic_group = st->symbolic_group;
    g_state.z_origin = st->z_origin;
    g_state.z_interpose = st->z_interpose;
    g_state.z_initfirst = st->z_initfirst;
    g_state.z_nodelete = st->z_nodelete;
    g_state.use_default_search_paths = st->use_default_search_paths;
    g_state.whole_archive = st->whole_archive;
    g_state.link_new_dtags = st->link_new_dtags;
    g_state.link_copy_dt_needed_entries = st->link_copy_dt_needed_entries;
    g_state.no_undefined = st->no_undefined;
    g_state.allow_shlib_undefined = st->allow_shlib_undefined;
    g_state.unresolved_policy = st->unresolved_policy;
    g_state.warn_unresolved = st->warn_unresolved;
    g_state.trace_enabled = st->trace_enabled;
    return ERR_OK;
}

/* ========================================================================
 * LAYER 1: Symbol resolution
 * ======================================================================== */

static void add_or_update_global(uint64_t name_hash, uint32_t obj_idx,
                                 uint32_t sym_idx, uint8_t bind) {
    int idx = find_global(name_hash);
    if (idx < 0) {
        if (g_state.globals_count >= g_state.globals_cap) {
            uint32_t nc = g_state.globals_cap ? g_state.globals_cap * 2 : 1024;
            ObjectSymbol *ns = (ObjectSymbol *)realloc(g_state.globals_sym, nc * sizeof(ObjectSymbol));
            uint64_t *nh = (uint64_t *)realloc(g_state.globals_hash, nc * sizeof(uint64_t));
            if (!ns || !nh) return; /* OOM fallback */
            memset(ns + g_state.globals_cap, 0, (nc - g_state.globals_cap) * sizeof(ObjectSymbol));
            memset(nh + g_state.globals_cap, 0, (nc - g_state.globals_cap) * sizeof(uint64_t));
            g_state.globals_sym = ns;
            g_state.globals_hash = nh;
            g_state.globals_cap = nc;
        }
        g_state.globals_hash[g_state.globals_count] = name_hash;
        g_state.globals_sym[g_state.globals_count].object_index = obj_idx;
        g_state.globals_sym[g_state.globals_count].symbol_index = sym_idx;
        g_state.globals_sym[g_state.globals_count].bind = bind;
        g_state.globals_sym[g_state.globals_count].defined = 1;
        g_state.globals_sym[g_state.globals_count].address = 0;
        g_state.globals_count++;
    } else {
        /* Already exists - keep existing for strong, override for stronger */
        uint8_t existing_bind = g_state.globals_sym[idx].bind;
        if (bind == 1 && existing_bind == 2) {
            /* Global overrides weak */
            g_state.globals_sym[idx].object_index = obj_idx;
            g_state.globals_sym[idx].symbol_index = sym_idx;
            g_state.globals_sym[idx].bind = bind;
        }
        /* else keep existing (strong >= global >= weak, first wins) */
    }
}

uint64_t host_linker_global_symbol_address(uint64_t name_hash) {
    int idx = find_global(name_hash);
    if (idx < 0) return 0;
    return g_state.globals_sym[idx].address;
}

uint32_t host_linker_global_symbol_defined(uint64_t name_hash) {
    return find_global(name_hash) >= 0 ? 1 : 0;
}

uint8_t host_linker_global_symbol_bind(uint64_t name_hash) {
    int idx = find_global(name_hash);
    if (idx < 0) return 0;
    return g_state.globals_sym[idx].bind;
}

uint32_t host_linker_global_symbol_count(void) {
    return g_state.globals_count;
}

uint32_t host_linker_global_symbol_define_absolute(uint64_t name_hash, uint64_t value) {
    int idx = find_global(name_hash);
    if (idx < 0) {
        if (g_state.globals_count >= g_state.globals_cap) {
            uint32_t nc = g_state.globals_cap ? g_state.globals_cap * 2 : 1024;
            ObjectSymbol *ns = (ObjectSymbol *)realloc(g_state.globals_sym, nc * sizeof(ObjectSymbol));
            uint64_t *nh = (uint64_t *)realloc(g_state.globals_hash, nc * sizeof(uint64_t));
            if (!ns || !nh) return ERR_NOT_IMPLEMENTED_YET; /* OOM fallback */
            memset(ns + g_state.globals_cap, 0, (nc - g_state.globals_cap) * sizeof(ObjectSymbol));
            memset(nh + g_state.globals_cap, 0, (nc - g_state.globals_cap) * sizeof(uint64_t));
            g_state.globals_sym = ns;
            g_state.globals_hash = nh;
            g_state.globals_cap = nc;
        }
        g_state.globals_hash[g_state.globals_count] = name_hash;
        g_state.globals_sym[g_state.globals_count].object_index = 0;
        g_state.globals_sym[g_state.globals_count].symbol_index = 0;
        g_state.globals_sym[g_state.globals_count].bind = 1;
        g_state.globals_sym[g_state.globals_count].defined = 1;
        g_state.globals_sym[g_state.globals_count].address = value;
        g_state.globals_count++;
        return ERR_OK;
    }
    g_state.globals_sym[idx].address = value;
    g_state.globals_sym[idx].defined = 1;
    return ERR_OK;
}

uint32_t host_linker_global_symbol_set(uint64_t name_hash, uint32_t obj_index,
                                          uint32_t sym_index, uint8_t bind,
                                          uint32_t defined) {
    if (defined == 0) return ERR_OK;
    add_or_update_global(name_hash, obj_index, sym_index, bind);
    return ERR_OK;
}

uint32_t host_linker_unresolved_symbol_count(void) {
    uint32_t c = 0;
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sym_count; si++) {
            ObjectSymbol *sym = &obj->symbols[si];
            if (sym->shndx == SHN_UNDEF && sym->bind != 0 && sym->name_hash != 0) {
                if (find_global(sym->name_hash) < 0) {
                    c++;
                }
            }
        }
    }
    return c;
}

uint32_t host_linker_weak_fallback_count(void) {
    uint32_t c = 0;
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sym_count; si++) {
            ObjectSymbol *sym = &obj->symbols[si];
            if (sym->shndx == SHN_UNDEF && sym->bind == 2 && sym->name_hash != 0) {
                if (find_global(sym->name_hash) < 0) {
                    c++;
                }
            }
        }
    }
    return c;
}

uint32_t host_linker_shared_undefined_symbol_count(void) {
    return g_state.shared_undefined_count;
}

uint64_t host_linker_resolved_symbol_address(uint32_t obj_index, uint32_t sym_index) {
    if (obj_index >= g_state.object_count) return 0;
    ObjectRecord *obj = &g_state.objects[obj_index];
    if (sym_index >= obj->sym_count) return 0;
    ObjectSymbol *sym = &obj->symbols[sym_index];
    return sym->value;
}

uint32_t host_linker_allow_dynamic_unresolved(void) {
    return g_state.allow_shlib_undefined ? 1 : 0;
}

/* ========================================================================
 * LAYER 2: Object ingestion (ELF, COFF, Mach-O)
 * ======================================================================== */

/* Object format detection */
uint32_t host_linker_probe_object_format(uint64_t path_h) {
    const char *p = cstring_from_handle(path_h);
    if (!p || !path_exists(p)) return OBJECT_FORMAT_UNKNOWN;
    uint64_t fsz = file_size_by_path(p);
    if (fsz < 64) return OBJECT_FORMAT_UNKNOWN;

    /* Check ELF magic: \x7fELF */
    if (read_u8_at_path(p, 0) == 0x7f &&
        read_u8_at_path(p, 1) == 'E' &&
        read_u8_at_path(p, 2) == 'L' &&
        read_u8_at_path(p, 3) == 'F') {
        return OBJECT_FORMAT_ELF64;
    }

    /* Check COFF magic (little-endian machine type) */
    uint16_t machine = read_u16_le_path(p, 0);
    if (machine == COFF_MACHINE_X86_64 || machine == COFF_MACHINE_AARCH64) {
        return OBJECT_FORMAT_COFF64;
    }

    /* Check Mach-O magic (0xfeedface or 0xfeedfacf) */
    uint32_t magic = read_u32_le_path(p, 0);
    if (magic == 0xfeedfacf || magic == 0xcefaedfe) {
        return OBJECT_FORMAT_MACHO64;
    }

    return OBJECT_FORMAT_UNKNOWN;
}

static uint32_t host_linker_object_begin(uint64_t path, uint64_t file_size, uint16_t elf_type, uint16_t machine);
uint32_t host_linker_object_add_section(uint32_t index, uint32_t sec_type, uint64_t flags, uint64_t offset, uint64_t size, uint32_t link, uint32_t info, uint64_t align, uint64_t entsize);
uint32_t host_linker_object_finalize(uint64_t path);

uint32_t host_linker_ingest_coff_object(uint64_t path) {
    const char *p = cstring_from_handle(path);
    if (!p || !path_exists(p)) return ERR_FILE_NOT_FOUND;
    uint64_t fsz = file_size_by_path(p);
    
    FILE *f = fopen(p, "rb");
    if (!f) return ERR_FILE_NOT_FOUND;
    
    uint16_t machine = 0;
    uint16_t num_sections = 0;
    uint32_t sym_table_val = 0;
    uint32_t num_syms = 0;
    
    fread(&machine, 1, 2, f);
    fread(&num_sections, 1, 2, f);
    fseek(f, 4, SEEK_CUR); /* TimeDateStamp */
    fread(&sym_table_val, 1, 4, f);
    fread(&num_syms, 1, 4, f);
    fseek(f, 4, SEEK_CUR); /* SizeOfOptionalHeader + Characteristics */
    
    uint32_t res = host_linker_object_begin(path, fsz, 1 /* REL */, machine);
    if (res != ERR_OK) { fclose(f); return res; }
    
    /* Read sections */
    for (uint16_t i = 1; i <= num_sections; i++) {
        char name[8];
        uint32_t v_size, v_addr, raw_data_size, ptr_raw_data;
        uint32_t ptr_relocs, ptr_linenums;
        uint16_t num_reloc, num_linenums;
        uint32_t characts;
        
        fread(name, 1, 8, f);
        fread(&v_size, 1, 4, f);
        fread(&v_addr, 1, 4, f);
        fread(&raw_data_size, 1, 4, f);
        fread(&ptr_raw_data, 1, 4, f);
        fread(&ptr_relocs, 1, 4, f);
        fread(&ptr_linenums, 1, 4, f);
        fread(&num_reloc, 1, 2, f);
        fread(&num_linenums, 1, 2, f);
        fread(&characts, 1, 4, f);
        
        host_linker_object_add_section(i, 1 /* PROGBITS */, characts, ptr_raw_data, raw_data_size, 0, 0, 1, 0);
    }
    
    fclose(f);
    host_linker_object_finalize(path);
    return ERR_OK;
}

uint32_t host_linker_ingest_macho_object(uint64_t path) {
    const char *p = cstring_from_handle(path);
    if (!p || !path_exists(p)) return ERR_FILE_NOT_FOUND;
    uint64_t fsz = file_size_by_path(p);
    
    FILE *f = fopen(p, "rb");
    if (!f) return ERR_FILE_NOT_FOUND;
    
    uint32_t magic = 0, cputype = 0, cpusubtype = 0, filetype = 0;
    uint32_t ncmds = 0, sizeofcmds = 0, flags = 0, reserved = 0;
    
    fread(&magic, 1, 4, f);
    fread(&cputype, 1, 4, f);
    fread(&cpusubtype, 1, 4, f);
    fread(&filetype, 1, 4, f);
    fread(&ncmds, 1, 4, f);
    fread(&sizeofcmds, 1, 4, f);
    fread(&flags, 1, 4, f);
    fread(&reserved, 1, 4, f);
    
    uint32_t res = host_linker_object_begin(path, fsz, 1 /* REL */, cputype);
    if (res != ERR_OK) { fclose(f); return res; }
    
    uint32_t sec_index = 1;
    for (uint32_t i = 0; i < ncmds; i++) {
        uint32_t cmd = 0, cmdsize = 0;
        fread(&cmd, 1, 4, f);
        fread(&cmdsize, 1, 4, f);
        
        if (cmd == 0x19) { /* LC_SEGMENT_64 */
            fseek(f, 56, SEEK_CUR); /* Skip to nsects */
            uint32_t nsects = 0, segflags = 0;
            fread(&nsects, 1, 4, f);
            fread(&segflags, 1, 4, f);
            
            for (uint32_t j = 0; j < nsects; j++) {
                char sectname[16], segname[16];
                uint64_t addr = 0, size = 0;
                uint32_t offset = 0, align = 0, reloff = 0, nreloc = 0, secflags = 0;
                
                fread(sectname, 1, 16, f);
                fread(segname, 1, 16, f);
                fread(&addr, 1, 8, f);
                fread(&size, 1, 8, f);
                fread(&offset, 1, 4, f);
                fread(&align, 1, 4, f);
                fread(&reloff, 1, 4, f);
                fread(&nreloc, 1, 4, f);
                fread(&secflags, 1, 4, f);
                fseek(f, 12, SEEK_CUR); /* reserved 1, 2, 3 */
                
                host_linker_object_add_section(sec_index++, 1 /* PROGBITS */, secflags, offset, size, 0, 0, align, 0);
            }
        } else {
            if (cmdsize > 8) {
                fseek(f, cmdsize - 8, SEEK_CUR);
            }
        }
    }
    
    fclose(f);
    host_linker_object_finalize(path);
    return ERR_OK;
}

/* Forward declaration for host_linker_object_begin */
static uint32_t host_linker_object_begin(uint64_t path, uint64_t file_size,
                                          uint16_t elf_type, uint16_t machine);

uint32_t host_linker_ingest_shared_object(uint64_t path) {
    /* Shared object ingest - same as ELF but for ET_DYN */
    uint64_t path_h = path;
    const char *p = cstring_from_handle(path_h);
    if (!p) return ERR_FILE_NOT_FOUND;
    return host_linker_object_begin((uint64_t)(uintptr_t)p,
                                      file_size_by_path(p), 3, 62); /* ET_DYN, EM_X86_64 default */
}

/* The core object ingestion sequence called from ELF parsing */
uint32_t host_linker_object_begin(uint64_t path, uint64_t file_size,
                                     uint16_t elf_type, uint16_t machine) {
    if (g_state.object_count >= g_state.object_cap) {
        uint32_t nc = g_state.object_cap ? g_state.object_cap * 2 : 256;
        ObjectRecord *no = (ObjectRecord *)realloc(g_state.objects, nc * sizeof(ObjectRecord));
        if (!no) return ERR_NOT_IMPLEMENTED_YET;
        memset(no + g_state.object_cap, 0, (nc - g_state.object_cap) * sizeof(ObjectRecord));
        g_state.objects = no;
        g_state.object_cap = nc;
    }

    ObjectRecord *obj = &g_state.objects[g_state.object_count];
    memset(obj, 0, sizeof(ObjectRecord));
    obj->path = path;
    obj->file_size = file_size;
    obj->elf_type = elf_type;
    obj->machine = machine;
    obj->object_kind = OBJECT_FORMAT_ELF64;

    /* Allocate initial arrays */
    obj->sec_cap = 64;
    obj->sections = (ObjectSection *)calloc(obj->sec_cap, sizeof(ObjectSection));
    obj->sym_cap = 512;
    obj->symbols = (ObjectSymbol *)calloc(obj->sym_cap, sizeof(ObjectSymbol));
    obj->reloc_cap = 512;
    obj->relocs = (ObjectRelocation *)calloc(obj->reloc_cap, sizeof(ObjectRelocation));

    return ERR_OK;
}

uint32_t host_linker_object_add_section(uint32_t index, uint32_t sec_type,
    uint64_t flags, uint64_t offset, uint64_t size, uint32_t link,
    uint32_t info, uint64_t align, uint64_t entsize) {

    if (g_state.object_count == 0) return ERR_INVALID_FORMAT;
    ObjectRecord *obj = &g_state.objects[g_state.object_count - 1];
    
    if (index >= obj->sec_cap) {
        uint32_t nc = obj->sec_cap ? obj->sec_cap * 2 : 64;
        while (index >= nc) nc *= 2;
        ObjectSection *ns = (ObjectSection *)realloc(obj->sections, nc * sizeof(ObjectSection));
        if (!ns) return ERR_INVALID_SECTION;
        memset(ns + obj->sec_cap, 0, (nc - obj->sec_cap) * sizeof(ObjectSection));
        obj->sections = ns;
        obj->sec_cap = nc;
    }

    ObjectSection *sec = &obj->sections[index];
    sec->index = index;
    sec->sec_type = sec_type;
    sec->flags = flags;
    sec->offset = offset;
    sec->size = size;
    sec->link = link;
    sec->info = info;
    sec->align = align;
    sec->entsize = entsize;

    if (g_state.object_count > 0 && index >= obj->sec_count)
        obj->sec_count = index + 1;

    return ERR_OK;
}

uint32_t host_linker_object_add_symbol(uint32_t name_idx, uint8_t bind, uint8_t sym_type,
    uint16_t shndx, uint64_t value, uint64_t size, uint32_t strtab_section) {

    if (g_state.object_count == 0) return ERR_INVALID_FORMAT;
    ObjectRecord *obj = &g_state.objects[g_state.object_count - 1];
    
    if (obj->sym_count >= obj->sym_cap) {
        uint32_t nc = obj->sym_cap ? obj->sym_cap * 2 : 512;
        ObjectSymbol *ns = (ObjectSymbol *)realloc(obj->symbols, nc * sizeof(ObjectSymbol));
        if (!ns) return ERR_NOT_IMPLEMENTED_YET;
        memset(ns + obj->sym_cap, 0, (nc - obj->sym_cap) * sizeof(ObjectSymbol));
        obj->symbols = ns;
        obj->sym_cap = nc;
    }

    ObjectSymbol *sym = &obj->symbols[obj->sym_count];
    sym->name_hash = name_idx;   /* This is the strtab offset, not a real handle yet */
    sym->bind = bind;
    sym->sym_type = sym_type;
    sym->shndx = shndx;
    sym->value = value;
    sym->size = size;
    sym->strtab_section = strtab_section;
    sym->defined = (shndx != SHN_UNDEF) ? 1 : 0;

    /* If we can resolve the name now, do basic global registration */
    /* Note: name_idx here is section-relative offset into strtab, not a CString handle.
       The host_linker_object_symbol_name_hash must resolve it properly.
       For now, store the offset and compute hash on demand. */

    obj->sym_count++;
    return ERR_OK;
}

uint32_t host_linker_object_add_relocation(uint32_t section, uint64_t offset,
    uint32_t reloc_type, uint32_t symbol, uint64_t addend) {

    if (g_state.object_count == 0) return ERR_INVALID_FORMAT;
    ObjectRecord *obj = &g_state.objects[g_state.object_count - 1];
    
    if (obj->reloc_count >= obj->reloc_cap) {
        uint32_t nc = obj->reloc_cap ? obj->reloc_cap * 2 : 512;
        ObjectRelocation *nr = (ObjectRelocation *)realloc(obj->relocs, nc * sizeof(ObjectRelocation));
        if (!nr) return ERR_NOT_IMPLEMENTED_YET;
        memset(nr + obj->reloc_cap, 0, (nc - obj->reloc_cap) * sizeof(ObjectRelocation));
        obj->relocs = nr;
        obj->reloc_cap = nc;
    }

    ObjectRelocation *r = &obj->relocs[obj->reloc_count];
    r->section = section;
    r->offset = offset;
    r->reloc_type = reloc_type;
    r->symbol = symbol;
    r->addend = addend;

    obj->reloc_count++;
    return ERR_OK;
}

uint32_t host_linker_object_finalize(uint64_t path) {
    (void)path;
    if (g_state.object_count > 0) {
        /* Allocate image buffer for this object's data */
        ObjectRecord *obj = &g_state.objects[g_state.object_count - 1];
        if (path != 0) {
            const char *p = cstring_from_handle(path);
            if (p && path_exists(p)) {
                obj->image_size = file_size_by_path(p);
                obj->image_data = (uint8_t *)malloc(obj->image_size);
                if (obj->image_data) {
                    FILE *f = fopen(p, "rb");
                    if (f) {
                        fread(obj->image_data, 1, obj->image_size, f);
                        fclose(f);
                    }
                }
            }
        }
        g_state.object_count++;
    }
    return ERR_OK;
}

/* ========================================================================
 * Object introspection queries
 * ======================================================================== */

uint32_t host_linker_object_count(void) { return g_state.object_count; }

uint16_t host_linker_object_machine(uint32_t obj_index) {
    if (obj_index >= g_state.object_count) return 0;
    return g_state.objects[obj_index].machine;
}

uint32_t host_linker_object_symbol_count(uint32_t obj_index) {
    if (obj_index >= g_state.object_count) return 0;
    return g_state.objects[obj_index].sym_count;
}

uint32_t host_linker_object_relocation_count(uint32_t obj_index) {
    if (obj_index >= g_state.object_count) return 0;
    return g_state.objects[obj_index].reloc_count;
}

uint64_t host_linker_object_section_size(uint32_t obj_index, uint32_t sec_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (sec_index >= g_state.objects[obj_index].sec_count) return 0;
    return g_state.objects[obj_index].sections[sec_index].size;
}

uint64_t host_linker_object_symbol_name_hash(uint32_t obj_index, uint32_t sym_index) {
    if (obj_index >= g_state.object_count) return 0;
    ObjectRecord *obj = &g_state.objects[obj_index];
    if (sym_index >= obj->sym_count) return 0;
    ObjectSymbol *sym = &obj->symbols[sym_index];

    /* If the name_hash field already holds a hash, return it directly.
       Otherwise, compute it from the strtab offset using the in-memory image data.
       We store the strtab offset as the name_hash initially, then resolve. */

    /* For simplicity, treat the stored value as the name string handle for now.
       In a full implementation, we'd read the actual string from the object's image. */
    return sym->name_hash;
}

uint32_t host_linker_object_symbol_defined(uint32_t obj_index, uint32_t sym_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (sym_index >= g_state.objects[obj_index].sym_count) return 0;
    return g_state.objects[obj_index].symbols[sym_index].defined;
}

uint8_t host_linker_object_symbol_bind(uint32_t obj_index, uint32_t sym_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (sym_index >= g_state.objects[obj_index].sym_count) return 0;
    return g_state.objects[obj_index].symbols[sym_index].bind;
}

uint32_t host_linker_object_relocation_section(uint32_t obj_index, uint32_t reloc_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (reloc_index >= g_state.objects[obj_index].reloc_count) return 0;
    return g_state.objects[obj_index].relocs[reloc_index].section;
}

uint64_t host_linker_object_relocation_offset(uint32_t obj_index, uint32_t reloc_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (reloc_index >= g_state.objects[obj_index].reloc_count) return 0;
    return g_state.objects[obj_index].relocs[reloc_index].offset;
}

uint32_t host_linker_object_relocation_type(uint32_t obj_index, uint32_t reloc_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (reloc_index >= g_state.objects[obj_index].reloc_count) return 0;
    return g_state.objects[obj_index].relocs[reloc_index].reloc_type;
}

uint32_t host_linker_object_relocation_symbol(uint32_t obj_index, uint32_t reloc_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (reloc_index >= g_state.objects[obj_index].reloc_count) return 0;
    return g_state.objects[obj_index].relocs[reloc_index].symbol;
}

uint64_t host_linker_object_relocation_addend(uint32_t obj_index, uint32_t reloc_index) {
    if (obj_index >= g_state.object_count) return 0;
    if (reloc_index >= g_state.objects[obj_index].reloc_count) return 0;
    return g_state.objects[obj_index].relocs[reloc_index].addend;
}

uint32_t host_linker_total_symbol_count(void) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < g_state.object_count; i++) {
        total += g_state.objects[i].sym_count;
    }
    return total;
}

uint32_t host_linker_total_relocation_count(void) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < g_state.object_count; i++) {
        total += g_state.objects[i].reloc_count;
    }
    return total;
}


/* ========================================================================
 * Ingest CLI inputs: process -l flags and register files
 * ======================================================================== */
uint32_t host_linker_ingest_cli_inputs(void) {
    /* Iterates inputs registered via host_linker_register_input and
       attempts ingestion for ELF/COFF/Mach-O format */
    for (uint32_t i = 0; i < g_state.inputs.count; i++) {
        uint64_t path_h = g_state.inputs.items[i];
        uint32_t fmt = host_linker_probe_object_format(path_h);
        if (fmt == OBJECT_FORMAT_ELF64) {
            /* ELF object - will be ingested via ingest_object_file called
               from Dust-side ELF parsing code */
        }
    }
    return ERR_OK;
}

/* ========================================================================
 * Section runtime address calculations
 * ======================================================================== */
uint64_t host_linker_section_runtime_address(uint32_t obj_index, uint32_t sec_index) {
    if (obj_index >= g_state.object_count) return 0;
    ObjectRecord *obj = &g_state.objects[obj_index];
    if (sec_index >= obj->sec_count) return 0;
    (void)obj; /* obj validated but address calculation uses base only for now */
    return g_state.image_base;
}

/* ========================================================================
 * Patching / reading output section data
 * ======================================================================== */
uint32_t host_linker_patch_u32(uint32_t section_index, uint64_t offset, uint64_t value) {
    if (section_index >= g_state.output_section_count) return ERR_INVALID_SECTION;
    OutputSection *os = &g_state.output_sections[section_index];
    if (offset + 4 > os->size) return ERR_INVALID_ADDRESS;
    os->data[offset + 0] = (uint8_t)(value & 0xFF);
    os->data[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
    os->data[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
    os->data[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
    return ERR_OK;
}

uint64_t host_linker_read_u32(uint32_t section_index, uint64_t offset) {
    if (section_index >= g_state.output_section_count) return 0;
    OutputSection *os = &g_state.output_sections[section_index];
    if (offset + 4 > os->size) return 0;
    uint32_t v = (uint32_t)os->data[offset]
               | ((uint32_t)os->data[offset + 1] << 8)
               | ((uint32_t)os->data[offset + 2] << 16)
               | ((uint32_t)os->data[offset + 3] << 24);
    return v;
}

uint32_t host_linker_patch_u64(uint32_t section_index, uint64_t offset, uint64_t value) {
    if (section_index >= g_state.output_section_count) return ERR_INVALID_SECTION;
    OutputSection *os = &g_state.output_sections[section_index];
    if (offset + 8 > os->size) return ERR_INVALID_ADDRESS;
    for (int i = 0; i < 8; i++) {
        os->data[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return ERR_OK;
}

/* ========================================================================
 * TLS synthetic slots (AArch64)
 * ======================================================================== */
uint32_t host_linker_aarch64_tls_synth_reserve(uint32_t obj_index,
                                                  uint32_t sym_index,
                                                  uint32_t reloc_type) {
    if (g_state.tls_synth_count >= g_state.tls_synth_cap) {
        uint32_t nc = g_state.tls_synth_cap ? g_state.tls_synth_cap * 2 : 256;
        TlsSynthSlot *ns = (TlsSynthSlot *)realloc(g_state.tls_synth_slots, nc * sizeof(TlsSynthSlot));
        if (!ns) return ERR_NOT_IMPLEMENTED_YET;
        memset(ns + g_state.tls_synth_cap, 0, (nc - g_state.tls_synth_cap) * sizeof(TlsSynthSlot));
        g_state.tls_synth_slots = ns;
        g_state.tls_synth_cap = nc;
    }

    uint32_t model = 0;
    if (reloc_type >= 512 && reloc_type <= 569) {
        /* TLSGD family */
        if (reloc_type == 515 || reloc_type == 516) model = 1; /* AARCH64_TLSGD */
        else if (reloc_type == 560 || reloc_type == 561 || reloc_type == 562
                 || reloc_type == 563 || reloc_type == 564 || reloc_type == 569)
            model = 3; /* TLSDESC */
        else model = 1;
    } else if (reloc_type >= 517 && reloc_type <= 538) {
        model = 2; /* TLSLD */
    }

    if (model == 0) { model = 1; } /* default to TLSGD */

    TlsSynthSlot *slot = &g_state.tls_synth_slots[g_state.tls_synth_count];
    slot->model = model;
    slot->canonical_name_hash = 0;
    slot->canonical_object_index = obj_index;
    slot->canonical_symbol_index = sym_index;
    slot->slot_index = g_state.tls_synth_count;
    slot->reloc_addend = 0;
    g_state.tls_synth_count++;

    return ERR_OK;
}

uint32_t host_linker_aarch64_tls_synth_count(void) {
    return g_state.tls_synth_count;
}

uint64_t host_linker_aarch64_tls_synth_slot_address(uint32_t slot_index) {
    uint64_t base = g_state.aarch64_tls_synth_base;
    if (base == 0) base = 0x02000000;
    return base + (uint64_t)slot_index * 16;
}

uint64_t host_linker_aarch64_tls_synth_reloc_value(uint32_t obj_index,
                                                      uint32_t sym_index,
                                                      uint32_t reloc_type,
                                                      uint64_t addend,
                                                      uint64_t place_addr) {
    /* Find matching synthetic slot and return its address */
    for (uint32_t i = 0; i < g_state.tls_synth_count; i++) {
        TlsSynthSlot *s = &g_state.tls_synth_slots[i];
        if (s->canonical_object_index == obj_index &&
            s->canonical_symbol_index == sym_index) {
            return host_linker_aarch64_tls_synth_slot_address(i);
        }
    }
    /* Reserve a new one */

    if (g_state.tls_synth_count < MAX_TLS_SLOTS) {
        TlsSynthSlot *slot = &g_state.tls_synth_slots[g_state.tls_synth_count];
        uint32_t model = (g_state.target == TARGET_AARCH64_LINUX
                             || g_state.target == TARGET_AARCH64_WINDOWS
                             || g_state.target == TARGET_AARCH64_MACOS) ? 2 : 1;
        slot->model = model;
        slot->canonical_name_hash = 0;
        slot->canonical_object_index = obj_index;
        slot->canonical_symbol_index = sym_index;
        slot->slot_index = g_state.tls_synth_count;
        slot->reloc_addend = addend;
        g_state.tls_synth_count++;
        return host_linker_aarch64_tls_synth_slot_address(slot->slot_index);
    }
    return 0;
}

uint32_t host_linker_aarch64_tls_data_reloc_value(uint32_t obj_index,
                                                     uint32_t sym_index,
                                                     uint32_t reloc_type,
                                                     uint64_t addend,
                                                     uint64_t place_addr) {
    (void)place_addr;
    g_state.last_error = ERR_OK;
    
    if (obj_index >= g_state.object_count) {
        g_state.last_error = ERR_INVALID_FORMAT;
        return 0;
    }
    
    ObjectRecord *obj = &g_state.objects[obj_index];
    uint64_t sym_val = 0;
    if (sym_index < obj->sym_count) {
        sym_val = obj->symbols[sym_index].value;
    }
    
    /* 1028: R_AARCH64_TLS_DTPMOD */
    if (reloc_type == 1028) {
        return 1;
    }
    /* 1029: R_AARCH64_TLS_DTPREL */
    if (reloc_type == 1029) {
        return (uint32_t)(sym_val + addend);
    }
    /* 1030: R_AARCH64_TLS_TPREL */
    if (reloc_type == 1030) {
        return (uint32_t)(sym_val + addend + 16);
    }
    
    g_state.last_error = ERR_NOT_IMPLEMENTED_YET;
    return 0;
}

/* ========================================================================
 * TLS GOT slots (x86_64)
 * ======================================================================== */
uint64_t host_linker_x86_64_tls_got_reloc_value(uint32_t obj_index,
                                                   uint32_t sym_index,
                                                   uint32_t reloc_type,
                                                   uint64_t addend,
                                                   uint64_t place_addr) {
    (void)obj_index; (void)sym_index; (void)reloc_type;
    (void)addend; (void)place_addr;
    return 0;
}

/* ========================================================================
 * Image size and section management
 * ======================================================================== */
uint32_t host_linker_calculate_image_size(void) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < g_state.output_section_count; i++) {
        total += (uint32_t)g_state.output_sections[i].size;
    }
    if (total == 0 && g_state.object_count > 0) {
        /* Fallback: sum of .text sections from all objects */
        for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
            ObjectRecord *obj = &g_state.objects[oi];
            for (uint32_t si = 0; si < obj->sec_count; si++) {
                if (obj->sections[si].flags & 0x4) { /* SHF_EXECINSTR */
                    total += (uint32_t)obj->sections[si].size;
                }
            }
        }
    }
    return total;
}

uint64_t host_linker_allocate_image(uint32_t size) {
    if (size == 0) return 0;
    g_state.image_buf = (uint8_t *)calloc(size, 1);
    if (!g_state.image_buf) return 0;
    g_state.image_buf_size = size;
    return (uint64_t)(uintptr_t)g_state.image_buf;
}

uint64_t host_linker_create_image_buffer(uint32_t format) {
    (void)format;
    return host_linker_allocate_image(0x400000); /* 4MB default */
}

uint32_t host_linker_add_output_section(uint64_t section) {
    if (g_state.output_section_count >= MAX_OUTPUT_SECS) return ERR_INVALID_SECTION;
    /* Section handle is treated as a string describing the section name */
    const char *name = cstring_from_handle(section);
    (void)name;
    g_state.output_sections[g_state.output_section_count].data = NULL;
    g_state.output_sections[g_state.output_section_count].size = 0;
    g_state.output_sections[g_state.output_section_count].cap = 0;
    g_state.output_section_count++;
    return ERR_OK;
}

uint32_t host_linker_output_section_count(void) {
    return g_state.output_section_count;
}

uint32_t host_linker_emit_output_section(uint64_t output, uint32_t section_index) {
    if (section_index >= g_state.output_section_count) return ERR_OK;
    /* Write section data to output file */
    const char *out = cstring_from_handle(output);
    OutputSection *os = &g_state.output_sections[section_index];
    if (os->data && os->size > 0) {
        FILE *f = fopen(out, "r+b");
        if (f) {
            fwrite(os->data, 1, os->size, f);
            fclose(f);
        }
    }
    return ERR_OK;
}

/* ========================================================================
 * Write ELF headers
 * ======================================================================== */
uint32_t host_linker_write_elf_headers(uint64_t output, uint64_t entry, uint64_t image_base) {
    const char *out = cstring_from_handle(output);
    if (!out) return ERR_WRITE_FAILED;

    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;

    /* Build ELF64 executable header */
    uint16_t e_machine = (g_state.target == TARGET_AARCH64_LINUX ||
                          g_state.target == TARGET_AARCH64_WINDOWS ||
                          g_state.target == TARGET_AARCH64_MACOS) ? 183 : 62;

    uint8_t ehdr[64];
    memset(ehdr, 0, 64);

    /* e_ident */
    ehdr[0] = 0x7f; ehdr[1] = 'E'; ehdr[2] = 'L'; ehdr[3] = 'F';
    ehdr[4] = 2;   /* ELFCLASS64 */
    ehdr[5] = 1;   /* ELFDATA2LSB */
    ehdr[6] = 1;   /* EV_CURRENT */

    /* e_hdr[7..15] = padding */
    ehdr[16] = 2;  ehdr[17] = 0;  /* e_type = ET_EXEC */
    ehdr[18] = e_machine & 0xFF;   /* e_machine */
    ehdr[19] = e_machine >> 8;
    ehdr[20] = 1;  ehdr[21] = 0;  ehdr[22] = 0; ehdr[23] = 0; /* e_version = EV_CURRENT */

    /* e_entry (8 bytes @ offset 24) */
    uint64_t e_entry = entry;
    for (int i = 0; i < 8; i++) ehdr[24 + i] = (uint8_t)((e_entry >> (i * 8)) & 0xFF);

    /* e_phoff = 64 (program header after ELF header) */
    uint64_t e_phoff = 64;
    for (int i = 0; i < 8; i++) ehdr[32 + i] = (uint8_t)((e_phoff >> (i * 8)) & 0xFF);

    /* e_shoff = 0 (we'll fix this later when known) */
    /* e_flags = 0 */
    ehdr[48] = 0;  ehdr[49] = 0;  /* e_ehsize = 64 */
    ehdr[50] = 64; ehdr[51] = 0;  ehdr[52] = 0; ehdr[53] = 0;

    /* e_phentsize = 56 (PT_NULL size) */
    ehdr[54] = 56; ehdr[55] = 0;

    /* e_phnum = 1 (LOAD segment) */
    ehdr[56] = 1;  ehdr[57] = 0;

    /* e_shentsize = 64 */
    ehdr[58] = 64; ehdr[59] = 0;
    /* e_shnum, e_shstrndx = 0 for now */

    fwrite(ehdr, 1, 64, f);

    /* Write a single LOAD program header */
    uint8_t phdr[56];
    memset(phdr, 0, 56);
    phdr[0] = 1;   /* p_type = PT_LOAD */
    phdr[4] = 5;   /* p_flags = PF_R | PF_X */

    /* p_offset = 0 */
    /* p_vaddr = image_base */
    for (int i = 8; i < 16; i++) phdr[i] = (uint8_t)((image_base >> ((i - 8) * 8)) & 0xFF);

    /* p_filesz = host_linker_calculate_image_size() */
    uint32_t imgsz = host_linker_calculate_image_size();
    for (int i = 32; i < 40; i++) phdr[i] = (uint8_t)((imgsz >> ((i - 32) * 8)) & 0xFF);
    for (int i = 40; i < 48; i++) phdr[i] = (uint8_t)((imgsz >> ((i - 40) * 8)) & 0xFF);

    fwrite(phdr, 1, 56, f);
    fclose(f);
    return ERR_OK;
}

/* ========================================================================
 * Finalize ELF
 * ======================================================================== */
uint32_t host_linker_finalize_elf(uint64_t output) {
    (void)output;
    /* For now the ELF header + segments are already written.
       Full finalization would write section headers, program headers,
       dynamic section, etc */
    return ERR_OK;
}

/* ========================================================================
 * Write flat binary
 * ======================================================================== */
uint32_t host_linker_write_flat_binary(uint64_t output) {
    const char *out = cstring_from_handle(output);
    if (!out) return ERR_WRITE_FAILED;

    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;

    /* Write each object's .text sections sequentially */
    uint32_t total = 0;
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sec_count; si++) {
            ObjectSection *sec = &obj->sections[si];
            if (sec->offset > 0 && sec->size > 0 &&
                obj->image_data && (sec->offset + sec->size) <= obj->image_size) {
                fwrite(&obj->image_data[sec->offset], 1, sec->size, f);
                total += (uint32_t)sec->size;
            }
        }
    }
    fclose(f);
    (void)total;
    return ERR_OK;
}

/* ========================================================================
 * Stub output writers (PE, Mach-O, MBR, EFI)
 * ======================================================================== */
uint32_t host_linker_write_pe_image(uint64_t output) {
    const char *out = cstring_from_handle(output);
    if (!out) return ERR_WRITE_FAILED;
    /* Basic PE stub - writes object sections as .text, .data, .rdata */
    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;

    /* Write a minimal PE/COFF header */
    uint8_t buf[256];
    memset(buf, 0, 256);

    /* DOS header (minimal) */
    buf[0] = 'M'; buf[1] = 'Z';

    /* COFF header at offset 60 */
    buf[60 + 0] = 0x64; buf[60 + 1] = 0x86; /* Machine = AMD64 (0x8664 LE) */
    buf[60 + 4] = 1;   /* Number of sections = 1 (text) */

    /* Write header + section data */
    fwrite(buf, 1, 64, f);

    /* Section data */
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sec_count; si++) {
            ObjectSection *sec = &obj->sections[si];
            if (sec->offset > 0 && sec->size > 0 &&
                obj->image_data && (sec->offset + sec->size) <= obj->image_size) {
                fwrite(&obj->image_data[sec->offset], 1, sec->size, f);
            }
        }
    }
    fclose(f);
    return ERR_OK;
}

uint32_t host_linker_write_macho_image(uint64_t output) {
    const char *out = cstring_from_handle(output);
    if (!out) return ERR_WRITE_FAILED;
    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;

    /* Write minimal Mach-O MH_MAGIC_64 header */
    uint32_t hdr[8] = {
        0xfeedfacf, /* MH_MAGIC_64 */
        0x01000007, /* CPU_TYPE_X86_64 */
        0x00000003, /* CPU_SUBTYPE_X86_64_ALL */
        0x00000002, /* MH_EXECUTE */
        0, /* ncmds */
        0, /* sizeofcmds */
        0, /* flags */
        0  /* reserved */
    };
    fwrite(hdr, 1, 32, f);

    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sec_count; si++) {
            ObjectSection *sec = &obj->sections[si];
            if (sec->offset > 0 && sec->size > 0 &&
                obj->image_data && (sec->offset + sec->size) <= obj->image_size) {
                fwrite(&obj->image_data[sec->offset], 1, sec->size, f);
            }
        }
    }
    fclose(f);
    return ERR_OK;
}

uint32_t host_linker_write_mbr_image(uint64_t output) {
    const char *out = cstring_from_handle(output);
    if (!out) return ERR_WRITE_FAILED;
    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;

    /* Write boot sector padding (512 bytes), then kernel data */
    uint8_t padding[512];
    memset(padding, 0, 512);
    fwrite(padding, 1, 512, f);

    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sec_count; si++) {
            ObjectSection *sec = &obj->sections[si];
            if (sec->offset > 0 && sec->size > 0 &&
                obj->image_data && (sec->offset + sec->size) <= obj->image_size) {
                fwrite(&obj->image_data[sec->offset], 1, sec->size, f);
            }
        }
    }
    fclose(f);
    return ERR_OK;
}

uint32_t host_linker_write_mbr_boot_image(uint64_t output, uint64_t kernel,
                                             uint32_t kernel_size) {
    const char *out = cstring_from_handle(output);
    const char *kern = cstring_from_handle(kernel);
    if (!out || !kern) return ERR_WRITE_FAILED;
    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;
    uint8_t padding[512];
    memset(padding, 0, 512);
    fwrite(padding, 1, 512, f);
    FILE *kf = fopen(kern, "rb");
    if (kf) {
        /* Read and write kernel data */
        uint8_t buf[4096];
        size_t rd;
        while ((rd = fread(buf, 1, sizeof(buf), kf)) > 0) {
            fwrite(buf, 1, rd, f);
        }
        fclose(kf);
    }
    fclose(f);
    return ERR_OK;
}

uint32_t host_linker_kernel_size(uint64_t kernel) {
    const char *k = cstring_from_handle(kernel);
    if (!k) return 0;
    return (uint32_t)file_size_by_path(k);
}

uint32_t host_linker_write_efi_image(uint64_t output) {
    const char *out = cstring_from_handle(output);
    if (!out) return ERR_WRITE_FAILED;
    FILE *f = fopen(out, "wb");
    if (!f) return ERR_WRITE_FAILED;

    /* Write minimal EFI/PE header */
    uint8_t dos_hdr[64] = { 'M', 'Z', 0 };
    /* COFF header offset at 60 */
    dos_hdr[60] = 0x40; /* Point to PE header immediately after */
    fwrite(dos_hdr, 1, 64, f);

    uint8_t pe_hdr[24] = {
        'P', 'E', 0, 0,
        0x64, 0x86, /* AMD64 */
        0, 0, /* n sections */
        0, 0, 0, 0, /* timestamp */
        0, 0, 0, 0, /* ptr sym */
        0, 0, 0, 0, /* num symbols */
        0xf0, 0, /* size of opt header */
        0x02, 0x20 /* characteristics target */
    };
    fwrite(pe_hdr, 1, 24, f);

    /* minimal optional header indicating Subsystem 10 (EFI application) */
    uint8_t opt_hdr[240] = { 0 };
    opt_hdr[0] = 0x0b; opt_hdr[1] = 0x02; /* PE32+ */
    opt_hdr[68] = 10; /* Subsystem: EFI_APPLICATION (10) */
    fwrite(opt_hdr, 1, 240, f);

    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        for (uint32_t si = 0; si < obj->sec_count; si++) {
            ObjectSection *sec = &obj->sections[si];
            if (sec->offset > 0 && sec->size > 0 &&
                obj->image_data && (sec->offset + sec->size) <= obj->image_size) {
                fwrite(&obj->image_data[sec->offset], 1, sec->size, f);
            }
        }
    }
    fclose(f);
    return ERR_OK;
}

/* ========================================================================
 * Image finalization
 * ======================================================================== */
uint32_t host_linker_finalize_image(void) {
    return ERR_OK;
}

/* ========================================================================
 * Map file helpers
 * ======================================================================== */
uint64_t host_linker_format_map_row(uint32_t index) {
    if (index >= g_state.map_row_count) return 0;
    uint64_t h = 0;
    string_store(&h, g_state.map_rows[index]);
    return h;
}

uint32_t host_linker_map_row_count(void) {
    return g_state.map_row_count;
}

/* ========================================================================
 * Linker script application
 * ======================================================================== */
uint32_t host_linker_apply_script(uint64_t script) {
    const char *path = cstring_from_handle(script);
    if (!path) return ERR_FILE_NOT_FOUND;
    /* Script parsing is done Dust-side; the dust linker
       calls this after Dust-side parsing. For now we validate
       the script file exists. */
    return path_exists(path) ? ERR_OK : ERR_FILE_NOT_FOUND;
}

/* ========================================================================
 * Dump/debug helpers
 * ======================================================================== */
uint32_t host_linker_dump_sections(void) {
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        printf("Object %u (%u sections):\n", oi, obj->sec_count);
        for (uint32_t si = 0; si < obj->sec_count; si++) {
            ObjectSection *sec = &obj->sections[si];
            printf("  Section %u: type=%u flags=0x%lx size=%lu align=%lu\n",
                   si, sec->sec_type, (unsigned long)sec->flags,
                   (unsigned long)sec->size, (unsigned long)sec->align);
        }
    }
    return ERR_OK;
}

uint32_t host_linker_dump_symbols(void) {
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        printf("Object %u (%u symbols):\n", oi, obj->sym_count);
        for (uint32_t si = 0; si < obj->sym_count; si++) {
            ObjectSymbol *sym = &obj->symbols[si];
            printf("  Symbol %u: hash=0x%lx bind=%u type=%u shndx=%u val=0x%lx\n",
                   si, (unsigned long)sym->name_hash, sym->bind, sym->sym_type,
                   sym->shndx, (unsigned long)sym->value);
        }
    }
    return ERR_OK;
}

uint32_t host_linker_dump_relocations(void) {
    for (uint32_t oi = 0; oi < g_state.object_count; oi++) {
        ObjectRecord *obj = &g_state.objects[oi];
        printf("Object %u (%u relocations):\n", oi, obj->reloc_count);
        for (uint32_t ri = 0; ri < obj->reloc_count; ri++) {
            ObjectRelocation *r = &obj->relocs[ri];
            printf("  Reloc %u: section=%u offset=0x%lx type=%u symbol=%u addend=0x%lx\n",
                   ri, r->section, (unsigned long)r->offset, r->reloc_type,
                   r->symbol, (unsigned long)r->addend);
        }
    }
    return ERR_OK;
}

/* ========================================================================
 * Dependency file
 * ======================================================================== */
uint32_t host_linker_write_dependency_file(void) {
    if (g_state.dependency_file == 0) return ERR_OK;
    const char *path = cstring_from_handle(g_state.dependency_file);
    if (!path) return ERR_OK;
    FILE *f = fopen(path, "w");
    if (!f) return ERR_WRITE_FAILED;
    /* Write basic depfile with output and inputs */
    uint64_t out = g_state.output_path;
    const char *out_path = cstring_from_handle(out);
    if (out_path) fprintf(f, "%s:", out_path);
    for (uint32_t i = 0; i < g_state.inputs.count; i++) {
        const char *inp = cstring_from_handle(g_state.inputs.items[i]);
        if (inp) fprintf(f, " \\\n  %s", inp);
    }
    fprintf(f, "\n");
    fclose(f);
    return ERR_OK;
}

/* ========================================================================
 * Emit unresolved warning
 * ======================================================================== */
uint32_t host_linker_emit_unresolved_warning(uint32_t unresolved, uint32_t weak_fallbacks) {
    (void)weak_fallbacks;
    printf("warning: %u unresolved symbol(s)\n", unresolved);
    return ERR_OK;
}

/* ========================================================================
 * Versioned shared library finding
 * ======================================================================== */
uint64_t host_linker_find_versioned_shared_in_path(uint64_t path_h, uint64_t lib_name) {
    const char *dir = cstring_from_handle(path_h);
    const char *name = cstring_from_handle(lib_name);
    if (!dir || !name) return 0;

    /* Scan directory for versioned shared libraries matching lib_name */
    /* Simple implementation: try common versioned library names */
    char buf[1024];
    /* Try dir/libname.so, dir/libname.so.1, etc */
    snprintf(buf, sizeof(buf), "%s/lib%s.so", dir, name);
    if (path_exists(buf)) {
        uint64_t h = 0;
        string_store(&h, buf);
        return h;
    }
    /* Try dir/name (Windows .dll) */
    snprintf(buf, sizeof(buf), "%s/%s.dll", dir, name);
    if (path_exists(buf)) {
        uint64_t h = 0;
        string_store(&h, buf);
        return h;
    }
    /* Try dir/libname.dylib (macOS) */
    snprintf(buf, sizeof(buf), "%s/lib%s.dylib", dir, name);
    if (path_exists(buf)) {
        uint64_t h = 0;
        string_store(&h, buf);
        return h;
    }
    return 0;
}

/* ========================================================================
 * Last error tracking
 * ======================================================================== */
uint64_t host_linker_last_error(void) { return g_state.last_error; }

/* ========================================================================
 * Archive member extraction
 * ======================================================================== */
uint32_t host_archive_member_count(uint64_t path) {
    const char *p = cstring_from_handle(path);
    if (!p) return 0;
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "!<arch>\n", 8) != 0) {
        fclose(f);
        return 0;
    }
    
    uint32_t count = 0;
    char header[60];
    while (fread(header, 1, 60, f) == 60) {
        if (header[58] != '`' || header[59] != '\n') break;
        char size_str[11];
        memcpy(size_str, header + 48, 10);
        size_str[10] = '\0';
        uint32_t size = (uint32_t)strtoul(size_str, NULL, 10);
        count++;
        fseek(f, size + (size & 1), SEEK_CUR);
    }
    fclose(f);
    return count;
}

uint64_t host_archive_extract_member_object(uint64_t path, uint32_t index) {
    (void)path; (void)index;
    return 0; /* Archive extraction done Dust-side via read_u8_file */
}

uint32_t host_archive_member_is_elf(uint64_t path, uint32_t index) {
    (void)path; (void)index;
    return 0;
}

uint32_t host_archive_member_object_kind(uint64_t path, uint32_t index) {
    (void)path; (void)index;
    return OBJECT_FORMAT_UNKNOWN;
}

uint32_t host_archive_member_matches_unresolved(uint64_t path, uint32_t index) {
    (void)path; (void)index;
    return 0;
}

uint32_t host_archive_member_is_loaded(uint64_t path, uint32_t index) {
    (void)path; (void)index;
    return 0;
}

uint32_t host_archive_member_mark_loaded(uint64_t path, uint32_t index) {
    (void)path; (void)index;
    return ERR_OK;
}

/* ========================================================================
 * Initialization
 * ======================================================================== */
__attribute__((constructor))
void host_linker_init(void) {
    reset_linker_state();
    init_args();

    /* Capture real argv if available via /proc/self or similar */
    /* For dust build targets, the dust compiler passes its own
       argc/argv; here we initialize defaults */
}

