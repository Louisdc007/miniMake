#ifndef PARSE_H
#define PARSE_H

typedef enum { RECIPE, DEF, VAR, EMPTY } line_type;

typedef struct {
    char *label;
    char *data;
} var;

typedef struct {
    char *target;
    char **dep;
    char **recipe;   /* raw, unexpanded — expansion happens at execution */
    int   nb_recipe;
} rule;

typedef struct {
    rule *rules;
    int   nb_rules;
    var  *var_tab;
    int   nb_var;
} makefile_data;

makefile_data parse_makefile(const char *filename);
char        **cutter(char *str, char sep);
line_type     get_line_type(char *line);
int           len(char **tab);

/* Variable expansion — used by both parse and exe layers */
char *expand_vars(char *line, var *var_tab, int nb_var);
char *get_var_value(char *name, var *var_tab, int nb_var);

#endif