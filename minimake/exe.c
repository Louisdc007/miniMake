#include "exe.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Exe commande une par une
 */
int execute_recipe(char **recipe, const char *target_name) {
  if (!recipe)
    return 0;
  for (int i = 0; recipe[i] != NULL; i++) {
    // affiche la commande
    printf("%s\n", recipe[i]);
    fflush(stdout);

    pid_t pid = fork(); // Fais le fork pour executer
    if (pid == -1) {
      printf("minimake: fork failed\n");
      fflush(stdout);
      return 2;
    }
    if (pid == 0) { // Dans l'enfant (le processus)
      char *args[] = {"/bin/sh", "-c", recipe[i], NULL};
      execvp(args[0], args);
      _exit(127);
    }
    // Le papa
    int wstatus;
    waitpid(pid, &wstatus, 0);
    int code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 2;
    if (code != 0) {
      printf("minimake: *** [%s] Error %d\n", target_name, code);
      fflush(stdout);
      return code;
    }
  }
  return 0;
}

/*
Exe avec dep et conditions de time
 */
static int build_target_internal(makefile_data data, char *target,
                                 char *needed_by, char ***visited,
                                 int *nb_visited, char ***built,
                                 int *nb_built) {
  if (!target || target[0] == '\0')
    return 0; // Si pas de sible : succes

  for (int i = 0; i < *nb_visited; i++) {     // Parcours les cibles déjà vue
    if (strcmp((*visited)[i], target) == 0) { // Si on y est déjà passé
      printf("minimake: Circular %s <- %s dependency dropped.\n",
             needed_by ? needed_by : target, target);
      fflush(stdout);
      return 0;
    }
  }

  // Detections des cible deja fabriquées
  for (int i = 0; i < *nb_built; i++) { // Cibles déja faites
    if (strcmp((*built)[i], target) == 0) {
      return 0;
    }
  }

  // Ajout aux deja vues
  char **tmp_v = realloc(*visited, sizeof(char *) * (*nb_visited + 1));
  if (tmp_v) {
    *visited = tmp_v;
    (*visited)[(*nb_visited)++] = target;
  }

  // Recherche de la cible dans les règles
  int ri = -1;
  for (int i = 0; i < data.nb_rules; i++) // Parcourt toutes les règles
    if (strcmp(data.rules[i].target, target) == 0) {
      ri = i;
      break;
    }

  if (ri == -1) { // Si rien correspond
    struct stat st;
    if (stat(target, &st) == 0) { // Si le fichier existe
      if (*nb_visited > 0)
        (*nb_visited)--;
      char **tmp_b = realloc(*built, sizeof(char *) * (*nb_built + 1));
      if (tmp_b) {
        *built = tmp_b;
        (*built)[(*nb_built)++] = target;
      }
      return 0;
    }
    if (needed_by) // Si cible était requise
      printf(
          "minimake: *** No rule to make target '%s', needed by '%s'.  Stop.\n",
          target, needed_by);
    else // Si c'était la cible principale demandée
      printf("minimake: *** No rule to make target '%s'.  Stop.\n",
             target); // erreur
    fflush(stdout);
    if (*nb_visited > 0)
      (*nb_visited)--;
    return 2;
  }

  rule *r = &data.rules[ri]; // règle qui correspond à la cible

  /* On vérif si la cible est à jour */
  struct stat ts;
  int target_exists = (stat(target, &ts) == 0); // Vrai si le fichier existe
  int needs_rebuild = !target_exists;           // Sinon on doit reconstruire

  /* On compte le nombre de dépendances */
  int nb_dep = 0;
  if (r->dep)
    while (r->dep[nb_dep])
      nb_dep++;

  /* On parcourt les dépendances */
  for (int i = 0; i < nb_dep; i++) {
    char *dep = r->dep[i];
    if (!dep || dep[0] == '\0')
      continue;

    /* Appel récursif pour construire ou check la dépendance */
    int dep_ret = build_target_internal(data, dep, target, visited, nb_visited,
                                        built, nb_built);
    if (dep_ret == 2) {
      if (*nb_visited > 0)
        (*nb_visited)--;
      return 2;
    }

    if (dep_ret == 1) {
      needs_rebuild = 1; // Besoin de reconstruire
    } else if (!needs_rebuild) {
      /* Vérifie si la dépendance est plus récente que la cible existante */
      struct stat ds;
      if (stat(dep, &ds) == 0 &&
          target_exists) { // vérif si la dépendance existe et que la cible
                           // aussi
#ifdef __APPLE__
        if (ds.st_mtimespec.tv_sec > ts.st_mtimespec.tv_sec ||
            (ds.st_mtimespec.tv_sec == ts.st_mtimespec.tv_sec &&
             ds.st_mtimespec.tv_nsec > ts.st_mtimespec.tv_nsec))
          needs_rebuild = 1; // Si dep récentes alors reconstruire
#else                        // Cas standard pour Linux / POSIX
        if (ds.st_mtim.tv_sec > ts.st_mtim.tv_sec ||
            (ds.st_mtim.tv_sec == ts.st_mtim.tv_sec &&
             ds.st_mtim.tv_nsec > ts.st_mtim.tv_nsec))
          needs_rebuild = 1;
#endif
      }
    }
  }

  /* pas de rebuild nécessaire */
  if (!needs_rebuild) { // cible à jour
    if (!needed_by) {
      printf("minimake: '%s' is up to date.\n", target);
      fflush(stdout);
    }
    if (*nb_visited > 0)
      (*nb_visited)--;
    char **tmp_b = realloc(*built, sizeof(char *) * (*nb_built + 1));
    if (tmp_b) {
      *built = tmp_b;
      (*built)[(*nb_built)++] = target;
    }
    return 0; // pas de prblm
  }

  /* pas de recette */
  if (r->nb_recipe == 0) {
    if (*nb_visited > 0)
      (*nb_visited)--;
    char **tmp_b = realloc(*built, sizeof(char *) * (*nb_built + 1));
    if (tmp_b) {
      *built = tmp_b;
      (*built)[(*nb_built)++] = target;
    }
    return 1; // signale qu'il faut reconstruire
  }

  /* Preparation a l' installation des var */
  var *local_var_tab =
      malloc(sizeof(var) * (data.nb_var + 3)); // Gérer $@, $<, $^
  if (!local_var_tab) {
    if (*nb_visited > 0)
      (*nb_visited)--;
    return 2;
  }
  for (int i = 0; i < data.nb_var; i++)
    local_var_tab[i] = data.var_tab[i]; // Récup variables globales

  local_var_tab[data.nb_var].label = "@";   // $@
  local_var_tab[data.nb_var].data = target; // Cible actuelle

  local_var_tab[data.nb_var + 1].label = "<"; // $<
  local_var_tab[data.nb_var + 1].data =
      (r->dep && r->dep[0]) ? r->dep[0] : ""; // première dépendance

  size_t deps_len = 1;
  if (r->dep) { // Si dep existent
    for (int i = 0; r->dep[i]; i++)
      deps_len += strlen(r->dep[i]) + 1; // taille total des dep
  }
  char *all_deps = malloc(deps_len);
  if (!all_deps) {
    free(local_var_tab);
    if (*nb_visited > 0)
      (*nb_visited)--;
    return 2;
  }
  all_deps[0] = '\0';
  if (r->dep) {
    for (int i = 0; r->dep[i]; i++) {
      if (i > 0)
        strcat(all_deps, " ");
      strcat(all_deps, r->dep[i]); // Concatène le nom de la dépendance
    }
  }
  local_var_tab[data.nb_var + 2].label = "^";     // $^
  local_var_tab[data.nb_var + 2].data = all_deps; // toutes les dep

  char **exp =
      malloc(sizeof(char *) *
             (r->nb_recipe + 1)); // prepare le tableau des recettes expansées
  if (!exp) {                     // echec
    free(all_deps);
    free(local_var_tab);
    if (*nb_visited > 0)
      (*nb_visited)--;
    return 2;
  }
  for (int i = 0; i < r->nb_recipe; i++) { // Parcourt all commands
    exp[i] = expand_vars(r->recipe[i], local_var_tab,
                         data.nb_var + 3); // replace var
    if (!exp[i]) {                         // expansion fail
      for (int m = 0; m < i; m++)          // free lines deja expanded
        free(exp[m]);                      // free
      free(exp);                           // free array
      free(all_deps);
      free(local_var_tab);
      if (*nb_visited > 0)
        (*nb_visited)--;
      return 2;
    }
  }
  exp[r->nb_recipe] = NULL; // Add null a la fin

  /* Exe recettes */
  int ret = execute_recipe(exp, target); // executer commands

  /*
  Ma santé mentale étant plus que dégradée, je vais dodo,
  je sais que demain je comprendrais plus rien à ce que j'ai fais mais bon au
  bout d'un moment... Je comprends pas deja les trucs aux moments ou je l'écris.
  */

  /* ça librère la mémoire, c'est clair nan ? y a 3 free wesh */
  for (int i = 0; i < r->nb_recipe; i++)
    free(exp[i]);
  free(exp);
  free(all_deps);
  free(local_var_tab);

  if (*nb_visited > 0)
    (*nb_visited)--;

  if (ret == 0) { // si c'est good, c'est marqué
    char **tmp_b = realloc(*built, sizeof(char *) * (*nb_built + 1));
    if (tmp_b) {
      *built = tmp_b;
      (*built)[(*nb_built)++] = target;
    }
  }

  return (ret == 0) ? 1 : 2; // PAPA J'AI CONSTRUIT LA CIBLE
}

/*
C'est la que main.c vient faire toctoc
 */
int build_target(makefile_data data, char *target, char *needed_by) {
  char **visited = NULL;
  int nb_visited = 0;
  char **built = NULL;
  int nb_built = 0;
  int ret = build_target_internal(data, target, needed_by, &visited,
                                  &nb_visited, &built, &nb_built);
  free(visited);
  free(built);
  return ret;
}

/*
Si qqn met -p

Je vais pas commenter ça me saoule
*/
void print_data(makefile_data data) {

  printf("# variables\n");
  for (int i = 0; i < data.nb_var; i++)
    printf("'%s' = '%s'\n", data.var_tab[i].label, data.var_tab[i].data);

  printf("\n# rules\n");
  for (int i = 0; i < data.nb_rules; i++) {

    printf("(%s):", data.rules[i].target);
    if (data.rules[i].dep)
      for (int j = 0; data.rules[i].dep[j]; j++)
        printf(" [%s]", data.rules[i].dep[j]);
    printf("\n");
    for (int j = 0; j < data.rules[i].nb_recipe; j++)
      printf("\t'%s'\n", data.rules[i].recipe[j]);
  }
}