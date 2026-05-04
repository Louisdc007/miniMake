#ifndef EXE_H
#define EXE_H

#include "parse.h"

int build_target(makefile_data data, char *target, char *needed_by);
int execute_recipe(char **recipe, const char *target_name);
void print_data(makefile_data data);

#endif