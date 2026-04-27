#include "exe.h"
#include "parse.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

/* ── Recipe execution ────────────────────────────────────────────────────── */

int execute_recipe(char **recipe, const char *target_name) {
    if (!recipe) return 0;
    for (int i = 0; recipe[i] != NULL; i++) {
        printf("%s\n", recipe[i]);
        fflush(stdout);

        pid_t pid = fork();
        if (pid == -1) {
            fprintf(stderr, "minimake: fork failed\n");
            return 2;
        }
        if (pid == 0) {
            char *args[] = { "/bin/sh", "-c", recipe[i], NULL };
            execvp(args[0], args);
            _exit(127);
        }
        int wstatus;
        waitpid(pid, &wstatus, 0);
        int code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 2;
        if (code != 0) {
            fprintf(stderr, "minimake: *** [%s] Error %d\n", target_name, code);
            fflush(stderr);
            return code;
        }
    }
    return 0;
}

/* ── Build engine ────────────────────────────────────────────────────────── */

/*
 * Returns: 0 = up-to-date / no work needed
 *          1 = recipe was executed successfully
 *          2 = error
 */
int build_target(makefile_data data, char *target, char *needed_by) {
    if (!target || target[0] == '\0') return 0;

    /* Find rule */
    int ri = -1;
    for (int i = 0; i < data.nb_rules; i++)
        if (strcmp(data.rules[i].target, target) == 0) { ri = i; break; }

    if (ri == -1) {
        /* Not a rule: acceptable if the file exists */
        struct stat st;
        if (stat(target, &st) == 0) return 0;
        if (needed_by)
            fprintf(stderr,
                "minimake: *** No rule to make target '%s', needed by '%s'.  Stop.\n",
                target, needed_by);
        else
            fprintf(stderr,
                "minimake: *** No rule to make target '%s'.  Stop.\n", target);
        fflush(stderr);
        return 2;
    }

    rule *r = &data.rules[ri];

    struct stat ts;
    int target_exists  = (stat(target, &ts) == 0);
    int needs_rebuild  = !target_exists;

    /* Process dependencies */
    int nb_dep = 0;
    if (r->dep) while (r->dep[nb_dep]) nb_dep++;

    for (int i = 0; i < nb_dep; i++) {
        char *dep = r->dep[i];
        if (!dep || dep[0] == '\0') continue;

        int dep_ret = build_target(data, dep, target);
        if (dep_ret == 2) return 2;

        if (dep_ret == 1) {
            needs_rebuild = 1;   /* dep was just built */
        } else if (!needs_rebuild) {
            /* dep up-to-date: compare timestamps */
            struct stat ds;
            if (stat(dep, &ds) == 0 && target_exists) {
#ifdef __APPLE__
                if (ds.st_mtimespec.tv_sec  > ts.st_mtimespec.tv_sec ||
                   (ds.st_mtimespec.tv_sec == ts.st_mtimespec.tv_sec &&
                    ds.st_mtimespec.tv_nsec > ts.st_mtimespec.tv_nsec))
                    needs_rebuild = 1;
#else
                if (ds.st_mtime > ts.st_mtime)
                    needs_rebuild = 1;
#endif
            }
        }
    }

    if (!needs_rebuild) {
        if (!needed_by) {   /* top-level target only */
            printf("minimake: '%s' is up to date.\n", target);
            fflush(stdout);
        }
        return 0;
    }

    /* No recipe → target is considered built (e.g. phony with no commands) */
    if (r->nb_recipe == 0) return 1;

    /* Expand recipes at execution time and run them */
    char **exp = malloc(sizeof(char *) * (r->nb_recipe + 1));
    if (!exp) return 2;
    for (int i = 0; i < r->nb_recipe; i++) {
        exp[i] = expand_vars(r->recipe[i], data.var_tab, data.nb_var);
        if (!exp[i]) {
            for (int m = 0; m < i; m++) free(exp[m]);
            free(exp); return 2;
        }
    }
    exp[r->nb_recipe] = NULL;

    int ret = execute_recipe(exp, target);
    for (int i = 0; i < r->nb_recipe; i++) free(exp[i]);
    free(exp);
    return (ret == 0) ? 1 : 2;
}

/* ── Print mode (-p) ─────────────────────────────────────────────────────── */

void print_data(makefile_data data) {
    printf("# variables\n");
    for (int i = 0; i < data.nb_var; i++)
        printf("'%s' = '%s'\n", data.var_tab[i].label, data.var_tab[i].data);

    printf("\n# rules\n");
    for (int i = 0; i < data.nb_rules; i++) {
        /* (target): [dep1] [dep2] ... */
        printf("(%s):", data.rules[i].target);
        if (data.rules[i].dep)
            for (int j = 0; data.rules[i].dep[j]; j++)
                printf(" [%s]", data.rules[i].dep[j]);
        printf("\n");
        /* Raw (unexpanded) recipe lines */
        for (int j = 0; j < data.rules[i].nb_recipe; j++)
            printf("\t'%s'\n", data.rules[i].recipe[j]);
    }
}