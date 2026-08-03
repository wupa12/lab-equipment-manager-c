#ifndef LAB_MANAGER_INPUT_H
#define LAB_MANAGER_INPUT_H

#include <stddef.h>

int input_read_line(const char *prompt, char *buffer, size_t size);
int input_parse_int(const char *text, int *value);
int input_parse_double(const char *text, double *value);
int input_prompt_int(const char *prompt, int minimum, int *value);
int input_prompt_double(const char *prompt, double minimum, double *value);

#endif
