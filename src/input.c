#include "input.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int input_read_line(const char *prompt, char *buffer, size_t size) {
    size_t length;
    int character;
    if (buffer == NULL || size < 2) {
        return 0;
    }
    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else {
        while ((character = getchar()) != '\n' && character != EOF) {
        }
    }
    if (length > 0 && buffer[length - 1] == '\r') {
        buffer[length - 1] = '\0';
    }
    return 1;
}

int input_parse_int(const char *text, int *value) {
    char *end;
    long parsed;
    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    parsed = strtol(text, &end, 10);
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    if (errno != 0 || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

int input_parse_double(const char *text, double *value) {
    char *end;
    double parsed;
    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    parsed = strtod(text, &end);
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    if (errno != 0 || *end != '\0') {
        return 0;
    }
    *value = parsed;
    return 1;
}

int input_prompt_int(const char *prompt, int minimum, int *value) {
    char line[128];
    while (input_read_line(prompt, line, sizeof(line))) {
        if (input_parse_int(line, value) && *value >= minimum) {
            return 1;
        }
        printf("输入无效，请输入不小于 %d 的整数。\n", minimum);
    }
    return 0;
}

int input_prompt_double(const char *prompt, double minimum, double *value) {
    char line[128];
    while (input_read_line(prompt, line, sizeof(line))) {
        if (input_parse_double(line, value) && *value >= minimum) {
            return 1;
        }
        printf("输入无效，请输入不小于 %.2f 的数字。\n", minimum);
    }
    return 0;
}
