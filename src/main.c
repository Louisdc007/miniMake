#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "parse.h"
#include "exe.h"

static void print_help(void) {
    printf("Usage: minimake [OPTIONS] [TARGETS]\n");
    printf("    OPTIONS can be:\n");
    printf("        \"-h\"            to display the helper\n");
    printf("        \"-p\"            to print the rules and variables\n");
    printf("        \"-f\" <filename> to change the make config file, \"Makefile\" by default\n");
    printf("    TARGETS are target name of makefile rule to execute\n");
}

int main(int argc, char **argv) {
    char  *makefile_name = "Makefile";
    char **targets       = NULL;
    int    nb_targets    = 0;
    int    print_mode    = 0;

    /* ── Pass 1: detect -h, but skip tokens consumed by -f ── */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            i++;  /* next token is the filename, not a flag */
        } else if (strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
    }

    /* ── Pass 2: parse options ── */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "minimake: option '-f' requires an argument\n");
                return 2;
            }
            makefile_name = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            print_mode = 1;
        } else {
            /* Target name */
            char **tmp = realloc(targets, sizeof(char *) * (nb_targets + 1));
            if (!tmp) { free(targets); return 2; }
            targets = tmp;
            targets[nb_targets++] = argv[i];
        }
    }

    /* ── Check makefile exists ── */
    struct stat st;
    if (stat(makefile_name, &st) == -1) {
        fprintf(stderr, "minimake: %s: No such file or directory\n", makefile_name);
        free(targets);
        return 3;
    }

    /* ── Parse ── */
    makefile_data data = parse_makefile(makefile_name);
    if (data.nb_rules < 0) {
        /* parse error already printed */
        free(targets);
        return 2;
    }

    /* ── Mode -p: print raw parsed data (recipes NOT expanded) ── */
    if (print_mode) {
        print_data(data);
        free(targets);
        return 0;
    }

    /* ── No targets in makefile ── */
    if (data.nb_rules == 0) {
        fprintf(stderr, "minimake: *** No targets.  Stop.\n");
        free(targets);
        return 2;
    }

    /* ── Default target: first rule ── */
    if (nb_targets == 0) {
        targets = malloc(sizeof(char *));
        if (!targets) return 2;
        targets[0]  = data.rules[0].target;
        nb_targets  = 1;
    }

    /* ── Build targets in order; continue even if one fails ── */
    int overall = 0;
    for (int i = 0; i < nb_targets; i++) {
        int r = build_target(data, targets[i], NULL);
        if (r == 2) overall = 2;
    }

    free(targets);
    return overall;
}