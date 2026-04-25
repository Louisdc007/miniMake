#ifndef EXE_H
#define EXE_H

#include "parse.h"

char **to_execute(makefile_data data, char* target); //retourne une liste de str représentant les comandes à executer dans la regles target

int execute_list(char **); //execute la liste de str donnée

#endif