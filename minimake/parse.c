#include "parse.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Vu que dans les tp on utilise du voc stylé je fais la même
mot = token
*/

/*
recup val de var avec son nom
 */
char *get_var_value(char *name, var *var_tab, int nb_var) {
  for (int i = 0; i < nb_var; i++)
    if (strcmp(name, var_tab[i].label) == 0)
      return var_tab[i].data;
  return "";
}

/*
check taille du buffer
 */
static int buf_ensure(char **buf, size_t *cap, size_t need) {
  while (need >= *cap) { // tant que capacité insuf
    *cap *= 2;           // double la cap
    char *tmp = realloc(*buf, *cap);
    if (!tmp) {
      free(*buf);
      *buf = NULL;
      return -1;
    }
    *buf = tmp;
  }
  return 0; // réussi
}

/*
Développe var dans chaine
 */
char *expand_vars(char *line, var *var_tab, int nb_var) {
  if (line == NULL)
    return strdup("");
  size_t cap = strlen(line) * 2 + 256;
  char *result = malloc(cap);
  if (!result)
    return NULL;
  result[0] = '\0';
  char *src = line;
  size_t len_r = 0;

  while (*src) {               // on parcourt la chaine
    if (*src == '$') {         // detecte appel var
      if (*(src + 1) == '$') { // gère $$
        if (buf_ensure(&result, &cap, len_r + 2) < 0)
          return NULL;
        result[len_r++] = '$';
        result[len_r] = '\0';
        src += 2;
        continue;
      }
      char var_name[256] = {0}; // nom de la variable
      int is_var = 0;           // booléen pour var trouvée
      char *end = NULL;         // fin variable

      if (*(src + 1) == '{' || *(src + 1) == '(') {      // ${} ou $()
        char end_char = (*(src + 1) == '{') ? '}' : ')'; // caractère de fin
        end = strchr(src + 2, end_char);      // cherche le caractère de fin
        if (end) { // Si on a trouvé la fin
          int vlen = (int)(end - (src + 2)); // Calcule la longueur du nom de la variable
          if (vlen >= 0 && vlen < 255) { // Sécurité pour ne pas déborder du buffer var_name
            strncpy(var_name, src + 2, vlen); // Copie le nom de la variable
            var_name[vlen] = '\0'; // Ajoute le terminateur de chaîne
            is_var = 1; // Variable trouvée
          }
        } else {
          printf("minimake: *** unterminated variable reference.  Stop.\n");
          fflush(stdout);
          free(result);
          return NULL;
        }
      } else if (*(src + 1) != '\0' && *(src + 1) != ' ' &&
                 *(src + 1) != '\t') { // les var prédef ou avec 1 char
        var_name[0] = *(src + 1);      // le noom est le char
        var_name[1] = '\0';            // termine la chaine
        is_var = 1;                    // var trouvée
        end = src + 1; // le caractère de fin est ce caractère même
      }

      if (is_var) { // si on a une var
        char *raw_value =
            get_var_value(var_name, var_tab, nb_var); // on recup la valeur
        char *value = expand_vars(
            raw_value, var_tab,
            nb_var); // on check voir si la var utilise elle meme des vars
        if (value) {
          size_t val_len = strlen(value); // longueur de valeur
          if (buf_ensure(&result, &cap, len_r + val_len + 1) <
              0) { // verif si assez place
            free(value);
            return NULL;
          }
          memcpy(result + len_r, value, val_len); // ajoute la valeur
          len_r += val_len;                       // up la long du result
          result[len_r] = '\0';
          free(value);
        }
        src = end + 1; // avance le curseur
        continue;
      }
    }
    // char pas $
    if (buf_ensure(&result, &cap, len_r + 2) < 0)
      return NULL;
    result[len_r++] = *src++; // on add ce char sans rien toucher
    result[len_r] = '\0';
  }
  return result;
}

/*
nb de mots sep par un space
 */
