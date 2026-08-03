#ifndef LAB_MANAGER_REPOSITORY_H
#define LAB_MANAGER_REPOSITORY_H

#include "instrument.h"

#include <stddef.h>

typedef struct {
    Instrument *items;
    size_t size;
    size_t capacity;
} InstrumentRepository;

void repository_init(InstrumentRepository *repository);
void repository_free(InstrumentRepository *repository);

int repository_load(InstrumentRepository *repository, const char *path,
                    char *error, size_t error_size);
int repository_save(const InstrumentRepository *repository, const char *path,
                    char *error, size_t error_size);

const Instrument *repository_find_by_id(const InstrumentRepository *repository, int id);
Instrument *repository_find_mutable_by_id(InstrumentRepository *repository, int id);
int repository_add(InstrumentRepository *repository, Instrument instrument,
                   char *error, size_t error_size);
int repository_update(InstrumentRepository *repository, Instrument instrument,
                      char *error, size_t error_size);
int repository_remove(InstrumentRepository *repository, int id,
                      char *error, size_t error_size);
int repository_next_id(const InstrumentRepository *repository);

#endif
