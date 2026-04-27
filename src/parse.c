#include "parse.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Variable expansion (usable at parse time) ───────────────────────────── */

char *get_var_value(char *name, var *var_tab, int nb_var) {
    for (int i = 0; i < nb_var; i++)
        if (strcmp(name, var_tab[i].label) == 0)
            return var_tab[i].data;
    return "";
}

char *expand_vars(char *line, var *var_tab, int nb_var) {
    if (line == NULL) return strdup("");
    size_t buf_size = strlen(line) * 2 + 256;
    char *result = malloc(buf_size);
    if (!result) return NULL;
    result[0] = '\0';
    char  *src      = line;
    size_t dest_len = 0;

    while (*src) {
        if (*src == '$' && *(src + 1) == '{') {
            char *end = strchr(src + 2, '}');
            if (end) {
                int  vlen = (int)(end - (src + 2));
                char var_name[256];
                if (vlen >= 0 && vlen < 255) {
                    strncpy(var_name, src + 2, vlen);
                    var_name[vlen] = '\0';
                    char  *value   = get_var_value(var_name, var_tab, nb_var);
                    size_t val_len = strlen(value);
                    while (dest_len + val_len + 1 >= buf_size) {
                        buf_size *= 2;
                        char *tmp = realloc(result, buf_size);
                        if (!tmp) { free(result); return NULL; }
                        result = tmp;
                    }
                    memcpy(result + dest_len, value, val_len);
                    dest_len += val_len;
                    result[dest_len] = '\0';
                }
                src = end + 1;
                continue;
            }
            /* No closing '}': copy '$' literally and keep going */
        }
        if (dest_len + 2 >= buf_size) {
            buf_size *= 2;
            char *tmp = realloc(result, buf_size);
            if (!tmp) { free(result); return NULL; }
            result = tmp;
        }
        result[dest_len++] = *src++;
        result[dest_len]   = '\0';
    }
    return result;
}

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* Split str by sep, returning a NULL-terminated array of whitespace-trimmed
   non-empty tokens.  Used for deps (sep=' ') and old var splitting. */
char **cutter(char *str, char sep) {
    if (!str) {
        char **t = malloc(sizeof(char *));
        if (t) t[0] = NULL;
        return t;
    }
    int count = 0, i = 0;
    while (str[i]) {
        while (str[i] == ' ' || str[i] == '\t') i++;
        if (!str[i]) break;
        count++;
        while (str[i] != sep && str[i]) i++;
        if (str[i] == sep) i++;
    }
    char **tab = malloc(sizeof(char *) * (count + 1));
    if (!tab) return NULL;
    int idx = 0; i = 0;
    while (idx < count && str[i]) {
        while ((str[i] == ' ' || str[i] == '\t') && str[i]) i++;
        if (!str[i]) break;
        int start = i;
        while (str[i] != sep && str[i]) i++;
        int end = i - 1;
        while (end >= start && (str[end] == ' ' || str[end] == '\t')) end--;
        int sz = (end >= start) ? end - start + 1 : 0;
        tab[idx] = malloc(sz + 1);
        if (!tab[idx]) {
            for (int m = 0; m < idx; m++) free(tab[m]);
            free(tab); return NULL;
        }
        for (int m = 0; m < sz; m++) tab[idx][m] = str[start + m];
        tab[idx][sz] = '\0';
        idx++;
        if (str[i] == sep) i++;
    }
    tab[count] = NULL;
    return tab;
}

line_type get_line_type(char *line) {
    if (line[0] == '\t') return RECIPE;
    /* Strip inline comments */
    for (size_t i = 0; line[i]; i++)
        if (line[i] == '#') { line[i] = '\0'; break; }
    /* Blank check */
    int blank = 1;
    for (size_t i = 0; line[i]; i++)
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r') { blank = 0; break; }
    if (blank) return EMPTY;
    for (size_t i = 0; line[i]; i++) {
        if (line[i] == ':') return DEF;
        if (line[i] == '=') return VAR;
    }
    return EMPTY;
}

int len(char **tab) {
    if (!tab || !*tab) return 0;
    int i = 0;
    while (tab[i]) i++;
    return i;
}

/* ── Parser ──────────────────────────────────────────────────────────────── */