static int count_tokens(char *str, char sep) {
  int count = 0, i = 0;
  while (str[i]) {
    while (str[i] == ' ' || str[i] == '\t')
      i++;
    if (!str[i])
      break;
    count++;
    while (str[i] != sep && str[i])
      i++;
    if (str[i] == sep)
      i++;
  }
  return count; // nb de mots trouvé
}

/*
recup le prochain mots, ignore les spaces
 */
static char *extract_token(char *str, char sep, int *pos) {
  int i = *pos; // commence a la pos
  while ((str[i] == ' ' || str[i] == '\t') && str[i])
    i++; // ignore les spaces
  if (!str[i])
    return NULL; // fin
  int start = i; // début du mot
  while (str[i] != sep && str[i])
    i++;           // cherche la fin du mot
  int end = i - 1; // Dernier caractère du mot
  while (end >= start && (str[end] == ' ' || str[end] == '\t'))
    end--;                                       // retire des espaces de la fin
  int sz = (end >= start) ? end - start + 1 : 0; // calcule taille du mot
  char *tok = malloc(sz + 1);
  if (!tok)
    return NULL;
  for (int m = 0; m < sz; m++)
    tok[m] = str[start + m]; // copie char/char
  tok[sz] = '\0';
  if (str[i] == sep)
    i++; // ignore les spaces
  *pos = i;
  return tok;
}

/*
chaine -> tab de mots
 */
char **cutter(char *str, char sep) {
  if (!str) {
    char **t = malloc(sizeof(char *));
    if (t)
      t[0] = NULL;
    return t;
  }
  int count = count_tokens(str, sep); // compte mots
  char **tab = malloc(sizeof(char *) * (count + 1));
  if (!tab)
    return NULL;
  int pos = 0; // pos de départ
  for (int idx = 0; idx < count; idx++) {
    tab[idx] = extract_token(str, sep, &pos);
    if (!tab[idx]) { // en cas de prblm
      for (int m = 0; m < idx; m++)
        free(tab[m]);
      free(tab);
      return NULL;
    }
  }
  tab[count] = NULL;
  return tab;
}

/*
determine type d'une ligne
 */
line_type get_line_type(char *line) {
  if (line[0] == '\t')
    return RECIPE;                 // si \t -> recette
  for (size_t i = 0; line[i]; i++) // on va chercher les #
    if (line[i] == '#') {
      line[i] = '\0';
      break;
    } // # = on ignore
  int blank = 1;                   // indique la ligne vide
  for (size_t i = 0; line[i]; i++) // check toute les cases
    if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r') {
      blank = 0;
      break;
    } // si char visible -> pas vide
  if (blank)
    return EMPTY;                    // si vide : type EMPTY
  for (size_t i = 0; line[i]; i++) { // cherche les motifs
    if (line[i] == ':')
      return DEF; // def regle
    if (line[i] == '=')
      return VAR; // def var
  }
  return EMPTY; // par défaut
}

/*
longueue tab de **char
 */
int len(char **tab) {
  if (!tab || !*tab)
    return 0;
  int i = 0;
  while (tab[i])
    i++;
  return i;
}

/*
suppr espace debut et fin + duplique (trim + dup)
 */
static char *trim_and_dup(char *raw, int raw_len) {
  int end = raw_len; // curs de fin
  while (end > 0 && (raw[end - 1] == ' ' || raw[end - 1] == '\t'))
    end--;       // recule pour les espaces de fin
  int start = 0; // curs de début
  while (start < end && (raw[start] == ' ' || raw[start] == '\t'))
    start++;            // avance tant que espace au debut
  int sz = end - start; // taille utile (sans esp)
  char *out = malloc(sz + 1);
  if (!out)
    return NULL;
  strncpy(out, raw + start, sz);
  out[sz] = '\0';
  return out;
}

/*
retire le saut a la fin du buf (genre pour getline)
 */
