#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "repository.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define CREATE_DIRECTORY(path) _mkdir(path)
#define SYNC_FILE(file) _commit(_fileno(file))
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define CREATE_DIRECTORY(path) mkdir(path, 0755)
#define SYNC_FILE(file) fsync(fileno(file))
#endif

#define LINE_CAPACITY 2048
#define FIELD_COUNT 8

static void set_error(char *error, size_t error_size, const char *message) {
    if (error != NULL && error_size > 0) {
        snprintf(error, error_size, "%s", message == NULL ? "unknown error" : message);
    }
}

static int ensure_capacity(InstrumentRepository *repository, size_t required) {
    Instrument *resized;
    size_t capacity;
    if (required <= repository->capacity) {
        return 1;
    }
    capacity = repository->capacity == 0 ? 8 : repository->capacity * 2;
    while (capacity < required) {
        capacity *= 2;
    }
    resized = (Instrument *)realloc(repository->items, capacity * sizeof(Instrument));
    if (resized == NULL) {
        return 0;
    }
    repository->items = resized;
    repository->capacity = capacity;
    return 1;
}

void repository_init(InstrumentRepository *repository) {
    if (repository != NULL) {
        repository->items = NULL;
        repository->size = 0;
        repository->capacity = 0;
    }
}

void repository_free(InstrumentRepository *repository) {
    if (repository != NULL) {
        free(repository->items);
        repository_init(repository);
    }
}

const Instrument *repository_find_by_id(const InstrumentRepository *repository, int id) {
    size_t index;
    if (repository == NULL) {
        return NULL;
    }
    for (index = 0; index < repository->size; ++index) {
        if (repository->items[index].id == id) {
            return &repository->items[index];
        }
    }
    return NULL;
}

Instrument *repository_find_mutable_by_id(InstrumentRepository *repository, int id) {
    return (Instrument *)repository_find_by_id(repository, id);
}

int repository_next_id(const InstrumentRepository *repository) {
    int maximum = 0;
    size_t index;
    if (repository == NULL) {
        return 1;
    }
    for (index = 0; index < repository->size; ++index) {
        if (repository->items[index].id > maximum) {
            maximum = repository->items[index].id;
        }
    }
    return maximum + 1;
}

int repository_add(InstrumentRepository *repository, Instrument instrument,
                   char *error, size_t error_size) {
    if (repository == NULL) {
        set_error(error, error_size, "repository is required");
        return 0;
    }
    if (!instrument_validate(&instrument, error, error_size)) {
        return 0;
    }
    if (repository_find_by_id(repository, instrument.id) != NULL) {
        set_error(error, error_size, "instrument id already exists");
        return 0;
    }
    if (!ensure_capacity(repository, repository->size + 1)) {
        set_error(error, error_size, "out of memory");
        return 0;
    }
    repository->items[repository->size++] = instrument;
    return 1;
}

int repository_update(InstrumentRepository *repository, Instrument instrument,
                      char *error, size_t error_size) {
    Instrument *existing;
    if (!instrument_validate(&instrument, error, error_size)) {
        return 0;
    }
    existing = repository_find_mutable_by_id(repository, instrument.id);
    if (existing == NULL) {
        set_error(error, error_size, "instrument not found");
        return 0;
    }
    *existing = instrument;
    return 1;
}

int repository_remove(InstrumentRepository *repository, int id,
                      char *error, size_t error_size) {
    size_t index;
    if (repository == NULL || id <= 0) {
        set_error(error, error_size, "valid id is required");
        return 0;
    }
    for (index = 0; index < repository->size; ++index) {
        if (repository->items[index].id == id) {
            if (index + 1 < repository->size) {
                memmove(&repository->items[index], &repository->items[index + 1],
                        (repository->size - index - 1) * sizeof(Instrument));
            }
            --repository->size;
            return 1;
        }
    }
    set_error(error, error_size, "instrument not found");
    return 0;
}

static int parse_int_exact(const char *text, int *value) {
    char *end;
    long parsed;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || *text == '\0' || *end != '\0' || parsed < -2147483647L - 1L
        || parsed > 2147483647L) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int parse_double_exact(const char *text, double *value) {
    char *end;
    errno = 0;
    *value = strtod(text, &end);
    return errno == 0 && *text != '\0' && *end == '\0';
}

