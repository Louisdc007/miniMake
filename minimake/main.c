#include "exe.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
Bon c pour -h sérieux t'avais aps compris ?
 */
static void print_help(void) {
  printf("Usage: minimake [OPTIONS] [TARGETS]\n");
  printf("    OPTIONS can be:\n");
  printf("        \"-h\"            to display the helper\n");
  printf("        \"-p\"            to print the rules and variables\n");
  printf("        \"-f\" <filename> to change the make config file, "
         "\"Makefile\" by default\n");
  printf("    TARGETS are target name of makefile rule to execute\n");
}

/*
J'aurai du etre fleuriste
 */
int main(int argc, char **argv) {
  char *makefile_name = "Makefile";
  char **targets = NULL;
  int nb_targets = 0;
  int print_mode = 0;

  /*
  -h en prio
   */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0) {
      i++;
    } else if (strcmp(argv[i], "-h") == 0) {
      print_help(); // -h
      return 0;     // Quitte le programme
    }
  }

  /*
  bon la on comprend quoi qu'il fait le monsieur avec le programme
   */
  for (int i = 1; i < argc; i++) {    // on reparcour touuuuuuuu
    if (strcmp(argv[i], "-f") == 0) { // -f
      if (i + 1 >= argc) { // est ce que le -f a son copain qui le suit
        printf(
            "minimake: option '-f' requires an argument\n"); // la honte j'ai
                                                             // utilisé google
                                                             // trad pour mes
                                                             // textes et mes
                                                             // noms de
                                                             // variables
        fflush(stdout);
        return 2; // erreur
      }
      makefile_name = argv[++i];             // recup nom fichier makefile
    } else if (strcmp(argv[i], "-p") == 0) { // le -p
      print_mode = 1;
    } else {
      // c'est donc une cible
      char **tmp = realloc(targets, sizeof(char *) * (nb_targets + 1));
      if (!tmp) {
        free(targets);
        return 2;
      }
      targets = tmp;
      targets[nb_targets++] = argv[i];
    }
  }

  /*
On vérifie que le user il invente aps des fichiers qui existent pas
   */
  struct stat st;
  if (stat(makefile_name, &st) == -1) {
    printf("minimake: %s: No such file or directory\n", makefile_name);
    fflush(stdout);
    free(targets);
    return 3;
  }

  /*
la on décortique le bazard
   */
  makefile_data data = parse_makefile(makefile_name);
  if (data.nb_rules < 0) {
    free(targets);
    return 2;
  }

  /*
le -p
   */
  if (print_mode) {
    print_data(data);
    free_makefile_data(data);
    free(targets);
    return 0;
  }

  /*
si y a pas de règles...
   */
  if (data.nb_rules == 0) {
    printf("minimake: *** No targets.  Stop.\n");
    fflush(stdout);
    free_makefile_data(data);
    free(targets);
    return 2;
  }

  /*
pas de cible = la premiere
   */
  if (nb_targets == 0) {
    targets = malloc(sizeof(char *));
    if (!targets) {
      free_makefile_data(data);
      return 2;
    }
    targets[0] = data.rules[0].target;
    nb_targets = 1;
  }

  /*
on construit les commandes et on les exe
   */
  int overall = 0; // on va stocker les errurs la
  for (int i = 0; i < nb_targets; i++) {
    int r = build_target(data, targets[i], NULL);
    if (r == 2)
      overall = 2;
  }

  free_makefile_data(data);
  free(targets);
  return overall;
}

// the end apres bcp trop de temps et de pleurs