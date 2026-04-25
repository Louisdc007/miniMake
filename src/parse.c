#include "parse.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **cutter(char *str, char sep) {
    int count = 1;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == sep)
            count++;
    }
    
    char **tab = malloc(sizeof(char *) * (count + 1));
    if (tab == NULL)
        return NULL;

    int token_idx = 0;
    int i = 0;
    
    while (token_idx < count && str[i] != '\0') {

        while (str[i] == ' ' && str[i] != '\0')
            i++;
        
        int start = i;
        
        while (str[i] != sep && str[i] != '\0')
            i++;
        
        int end = i - 1;
        
        while (end >= start && str[end] == ' ')
            end--;
        
        int size = 0;
        if (end >= start)
            size = end - start + 1;
        
        tab[token_idx] = malloc(sizeof(char) * (size + 1));
        if (tab[token_idx] == NULL) {
            for (int m = 0; m < token_idx; m++) {
                free(tab[m]);
            }
            free(tab);
            return NULL;
        }
        
        for (int m = 0; m < size; m++) {
            tab[token_idx][m] = str[start + m];
        }
        tab[token_idx][size] = '\0';
        
        token_idx++;
        
        if (str[i] == sep)
            i++;
    }
    
    tab[count] = NULL;
    return tab;
}

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

int len(char **tab) {
    if (tab == NULL || *tab == NULL)
        return 0;

    int i = 0;
    while (tab[i] != NULL)
        i++;

    return i;
}

makefile_data parse_makefile(const char *filename) {
    if (filename == NULL)
        {makefile_data final_err;
        final_err.nb_rules = -1;
        return final_err;}

    FILE *f = fopen(filename, "r");
    if (f == NULL)
        {makefile_data final_err;
        final_err.nb_rules = -1;
        return final_err;}

    makefile_data final;
    final.nb_rules = 0;
    final.nb_var = 0;
    final.rules = NULL;
    final.var_tab = NULL;
    char *buffer = NULL;
    size_t s = 0;

    int in_rule = 0;

    while (getline(&buffer, &s, f) != -1) {

        line_type curr_line_type = get_line_type(buffer);
        if (buffer[strlen(buffer) - 1] == '\n') buffer[strlen(buffer) - 1] = '\0';


        if (curr_line_type == VAR) {

            char **tab = cutter(buffer, '=');

            var curr_var;
            curr_var.data = tab[1];
            curr_var.label = tab[0];


            var *temp = realloc(final.var_tab, sizeof(var) * (final.nb_var + 1));
            
            if (temp == NULL) {
                makefile_data final_err;
                final_err.nb_rules = -1;
                return final_err;
            }

            final.var_tab = temp;
            final.nb_var++;
            final.var_tab[final.nb_var - 1] = curr_var;
            free(tab);
            in_rule = 0;
        }
        else if (curr_line_type == DEF)
        {
            char **tab = cutter(buffer, ':');

            rule curr_rule;
            curr_rule.target = tab[0];

            char **dep_tab = cutter(tab[1], ' ');
            curr_rule.dep = dep_tab;
            curr_rule.recipe = NULL;
            curr_rule.nb_recipe = 0;

            rule *temp = realloc(final.rules, sizeof(rule) * (final.nb_rules + 1));

            if (temp == NULL) {
                makefile_data final_err;
                final_err.nb_rules = -1;
                return final_err;
            }

            final.rules = temp;
            final.nb_rules++;
            final.rules[final.nb_rules - 1] = curr_rule;

            in_rule = 1;
            free(tab[1]);
            free(tab);
        }
        else if (curr_line_type == RECIPE)
        {
            if (in_rule != 1) {
                makefile_data final_err;
                final_err.nb_rules = -1;
                return final_err;
            }

            if (final.rules[final.nb_rules - 1].nb_recipe == 0) {
                final.rules[final.nb_rules - 1].recipe = malloc(sizeof(char *));
                if (final.rules[final.nb_rules - 1].recipe == NULL) {
                    makefile_data final_err;
                    final_err.nb_rules = -1;
                    return final_err;
                }

                final.rules[final.nb_rules - 1].nb_recipe++;

                char *curr_recipe = strdup(buffer);
                if (curr_recipe == NULL) {
                    makefile_data final_err;
                    final_err.nb_rules = -1;
                    return final_err;
                }
                final.rules[final.nb_rules - 1].recipe[0] = curr_recipe;
            } else {
                char **temp = realloc(final.rules[final.nb_rules - 1].recipe, sizeof(char *) * (final.rules[final.nb_rules - 1].nb_recipe + 1));
                if (temp == NULL) {
                    makefile_data final_err;
                    final_err.nb_rules = -1;
                    return final_err;
                }
                final.rules[final.nb_rules - 1].recipe = temp;

                char *curr_recipe = strdup(buffer);
                if (curr_recipe == NULL) {
                    makefile_data final_err;
                    final_err.nb_rules = -1;
                    return final_err;
                }
                final.rules[final.nb_rules - 1].recipe[final.rules[final.nb_rules - 1].nb_recipe] = curr_recipe;
                final.rules[final.nb_rules - 1].nb_recipe++;
            }
        } else if (curr_line_type == EMPTY) {
            in_rule = 0;
        } else {
            in_rule = 0;
            //cas normalement non atteint
        }
    }
    free(buffer);
    fclose(f);
    return final;
}