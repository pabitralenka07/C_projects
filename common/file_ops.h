#ifndef COMMON_FILE_OPS_H
#define COMMON_FILE_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Binary data-file operations with integrity guarantees:
 * - Magic/version header validation
 * - Atomic save (tmp → flush → fsync → backup → rename)
 * - Backup recovery on corruption
 */

#define FILE_MAGIC_LEN    4
#define FILE_VERSION      1
#define FILE_MAX_RECORDS  100000

/* File header — packed to ensure consistent on-disk layout */
#pragma pack(push, 1)
typedef struct {
    char     magic[FILE_MAGIC_LEN];
    uint16_t version;
    uint32_t record_count;
    uint32_t next_id;
} FileHeader;
#pragma pack(pop)

/* Return codes for file_validated_load */
#define FILE_LOAD_OK       0   /* Loaded successfully */
#define FILE_LOAD_FRESH    1   /* File missing — caller should start fresh */
#define FILE_LOAD_CORRUPT (-1) /* Corrupted — backup recovery offered */

/* Atomically save records to file:
 * 1. Write header + records to path.tmp
 * 2. fflush + fsync (where available)
 * 3. Copy current file to path.bak
 * 4. rename tmp → original (atomic on POSIX)
 * Returns true on success. */
bool file_atomic_save(const char *path, const char *magic,
                      const void *records, size_t record_size,
                      uint32_t count, uint32_t next_id);

/* Load and validate a data file.
 * Checks: magic, version, record count sanity, file size match.
 * On success (FILE_LOAD_OK): allocates *records (caller must free),
 *   sets *count and *next_id.
 * On fresh (FILE_LOAD_FRESH): sets *records=NULL, *count=0, *next_id=1.
 * On corrupt (FILE_LOAD_CORRUPT): reports error. */
int file_validated_load(const char *path, const char *magic,
                        void **records, size_t record_size,
                        uint32_t *count, uint32_t *next_id);

/* Attempt to recover from the .bak file by copying it over the main file.
 * Returns true if backup was successfully restored. */
bool file_recover_backup(const char *path);

#endif /* COMMON_FILE_OPS_H */
