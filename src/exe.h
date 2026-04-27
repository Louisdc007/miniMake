#ifndef EXE_H
#define EXE_H

#include "parse.h"

/*
 * Build a target recursively.
 * needed_by: parent rule name (for error messages), or NULL if top-level.
 * Returns: 0 = up-to-date, 1 = built successfully, 2 = error.
 */
int build_target(makefile_data data, char *target, char *needed_by);

/*
 * Execute recipe commands.
 * target_name is used in the error message on failure.
 * Returns 0 on success, the command's exit code on failure.
 */
int execute_recipe(char **recipe, const char *target_name);

/* Print parsed makefile in the -p format. */
void print_data(makefile_data data);

#endif