static int decode_fields(const char *line, char fields[FIELD_COUNT][INSTRUMENT_TEXT_CAPACITY]) {
    size_t field = 0;
    size_t position = 0;
    int escaped = 0;
    const unsigned char *cursor = (const unsigned char *)line;
    memset(fields, 0, FIELD_COUNT * INSTRUMENT_TEXT_CAPACITY);
    while (*cursor != '\0' && *cursor != '\n' && *cursor != '\r') {
        unsigned char character = *cursor++;
        if (escaped) {
            if (character == 't') {
                character = '\t';
            } else if (character == 'n') {
                character = '\n';
            } else if (character == 'r') {
                character = '\r';
            }
            escaped = 0;
        } else if (character == '\\') {
            escaped = 1;
            continue;
        } else if (character == '\t') {
            if (++field >= FIELD_COUNT) {
                return 0;
            }
            position = 0;
            continue;
        }
        if (position + 1 >= INSTRUMENT_TEXT_CAPACITY) {
            return 0;
        }
        fields[field][position++] = (char)character;
    }
    if (escaped) {
        if (position + 1 >= INSTRUMENT_TEXT_CAPACITY) {
            return 0;
        }
        fields[field][position++] = '\\';
    }
    return field == FIELD_COUNT - 1;
}

static int decode_instrument(const char *line, Instrument *instrument) {
    char fields[FIELD_COUNT][INSTRUMENT_TEXT_CAPACITY];
    if (!decode_fields(line, fields)
        || !parse_int_exact(fields[0], &instrument->id)
        || !parse_double_exact(fields[6], &instrument->price)
        || !parse_int_exact(fields[7], &instrument->quantity)) {
        return 0;
    }
    snprintf(instrument->name, sizeof(instrument->name), "%s", fields[1]);
    snprintf(instrument->category, sizeof(instrument->category), "%s", fields[2]);
    snprintf(instrument->specification, sizeof(instrument->specification), "%s", fields[3]);
    snprintf(instrument->model, sizeof(instrument->model), "%s", fields[4]);
    snprintf(instrument->purchase_date, sizeof(instrument->purchase_date), "%s", fields[5]);
    return 1;
}

static int file_exists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    fclose(file);
    return 1;
}

int repository_load(InstrumentRepository *repository, const char *path,
                    char *error, size_t error_size) {
    FILE *file;
    char line[LINE_CAPACITY];
    char backup[LINE_CAPACITY];
    unsigned long line_number = 0;
    if (repository == NULL || path == NULL || *path == '\0') {
        set_error(error, error_size, "repository and path are required");
        return 0;
    }
    repository_free(repository);
    snprintf(backup, sizeof(backup), "%s.bak", path);
    file = fopen(path, "rb");
    if (file == NULL && file_exists(backup)) {
        rename(backup, path);
        file = fopen(path, "rb");
    }
    if (file == NULL) {
        if (errno == ENOENT) {
            return 1;
        }
        set_error(error, error_size, "cannot open data file");
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        Instrument instrument;
        ++line_number;
        if (line_number == 1 && strncmp(line, "id\t", 3) == 0) {
            continue;
        }
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
            continue;
        }
        if (!strchr(line, '\n') && !feof(file)) {
            char detail[128];
            snprintf(detail, sizeof(detail), "line %lu is too long", line_number);
            set_error(error, error_size, detail);
            fclose(file);
            repository_free(repository);
            return 0;
        }
        if (!decode_instrument(line, &instrument)
            || !instrument_validate(&instrument, error, error_size)
            || !repository_add(repository, instrument, error, error_size)) {
            char detail[256];
            snprintf(detail, sizeof(detail), "invalid data at line %lu: %s", line_number,
                     error != NULL && *error != '\0' ? error : "malformed record");
            set_error(error, error_size, detail);
            fclose(file);
            repository_free(repository);
            return 0;
        }
    }
    if (ferror(file)) {
        set_error(error, error_size, "error while reading data file");
        fclose(file);
        repository_free(repository);
        return 0;
    }
    fclose(file);
    return 1;
}

