#include "ramfs.h"
#include "kheap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TODO: à la création d'un fichier, stocker son path complet dans nom (ou dans
// un autre champ)

// NOTE: Un dossier est un fichier.

static file_t *cur_file = NULL;

// Initialise le fichier racine '/' sur le tas kernel et place le pointeur de
// fichiers cur_file sur '/'. Le tas kernel doit donc déjà avoir été initialisé.
void init_ramfs() {
  // Le header du fichier (file_t) est stocké sur le tas kernel (car il doit
  // être accessible depuis n'importe quel processus à partir d'un syscall en
  // mode S), ainsi que ses champs pointeurs (nom et children). Donc les
  // adresses des headers des fichiers sont physiques.
  char *pa_name = (char *)kmalloc(2 * sizeof(char));
  if (pa_name == NULL) {
    printf("init_ramfs error.\n");
    return;
  }
  pa_name[0] = '/';
  pa_name[1] = '\0';

  file_t *pa_root = kmalloc(sizeof(file_t));
  if (pa_root == NULL) {
    printf("init_ramfs error.\n");
    return;
  }
  file_t root = {
      .name = pa_name, .path = pa_name, .nb_children = 0, .children = NULL};
  *(file_t *)pa_root = root;

  pa_root->father = pa_root; // la racine est son propre père

  cur_file = (file_t *)pa_root;
}

// Ajoute le fichier nommé name (=va) aux enfants du fichier courant et renvoie
// le fichier nouvellement créé. S'il y a une erreur, renvoie NULL et ne modifie
// pas l'arborescence.
// WARNING: si mkdir est lancé depuis un processus alors name est une va. Il
// faut donc s'assurer que les mappings user sont transmis au mode S via le SUM
// TODO: Écraser deux fichiers de même nom ?
file_t *mkdir(char *name) {
  char *pa_name = kmalloc(FILENAME_MAXSIZE * sizeof(char));
  if (pa_name == NULL) {
    printf("mkdir error.\n");
    return NULL;
  }
  // on copie name (potentiellement va) sur le tas (pa)
  strncpy(pa_name, name, FILENAME_MAXSIZE);

  char *pa_path = kmalloc(PATH_MAXSIZE * sizeof(char));
  if (pa_path == NULL) {
    printf("mkdir error.\n");
    return NULL;
  }
  // le chemin du nouveau fichier est celui du père du nouveau fichier (donc
  // cur_file) + le nouveau nom + '/'
  // TODO: gérer le cas où c'est tronqué.
  size_t path_maxsize = PATH_MAXSIZE * sizeof(char);
  strncpy(pa_path, cur_file->path, path_maxsize);
  strncat(strncpy(pa_path + strlen(cur_file->path), pa_name, path_maxsize), "/",
          path_maxsize);

  file_t *pa_f = kmalloc(sizeof(file_t));
  if (pa_f == NULL) {
    printf("mkdir error.\n");
    return NULL;
  }
  file_t f = {.name = pa_name,
              .path = pa_path,
              .nb_children = 0,
              .father = cur_file,
              .children = NULL};
  *(file_t *)pa_f = f;

  // On ajoute le nouveau fichier aux enfants du fichier courant
  int new_nbc = cur_file->nb_children + 1;
  // On alloue le fils sur le tas du kernel (pour avoir un tableau contigu)
  file_t **new_children =
      krealloc(cur_file->children, new_nbc * sizeof(file_t *));
  if (new_children == NULL) {
    printf("mkdir error: la créeation du fichier n'a pas eu lieue et rien n'a "
           "été modifié.\n");
    return NULL;
  }
  cur_file->children = new_children;

  cur_file->children[new_nbc - 1] = pa_f;
  cur_file->nb_children = new_nbc;

  return pa_f;
}

// Affiche la liste des enfants du fichier courant.
void ls() {
  for (int i = 0; i < cur_file->nb_children; i++) {
    printf("%s ", cur_file->children[i]->name);
  }
  printf("\n");
}

void pwd() { printf("%s\n", cur_file->path); }

// Supprime le fichier de nom "name" s'il exsite en libérant ses données
// allouées sur le tas.
// TODO: mettre une sécurité pour ne pas supprimer la racine
int rm(char *name) {
  for (int i = 0; i < cur_file->nb_children; i++) {
    if (strcmp(cur_file->children[i]->name, name) == 0) {
      if (cur_file->children[i]->children != NULL) {
        // TODO:
        printf("rm: suppression récursive pas encore prise en charge.\n");
        return 1;
      }

      // on libère tout ce qui a été alloué sur le tas (mais on ne supprime pas
      // le père)
      kfree(cur_file->children[i]->name);
      kfree(cur_file->children[i]->path);
      kfree(cur_file->children[i]);

      // On décale tous les enfants du fichier courant pour combler le trou
      size_t size = (cur_file->nb_children - i - 1) * sizeof(file_t *);
      memmove(cur_file->children + i, cur_file->children + i + 1, size);
      cur_file->nb_children--;

      return 0;
    }
  }

  printf("rm error: le fichier %s n'existe pas.\n", name);
  return 1;
}

int cd(char *path) {
  if (strcmp(path, "..") == 0 || strcmp(path, "../") == 0) {
    cur_file = cur_file->father;
    return 0;
  }

  for (int i = 0; i < cur_file->nb_children; i++) {
    if (strcmp(path, cur_file->children[i]->name) == 0) {
      cur_file = cur_file->children[i];
      return 0;
    }
  }

  printf("cd: %s: Pas de tel fichier ou dossier.\n", path);
  return 1;
}
