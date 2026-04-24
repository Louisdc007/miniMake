#ifndef PARSE_H
#define PARSE_H


typedef enum 
{
    RECIPE,
    DEF,
    VAR,
    EMPTY
} line_type;

typedef struct {
    char *label;
    char* data; 
} var;


typedef struct {
    char *target;
    char **dep;
    char **recipe;
} rule;

typedef struct {
    rule *rules;
    int nb_rules;

    var *var_tab;
    int nb_var;
} makefile_data;

makefile_data parse_makefile(const char *filename);

#endif