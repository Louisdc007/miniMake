#include "exe.h"
#include "parse.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

char **to_execute(makefile_data data, char *target) {
    if (target == NULL)
        return NULL;
    if (data.nb_rules == -1)
        return NULL;

    rule targeted_rule;
    int verif_single = 0;
    var *data_var = data.var_tab;
    for (int i = 0; i < data.nb_rules; i++)
    {
        if (strcmp(data.rules[i].target, target) == 0)
        {
            targeted_rule = data.rules[i];
            verif_single++;
        }
        
    }

    if (verif_single > 1) {
        printf("Warning : plusieurs règles ont le nom %s, la dérnière définie est executée.\n", target);
    } else if (verif_single == 0)
        return NULL;

    
}