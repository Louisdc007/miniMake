void free_makefile_data(makefile_data data) {
    if (data.var_tab) {
        for (int i = 0; i < data.nb_var; i++) {
            free(data.var_tab[i].label);
            free(data.var_tab[i].data);
        }
        free(data.var_tab);
    }
    if (data.rules) {
        for (int i = 0; i < data.nb_rules; i++) {
            free(data.rules[i].target);
            if (data.rules[i].dep) {
                for (int j = 0; data.rules[i].dep[j]; j++) {
                    free(data.rules[i].dep[j]);
                }
                free(data.rules[i].dep);
            }
            if (data.rules[i].recipe) {
                for (int j = 0; j < data.rules[i].nb_recipe; j++) {
                    free(data.rules[i].recipe[j]);
                }
                free(data.rules[i].recipe);
            }
        }
        free(data.rules);
    }
}
