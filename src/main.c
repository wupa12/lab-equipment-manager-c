#include "cli.h"

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>

static char *argument_to_utf8(const char *argument) {
    int wide_length;
    int utf8_length;
    wchar_t *wide;
    char *utf8;
    wide_length = MultiByteToWideChar(CP_ACP, 0, argument, -1, NULL, 0);
    if (wide_length <= 0) {
        return NULL;
    }
    wide = (wchar_t *)malloc((size_t)wide_length * sizeof(wchar_t));
    if (wide == NULL) {
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, argument, -1, wide, wide_length);
    utf8_length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (utf8_length <= 0) {
        free(wide);
        return NULL;
    }
    utf8 = (char *)malloc((size_t)utf8_length);
    if (utf8 != NULL) {
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8_length, NULL, NULL);
    }
    free(wide);
    return utf8;
}
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    char **utf8_arguments;
    int index;
    int result;
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    utf8_arguments = (char **)calloc((size_t)argc + 1, sizeof(char *));
    if (utf8_arguments == NULL) {
        return 1;
    }
    for (index = 0; index < argc; ++index) {
        utf8_arguments[index] = argument_to_utf8(argv[index]);
        if (utf8_arguments[index] == NULL) {
            while (index > 0) {
                free(utf8_arguments[--index]);
            }
            free(utf8_arguments);
            return 1;
        }
    }
    result = cli_run(argc, utf8_arguments);
    for (index = 0; index < argc; ++index) {
        free(utf8_arguments[index]);
    }
    free(utf8_arguments);
    return result;
#else
    return cli_run(argc, argv);
#endif
}
