/* hostlinker_shim.h - DustLink Host Runtime Shim Header
 *
 * C FFI declarations for DustLink host_* functions.
 * These match the Dust procedures in shim_host_v02.ds
 */

#ifndef HOSTLINKER_SHIM_H
#define HOSTLINKER_SHIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CLI Arguments
 * ======================================================================== */
uint32_t host_args_count(void);
uint64_t host_arg_at(uint32_t index);
void     host_args_get(uint64_t argv_ptr, uint64_t argv_buf_ptr);
void     host_args_sizes_get(uint64_t argc_out, uint64_t argv_buf_size_out);

/* ========================================================================
 * Environment Variables
 * ======================================================================== */
uint64_t host_env_get(uint64_t name);
void     host_env_set(uint64_t name, uint64_t value);

/* ========================================================================
 * Path Operations
 * ======================================================================== */
uint32_t host_path_exists(uint64_t path);
uint32_t host_path_is_file(uint64_t path);
uint64_t host_path_join(uint64_t base, uint64_t leaf);

/* ========================================================================
 * File Operations (POSIX)
 * ======================================================================== */
int64_t  host_file_open(uint64_t pathname, uint32_t flags, uint32_t mode);
int64_t  host_file_close(uint32_t fd);
int64_t  host_file_read(uint32_t fd, uint64_t buf, uint64_t count);
int64_t  host_file_write(uint32_t fd, uint64_t buf, uint64_t count);
int64_t  host_file_seek(uint32_t fd, int64_t offset, uint32_t whence);
int64_t  host_file_tell(uint32_t fd);

/* ========================================================================
 * Memory Mapping
 * ======================================================================== */
int64_t  host_mmap(uint64_t addr, uint64_t length, uint32_t prot, uint32_t flags, int64_t fd, int64_t offset);
int32_t  host_munmap(uint64_t addr, uint64_t length);

/* ========================================================================
 * Logging
 * ======================================================================== */
void     host_write_stdout(uint64_t s);
void     host_write_stderr(uint64_t s);

/* ========================================================================
 * Memory Allocation
 * ======================================================================== */
uint64_t host_alloc(uint64_t size);
void     host_free(uint64_t ptr);

/* ========================================================================
 * String Operations
 * ======================================================================== */
uint64_t host_str_concat(uint64_t lhs, uint64_t rhs);
uint32_t host_str_eq(uint64_t lhs, uint64_t rhs);
uint64_t host_str_hash64(uint64_t text);

/* ========================================================================
 * Filesystem Helpers
 * ======================================================================== */
uint64_t host_fs_file_size(uint64_t path);
uint8_t  host_fs_read_u8(uint64_t path, uint64_t offset);
uint32_t host_fs_write_u8(uint64_t path, uint64_t offset, uint8_t value);
uint32_t host_fs_truncate(uint64_t path, uint64_t size);
uint32_t host_fs_append_line(uint64_t path, uint64_t line);

/* ========================================================================
 * Linker State (Layer 1)
 * ======================================================================== */
uint32_t host_linker_set_output_path(uint64_t path);
uint32_t host_linker_set_output_format(uint32_t format);
uint32_t host_linker_set_target(uint32_t target);
uint32_t host_linker_set_entry(uint64_t entry);
uint32_t host_linker_set_base_address(uint64_t base);
uint64_t host_linker_last_error(void);
uint32_t host_linker_reset_state(void);

/* ========================================================================
 * Archive Operations
 * ======================================================================== */
uint32_t host_archive_validate_magic(uint64_t path);

#ifdef __cplusplus
}
#endif

#endif /* HOSTLINKER_SHIM_H */