static int write_escaped(FILE *file, const char *text) {
    while (*text != '\0') {
        if (*text == '\\') {
            if (fputs("\\\\", file) == EOF) return 0;
        } else if (*text == '\t') {
            if (fputs("\\t", file) == EOF) return 0;
        } else if (*text == '\n') {
            if (fputs("\\n", file) == EOF) return 0;
        } else if (*text == '\r') {
            if (fputs("\\r", file) == EOF) return 0;
        } else if (fputc((unsigned char)*text, file) == EOF) {
            return 0;
        }
        ++text;
    }
    return 1;
}

static int write_instrument(FILE *file, const Instrument *instrument) {
    if (fprintf(file, "%d\t", instrument->id) < 0 || !write_escaped(file, instrument->name)
        || fputc('\t', file) == EOF || !write_escaped(file, instrument->category)
        || fputc('\t', file) == EOF || !write_escaped(file, instrument->specification)
        || fputc('\t', file) == EOF || !write_escaped(file, instrument->model)
        || fputc('\t', file) == EOF || !write_escaped(file, instrument->purchase_date)
        || fprintf(file, "\t%.2f\t%d\n", instrument->price, instrument->quantity) < 0) {
        return 0;
    }
    return 1;
}

static int ensure_parent_directories(const char *path) {
    char copy[LINE_CAPACITY];
    size_t index;
    if (strlen(path) >= sizeof(copy)) {
        return 0;
    }
    snprintf(copy, sizeof(copy), "%s", path);
    for (index = 1; copy[index] != '\0'; ++index) {
        if (copy[index] == '/' || copy[index] == '\\') {
            char saved = copy[index];
            if (index == 2 && copy[1] == ':') {
                continue;
            }
            copy[index] = '\0';
            if (*copy != '\0' && CREATE_DIRECTORY(copy) != 0 && errno != EEXIST) {
                return 0;
            }
            copy[index] = saved;
        }
    }
    return 1;
}

int repository_save(const InstrumentRepository *repository, const char *path,
                    char *error, size_t error_size) {
    FILE *file;
    char temporary[LINE_CAPACITY];
    char backup[LINE_CAPACITY];
    size_t index;
    int had_original;
    int flush_succeeded;
    if (repository == NULL || path == NULL || *path == '\0') {
        set_error(error, error_size, "repository and path are required");
        return 0;
    }
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary)
        || snprintf(backup, sizeof(backup), "%s.bak", path) >= (int)sizeof(backup)) {
        set_error(error, error_size, "data path is too long");
        return 0;
    }
    if (!ensure_parent_directories(path)) {
        set_error(error, error_size, "cannot create data directory");
        return 0;
    }
    remove(temporary);
    file = fopen(temporary, "wb");
    if (file == NULL) {
        set_error(error, error_size, "cannot create temporary data file");
        return 0;
    }
    if (fputs("id\tname\tcategory\tspecification\tmodel\tpurchase_date\tprice\tquantity\n", file) == EOF) {
        set_error(error, error_size, "cannot write data header");
        fclose(file);
        remove(temporary);
        return 0;
    }
    for (index = 0; index < repository->size; ++index) {
        if (!write_instrument(file, &repository->items[index])) {
            set_error(error, error_size, "cannot write instrument data");
            fclose(file);
            remove(temporary);
            return 0;
        }
    }
    flush_succeeded = fflush(file) == 0;
    if (flush_succeeded) {
        flush_succeeded = SYNC_FILE(file) == 0;
    }
    if (fclose(file) != 0) {
        flush_succeeded = 0;
    }
    if (!flush_succeeded) {
        set_error(error, error_size, "cannot flush instrument data");
        remove(temporary);
        return 0;
    }
    had_original = file_exists(path);
    remove(backup);
    if (had_original && rename(path, backup) != 0) {
        set_error(error, error_size, "cannot create data backup");
        remove(temporary);
        return 0;
    }
    if (rename(temporary, path) != 0) {
        if (had_original) {
            rename(backup, path);
        }
        set_error(error, error_size, "cannot replace data file");
        remove(temporary);
        return 0;
    }
    remove(backup);
    return 1;
}