static void strip_newline(char *buffer, size_t *b) {
  if (*b > 0 && buffer[*b - 1] == '\n')
    buffer[--(*b)] = '\0';
  if (*b > 0 && buffer[*b - 1] == '\r')
    buffer[--(*b)] = '\0';
}

/*
ajoute une var au tab de donnée du makefile
 */
static int add_var(makefile_data *data, char *label, char *value) {
  var *tmp = realloc(data->var_tab, sizeof(var) * (data->nb_var + 1));
  if (!tmp)
    return -1;
  data->var_tab = tmp;
  data->var_tab[data->nb_var].label = label; // nom de la var
  data->var_tab[data->nb_var].data = value;  // val de la var
  data->nb_var++;
  return 0;
}

/*
parse une ligne VAR
 */
static int parse_var_line(char *buffer, makefile_data *data) {
  char *eq = strchr(buffer, '=');
  if (!eq)
    return 0;
  *eq = '\0';

  char *label = trim_and_dup(buffer, (int)strlen(buffer));
  if (!label)
    return -1;

  char *vraw = eq + 1; // debut de valeur
  while (*vraw == ' ' || *vraw == '\t')
    vraw++; // degage espaces apres =
  char *value = expand_vars(vraw, data->var_tab,
                            data->nb_var); // integre les var dans la var
  if (!value) {
    free(label);
    return -1;
  }

  if (add_var(data, label, value) < 0) {
    free(label);
    free(value);
    return -1;
  }
  return 0; // victoire
}

/*
expanse les deps de la regle
 */
static char **expand_deps(char **raw_deps, int ndep, var *var_tab, int nb_var,
                          int *out_count) {
  char **deps = malloc(sizeof(char *) * (ndep + 1));
  if (!deps)
    return NULL;
  int vdc = 0;                     // compteur de dep (valid dep counter)
  for (int d = 0; d < ndep; d++) { // Parcourt toutes les dépendances brutes
    char *exp = expand_vars(raw_deps[d], var_tab, nb_var); // Remplace les variables dans la dépendance
    free(raw_deps[d]); // On n'a plus besoin du nom de dépendance brut
    if (!exp) {
      for (int m = 0; m < vdc; m++) free(deps[m]);
      free(deps);
      for (int k = d + 1; k < ndep; k++) free(raw_deps[k]);
      return NULL;
    }
    if (exp[0] != '\0') // Si la dépendance expansée n'est pas vide
      deps[vdc++] = exp; // On l'ajoute à la liste des dépendances valides
    else // Si elle est devenue vide suite à l'expansion (ex: var non définie)
      free(exp); // On l'ignore
  }
  deps[vdc] = NULL;
  *out_count = vdc; // nb de vraie dep
  return deps;
}

/*
Parse règle
 */
static int parse_rule_line(char *buffer, makefile_data *data) {
  char *sep = strchr(buffer, ':'); // cherche les : qui sépare cible de dep
  if (!sep)
    return 0;
  *sep = '\0'; // coupe les chaines

  char *target = trim_and_dup(buffer, (int)strlen(buffer));
  if (!target)
    return -1;
  char *exp_target = expand_vars(target, data->var_tab,
                                 data->nb_var); // integre les var dans le nom
  free(target);
  if (!exp_target)
    return -1;
  if (exp_target[0] == '\0') {
    free(exp_target);
    return 2;
  }

  char **raw_deps = cutter(sep + 1, ' '); // découpe les deps entre espace
  int ndep = 0;                           // on compte les deps
  if (raw_deps)
    while (raw_deps[ndep])
      ndep++;

  int vdc = 0; // variable qui recoit le nb de dep final
  char **deps = expand_deps(raw_deps, ndep, data->var_tab, data->nb_var,
                            &vdc); // expansion des deps
  free(raw_deps);                  // libere brut
  if (!deps) {
    free(exp_target);
    return -1;
  }

  rule curr = {exp_target, deps, NULL, 0}; // structure de la regle
  rule *tmp = realloc(data->rules, sizeof(rule) * (data->nb_rules + 1));
  if (!tmp) { // en cas d'echec
    free(exp_target);
    for (int m = 0; m < vdc; m++)
      free(deps[m]);
    free(deps);
    return -1;
  }
  data->rules = tmp;
  data->rules[data->nb_rules++] = curr; // ajoute la règle et +1 le nb
  return 1;
}

