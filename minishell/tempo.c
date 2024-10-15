#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define PAGESIZE 4096
#define NUM_PAGES 3

int main() {
  // Créer un fichier temporaire
  char *tmp_file = "tmp.txt";
  int fd = open(tmp_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  // Écrire trois pages de 'a' dans le fichier temporaire
  char *buf = malloc(PAGESIZE * NUM_PAGES);
  memset(buf, 'a', PAGESIZE * NUM_PAGES);
  if (write(fd, buf, PAGESIZE * NUM_PAGES) != PAGESIZE * NUM_PAGES) {
    perror("write");
    exit(1);
  }
  free(buf);

  // Créer un processus fils
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }

  // Processus père
  if (pid > 0) {
    // Mapper un segment de taille 3 pages au fichier temporaire en mode partagé
    char *shared_mem = mmap(NULL, PAGESIZE * NUM_PAGES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }

    // Attendre 2 secondes
    sleep(2);

    // Remplir les pages 1 et 2 de 'b'
    memset(shared_mem, 'b', PAGESIZE * 2);

    // Attendre 6 secondes
    sleep(6);

    // Afficher le premier caractère des pages 1 et 2
    printf("Père : Page 1 : %c\n", shared_mem[0]);
    printf("Père : Page 2 : %c\n", shared_mem[PAGESIZE]);

    // Remplir la page 2 de 'c'
    memset(shared_mem + PAGESIZE, 'c', PAGESIZE);

    // Démapper le segment partagé
    if (munmap(shared_mem, PAGESIZE * NUM_PAGES) == -1) {
      perror("munmap");
      exit(1);
    }

    // Fermer le fichier temporaire
    if (close(fd) == -1) {
      perror("close");
      exit(1);
    }

    // Terminer
    printf("Père : Terminé\n");
  }

  // Processus fils
  else {
    // Mapper un segment de taille 3 pages au fichier temporaire en mode privé
    char *private_mem = mmap(NULL, PAGESIZE * NUM_PAGES, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (private_mem == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }

    // Afficher le premier caractère de la page 1
    printf("Fils : Page 1 : %c\n", private_mem[0]);

    // Attendre 4 secondes
    sleep(4);

    // Afficher le premier caractère de chacune des pages 1, 2 et 3
    printf("Fils : Page 1 : %c\n", private_mem[0]);
    printf("Fils : Page 2 : %c\n", private_mem[PAGESIZE]);
    printf("Fils : Page 3 : %c\n", private_mem[PAGESIZE * 2]);

    // Remplir la page 2 de 'd'
    memset(private_mem + PAGESIZE, 'd', PAGESIZE);

    // Attendre 8 secondes
    sleep(8);

    // Afficher à nouveau le premier caractère de chacune des pages 1, 2 et 3
    printf("Fils : Page 1 : %c\n", private_mem[0]);
    printf("Fils : Page 2 : %c\n", private_mem[PAGESIZE]);
    printf("Fils : Page 3 : %c\n", private_mem[PAGESIZE * 2]);

    // Démapper le segment privé
    if (munmap(private_mem, PAGESIZE * NUM_PAGES) == -1) {
      perror("munmap");
      exit(1);
    }

    // Fermer le fichier temporaire
    if (close(fd) == -1) {
      perror("close");
      exit(1);
    }

    // Terminer
    printf("Fils : Terminé\n");
  }

  return 0;
}
