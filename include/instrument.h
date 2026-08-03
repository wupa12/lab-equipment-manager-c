#ifndef LAB_MANAGER_INSTRUMENT_H
#define LAB_MANAGER_INSTRUMENT_H

#include <stddef.h>

#define INSTRUMENT_TEXT_CAPACITY 128

typedef struct {
    int id;
    char name[INSTRUMENT_TEXT_CAPACITY];
    char category[INSTRUMENT_TEXT_CAPACITY];
    char specification[INSTRUMENT_TEXT_CAPACITY];
    char model[INSTRUMENT_TEXT_CAPACITY];
    char purchase_date[INSTRUMENT_TEXT_CAPACITY];
    double price;
    int quantity;
} Instrument;

int instrument_validate(const Instrument *instrument, char *error, size_t error_size);
int instrument_contains(const Instrument *instrument, const char *keyword);

#endif
