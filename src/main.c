#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

    //variables indiquant si les options sont activées
    int phelper = 0;
    int pprint = 0;
    int pfile = 0;

    int farg_index = -1; //stocke la position de l'argument nécessaire à -f
    int targets_index[1024]; //stocke les positions des cibles
    
    int k = 0; //permet le parcourt de targets_index

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
            pprint = 1;
        else if (strcmp(argv[i], "-h") == 0)
            phelper = 1;
        else if (strcmp(argv[i], "-f") == 0) {
            pfile = 1;
            if (i+1 == argc) {
                printf("Un argument est attendu pour le paramètre -f.");
                return 1;
            }
            farg_index = i + 1;
            i++;
        } else {
            targets_index[k] = i;
            k++;
        }
    }
}