makefile_data parse_makefile(const char *filename) {
    makefile_data err = { NULL, -1, NULL, 0 };
    if (!filename) return err;
    FILE *f = fopen(filename, "r");
    if (!f) return err;

    makefile_data data = { NULL, 0, NULL, 0 };
    char  *buffer = NULL;
    size_t s      = 0;
    int    in_rule = 0;

    while (getline(&buffer, &s, f) != -1) {
        size_t b = strlen(buffer);
        if (b > 0 && buffer[b-1] == '\n') buffer[--b] = '\0';
        if (b > 0 && buffer[b-1] == '\r') buffer[--b] = '\0';

        line_type ltype = get_line_type(buffer);

        /* ── Variable definition ─────────────────────────────────────── */
        if (ltype == VAR) {
            char *eq = strchr(buffer, '=');
            if (!eq) { in_rule = 0; continue; }
            *eq = '\0';

            /* Label: trim both sides */
            char *lraw = buffer;
            int   ll   = (int)strlen(lraw);
            while (ll > 0 && (lraw[ll-1] == ' ' || lraw[ll-1] == '\t')) ll--;
            int ls = 0;
            while (lraw[ls] == ' ' || lraw[ls] == '\t') ls++;
            char *label = malloc(ll - ls + 1);
            if (!label) { free(buffer); fclose(f); return err; }
            strncpy(label, lraw + ls, ll - ls);
            label[ll - ls] = '\0';

            /* Value: trim leading whitespace only, preserve trailing.
               CCF: "The value of a variable spreads until the end of the line." */
            char *vraw = eq + 1;
            while (*vraw == ' ' || *vraw == '\t') vraw++;
            /* Expand using variables known so far (parse-time expansion) */
            char *value = expand_vars(vraw, data.var_tab, data.nb_var);
            if (!value) { free(label); free(buffer); fclose(f); return err; }

            var *tmp = realloc(data.var_tab, sizeof(var) * (data.nb_var + 1));
            if (!tmp) { free(label); free(value); free(buffer); fclose(f); return err; }
            data.var_tab = tmp;
            data.var_tab[data.nb_var].label = label;
            data.var_tab[data.nb_var].data  = value;
            data.nb_var++;
            in_rule = 0;
        }
        /* ── Rule definition ─────────────────────────────────────────── */
        else if (ltype == DEF) {
            char *colon = strchr(buffer, ':');
            if (!colon) { in_rule = 0; continue; }
            *colon = '\0';

            /* Target: trim + expand at parse time */
            char *traw = buffer;
            int   tl   = (int)strlen(traw);
            while (tl > 0 && (traw[tl-1] == ' ' || traw[tl-1] == '\t')) tl--;
            int ts = 0;
            while (traw[ts] == ' ' || traw[ts] == '\t') ts++;
            char tmp_t[tl - ts + 1];
            strncpy(tmp_t, traw + ts, tl - ts);
            tmp_t[tl - ts] = '\0';
            char *target = expand_vars(tmp_t, data.var_tab, data.nb_var);
            if (!target) { free(buffer); fclose(f); return err; }
            /* Ignore rules with empty target after expansion */
            if (target[0] == '\0') { free(target); in_rule = 0; continue; }

            /* Deps: split by whitespace, expand each, skip empty results */
            char **raw_deps = cutter(colon + 1, ' ');
            int    ndep     = 0;
            if (raw_deps) while (raw_deps[ndep]) ndep++;
            char **deps = malloc(sizeof(char *) * (ndep + 1));
            if (!deps) { free(target); if (raw_deps) { for (int m=0;raw_deps[m];m++) free(raw_deps[m]); free(raw_deps); } free(buffer); fclose(f); return err; }
            int vdc = 0;
            for (int d = 0; d < ndep; d++) {
                char *exp = expand_vars(raw_deps[d], data.var_tab, data.nb_var);
                free(raw_deps[d]);
                if (exp && exp[0] != '\0')
                    deps[vdc++] = exp;
                else
                    free(exp);
            }
            deps[vdc] = NULL;
            free(raw_deps);

            rule curr = { target, deps, NULL, 0 };
            rule *tmp2 = realloc(data.rules, sizeof(rule) * (data.nb_rules + 1));
            if (!tmp2) { free(target); for (int m=0;m<vdc;m++) free(deps[m]); free(deps); free(buffer); fclose(f); return err; }
            data.rules = tmp2;
            data.rules[data.nb_rules++] = curr;
            in_rule = 1;
        }
        /* ── Recipe line ─────────────────────────────────────────────── */
        else if (ltype == RECIPE) {
            if (!in_rule) {
                fprintf(stderr, "minimake: command outside of rule\n");
                free(buffer); fclose(f); return err;
            }
            rule *cur = &data.rules[data.nb_rules - 1];
            char **tmp3 = realloc(cur->recipe, sizeof(char *) * (cur->nb_recipe + 2));
            if (!tmp3) { free(buffer); fclose(f); return err; }
            cur->recipe = tmp3;
            /* Strip leading tab; store RAW (unexpanded) */
            char *rl = buffer;
            if (*rl == '\t') rl++;
            char *raw = strdup(rl);
            if (!raw) { free(buffer); fclose(f); return err; }
            cur->recipe[cur->nb_recipe]     = raw;
            cur->recipe[cur->nb_recipe + 1] = NULL;
            cur->nb_recipe++;
        }
        /* ── Empty / comment line ────────────────────────────────────── */
        else {
            in_rule = 0;
        }
    }

    free(buffer);
    fclose(f);
    return data;
}