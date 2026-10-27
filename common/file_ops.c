#include "file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <io.h>
    #define fsync_fd(fd) _commit(fd)
#else
    #include <unistd.h>
    #define fsync_fd(fd) fsync(fd)
#endif

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

/* Write header + records to a file path. Flushes and fsyncs. */
static bool write_file(const char *path, const char *magic,
                       const void *records, size_t record_size,
                       uint32_t count, uint32_t next_id) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open '%s' for writing: ", path);
        perror("");
        return false;
    }

    FileHeader hdr;
    memcpy(hdr.magic, magic, FILE_MAGIC_LEN);
    hdr.version      = FILE_VERSION;
    hdr.record_count = count;
    hdr.next_id      = next_id;

    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to write header to '%s'\n", path);
        fclose(fp);
        return false;
    }

    if (count > 0 && records != NULL) {
        if (fwrite(records, record_size, count, fp) != count) {
            fprintf(stderr, "Error: Failed to write %u records to '%s'\n",
                    count, path);
            fclose(fp);
            return false;
        }
    }

    if (fflush(fp) != 0) {
        fprintf(stderr, "Error: fflush failed for '%s'\n", path);
        fclose(fp);
        return false;
    }

    /* Best-effort fsync — not fatal if it fails (e.g., on some FSes) */
    int fd = fileno(fp);
    if (fd >= 0) {
        (void)fsync_fd(fd);
    }

    fclose(fp);
    return true;
}

/* Byte-for-byte file copy */
static bool copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return false;

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    char buf[4096];
    size_t n;
    bool ok = true;

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = false;
            break;
        }
    }

    if (ferror(in)) ok = false;

    fclose(in);
    fclose(out);
    return ok;
}

/* Build a path with a suffix (e.g., ".tmp", ".bak") */
static char *make_suffixed_path(const char *path, const char *suffix) {
    size_t plen = strlen(path);
    size_t slen = strlen(suffix);
    char *result = malloc(plen + slen + 1);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    memcpy(result, path, plen);
    memcpy(result + plen, suffix, slen + 1);
    return result;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

bool file_atomic_save(const char *path, const char *magic,
                      const void *records, size_t record_size,
                      uint32_t count, uint32_t next_id) {
    char *tmp_path = make_suffixed_path(path, ".tmp");
    char *bak_path = make_suffixed_path(path, ".bak");
    if (!tmp_path || !bak_path) {
        free(tmp_path);
        free(bak_path);
        return false;
    }

    /* Step 1: Write to temporary file (includes flush + fsync) */
    if (!write_file(tmp_path, magic, records, record_size, count, next_id)) {
        free(tmp_path);
        free(bak_path);
        return false;
    }

    /* Step 2: Backup current file if it exists */
    FILE *existing = fopen(path, "rb");
    if (existing) {
        fclose(existing);
        /* Best-effort backup — not fatal if it fails */
        (void)copy_file(path, bak_path);
    }

    /* Step 3: Atomic rename (POSIX guarantees atomicity) */
    if (rename(tmp_path, path) != 0) {
        fprintf(stderr, "Error: Failed to rename '%s' -> '%s': ", tmp_path, path);
        perror("");
        free(tmp_path);
        free(bak_path);
        return false;
    }

    free(tmp_path);
    free(bak_path);
    return true;
}

int file_validated_load(const char *path, const char *magic,
                        void **records, size_t record_size,
                        uint32_t *count, uint32_t *next_id) {
    *records = NULL;
    *count   = 0;
    *next_id = 1;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        /* File doesn't exist — caller creates fresh data */
        return FILE_LOAD_FRESH;
    }

    /* Read header */
    FileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        fprintf(stderr, "Error: '%s' is too small to contain a valid header.\n", path);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    /* Validate magic */
    if (memcmp(hdr.magic, magic, FILE_MAGIC_LEN) != 0) {
        fprintf(stderr, "Error: '%s' has invalid magic number "
                "(expected '%.4s', got '%.4s').\n",
                path, magic, hdr.magic);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    /* Validate version */
    if (hdr.version != FILE_VERSION) {
        fprintf(stderr, "Error: '%s' has unsupported version %u (expected %u).\n",
                path, hdr.version, FILE_VERSION);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    /* Sanity-check record count */
    if (hdr.record_count > FILE_MAX_RECORDS) {
        fprintf(stderr, "Error: '%s' claims %u records (max %d). Likely corrupted.\n",
                path, hdr.record_count, FILE_MAX_RECORDS);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    /* Validate file size matches header + data */
    long data_start = ftell(fp);
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: Cannot seek in '%s'.\n", path);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }
    long file_size = ftell(fp);
    long expected  = data_start + (long)((size_t)hdr.record_count * record_size);

    if (file_size != expected) {
        fprintf(stderr, "Error: '%s' size mismatch — expected %ld bytes, got %ld. "
                "File may be truncated or corrupted.\n",
                path, expected, file_size);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    /* Seek back to data */
    if (fseek(fp, data_start, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Cannot seek in '%s'.\n", path);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    /* Handle empty file (valid, zero records) */
    if (hdr.record_count == 0) {
        *count   = 0;
        *next_id = hdr.next_id;
        fclose(fp);
        return FILE_LOAD_OK;
    }

    /* Allocate and read records */
    void *data = malloc((size_t)hdr.record_count * record_size);
    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed for %u records.\n",
                hdr.record_count);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    if (fread(data, record_size, hdr.record_count, fp) != hdr.record_count) {
        fprintf(stderr, "Error: Failed to read records from '%s'.\n", path);
        free(data);
        fclose(fp);
        return FILE_LOAD_CORRUPT;
    }

    *records = data;
    *count   = hdr.record_count;
    *next_id = hdr.next_id;

    fclose(fp);
    return FILE_LOAD_OK;
}

bool file_recover_backup(const char *path) {
    char *bak_path = make_suffixed_path(path, ".bak");
    if (!bak_path) return false;

    FILE *fp = fopen(bak_path, "rb");
    if (!fp) {
        fprintf(stderr, "No backup file found at '%s'.\n", bak_path);
        free(bak_path);
        return false;
    }
    fclose(fp);

    if (!copy_file(bak_path, path)) {
        fprintf(stderr, "Error: Failed to restore backup from '%s'.\n", bak_path);
        free(bak_path);
        return false;
    }

    printf("Successfully restored data from backup '%s'.\n", bak_path);
    free(bak_path);
    return true;
}
