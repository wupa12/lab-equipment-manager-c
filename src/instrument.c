#include "instrument.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int is_blank(const char *text) {
    if (text == NULL || *text == '\0') {
        return 1;
    }
    while (*text != '\0') {
        if (!isspace((unsigned char)*text)) {
            return 0;
        }
        ++text;
    }
    return 1;
}

static int valid_date(const char *date) {
    int year;
    int month;
    int day;
    char extra;
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') {
        return 0;
    }
    if (sscanf(date, "%4d-%2d-%2d%c", &year, &month, &day, &extra) != 3) {
        return 0;
    }
    return year >= 1900 && year <= 9999 && month >= 1 && month <= 12
           && day >= 1 && day <= 31;
}

int instrument_validate(const Instrument *instrument, char *error, size_t error_size) {
    const char *message = NULL;
    if (instrument == NULL) {
        message = "instrument is required";
    } else if (instrument->id <= 0) {
        message = "id must be positive";
    } else if (is_blank(instrument->name)) {
        message = "name must not be blank";
    } else if (is_blank(instrument->category)) {
        message = "category must not be blank";
    } else if (is_blank(instrument->specification)) {
        message = "specification must not be blank";
    } else if (is_blank(instrument->model)) {
        message = "model must not be blank";
    } else if (!valid_date(instrument->purchase_date)) {
        message = "purchase date must use YYYY-MM-DD";
    } else if (instrument->price < 0.0) {
        message = "price must not be negative";
    } else if (instrument->quantity < 0) {
        message = "quantity must not be negative";
    }
    if (message != NULL && error != NULL && error_size > 0) {
        snprintf(error, error_size, "%s", message);
    }
    return message == NULL;
}

static int contains_case_insensitive_ascii(const char *text, const char *keyword) {
    size_t keyword_length = strlen(keyword);
    const char *cursor;
    if (keyword_length == 0) {
        return 1;
    }
    for (cursor = text; *cursor != '\0'; ++cursor) {
        size_t index = 0;
        while (index < keyword_length && cursor[index] != '\0'
               && tolower((unsigned char)cursor[index]) == tolower((unsigned char)keyword[index])) {
            ++index;
        }
        if (index == keyword_length) {
            return 1;
        }
    }
    return 0;
}

int instrument_contains(const Instrument *instrument, const char *keyword) {
    return instrument != NULL && keyword != NULL
           && (contains_case_insensitive_ascii(instrument->name, keyword)
               || contains_case_insensitive_ascii(instrument->category, keyword)
               || contains_case_insensitive_ascii(instrument->specification, keyword)
               || contains_case_insensitive_ascii(instrument->model, keyword)
               || contains_case_insensitive_ascii(instrument->purchase_date, keyword));
}