/*
Parse ligne de recette (la commande) et add a la rule
 */
static int parse_recipe_line(char *buffer, makefile_data *data) {
  rule *cur = &data->rules[data->nb_rules - 1]; // recup la derniere rule
  char *rl = buffer;                            // curseur de lecture
  while (*rl == ' ' || *rl == '\t')
    rl++; // ignore tab et espace du début (ce qui dit que c'est une recette)
  if (*rl == '\0')
    return 0;

  char **tmp =
      realloc(cur->recipe,
              sizeof(char *) *
                  (cur->nb_recipe + 2)); // alloue de la place pour la nouvelle
                                         // commande et le NULL de fin
  if (!tmp)
    return -1;            // erreur allocation
  cur->recipe = tmp;      // met a jour le pointeur
  char *raw = strdup(rl); // copie la commande

  if (!raw)
    return -1;                            // erreur allocation
  cur->recipe[cur->nb_recipe] = raw;      // ajoute au tab
  cur->recipe[cur->nb_recipe + 1] = NULL; // term par null
  cur->nb_recipe++;                       // on vient d'ajouter une commande
  return 0;
}

/*
la main de cette cité (mdr) qui va connecter le reste pour analyser le mkfile
complet
 */
makefile_data parse_makefile(const char *filename) {
  makefile_data err = {NULL, -1, NULL, 0}; // on met l'erreur par def
  if (!filename)
    return err;
  FILE *f = fopen(filename, "r");
  if (!f)
    return err;

  makefile_data data = {NULL, 0, NULL,
                        0}; // on prepare pour y stocker l'environnement
  char *buffer = NULL;
  size_t s = 0;
  int in_rule = 0; // utilise 1 = dans règle, 2= rgle ignorée

  while (getline(&buffer, &s, f) != -1) {
    size_t b = strlen(buffer);
    strip_newline(buffer, &b);               // dgage le \n
    line_type ltype = get_line_type(buffer); // trouve le type de la ligne

    if (ltype == VAR) {
      if (parse_var_line(buffer, &data) < 0) { // tente de parser la var
        free(buffer);
        fclose(f);
        free_makefile_data(data);
        return err;
      }
      in_rule = 0;             // variable = fin de la def de regle
    } else if (ltype == DEF) { // definition de regle cible: dep
      int ret = parse_rule_line(buffer, &data); // parse regle
      if (ret < 0) {
        free(buffer);
        fclose(f);
        free_makefile_data(data);
        return err;
      }
      if (ret == 1)
        in_rule = 1; // on est dans la regles, on s'attend a des recettes
      else if (ret == 2)
        in_rule = 2;
      else
        in_rule = 0;
    } else if (ltype == RECIPE) { // si c'est une commande
      if (in_rule == 2)
        continue;     // si regle ignorée, on zap la commande
      if (!in_rule) { // si commande en dehors de regle
        printf("minimake: command outside of rule\n");
        fflush(stdout);
        free(buffer);
        fclose(f);
        free_makefile_data(data);
        return err;
      }
      if (parse_recipe_line(buffer, &data) <
          0) { // garde la recette pour la règle actuelle
        free(buffer);
        fclose(f);
        free_makefile_data(data);
        return err;
      }
    } else { // ltype == EMPTY
    }
  }

  free(buffer);
  fclose(f);
  return data;
}

/*
grand nettoyage de printemps
 */
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
