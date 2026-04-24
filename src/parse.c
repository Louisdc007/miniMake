#include "parse.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

line_type get_line_type (char *line) {
    if (line[0] == '\t') {
        return RECIPE;
    }
    for (size_t i = 0; i < strlen(line); i++) {
        switch (line[i])
        {
        case ':':
            return DEF;
            break;
        case '=':
            return VAR;
            break;
        default:
            break;
        }
    }
    return EMPTY;
}

makefile_data parse_makefile(const char *filename) {
    if (filename == NULL)
        return;

    FILE *f = fopen(filename, "r");
    if (f == NULL)
        return;

    char *buffer = NULL;
    size_t s = 0;
    while (getline(&buffer, &s, f) != -1) {
        line_type curr_line_type = get_line_type(buffer);
        // utilisation a l'étape suivante
    }
    free(buffer);
    fclose(f);
    return;
}