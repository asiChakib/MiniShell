#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/select.h>

#define BUFSIZE 512

void traiter(char tampon[], char cde, int nb) {
    int i;
    switch (cde) {
    case 'X':
        break;
    case 'Q':
        exit(0);
        break;
    case 'R':
        write(1, tampon, nb);
        break;
    case 'M':
        for (i = 0; i < nb; i++) {
            tampon[i] = toupper(tampon[i]);
        }
        write(1, tampon, nb);
        break;
    case 'm':
        for (i = 0; i < nb; i++) {
            tampon[i] = tolower(tampon[i]);
        }
        write(1, tampon, nb);
        break;
    default:
        printf("????\n");
    }
}

int main(int argc, char *argv[]) {
    int p[2];
    pid_t pid;
    int d, nlus;
    char buf[BUFSIZE];
    char commande = 'R'; /* mode normal */
    fd_set rfds;
    int maxfd;

    if (argc != 2) {
        printf("utilisation : %s <fichier source>\n", argv[0]);
        exit(1);
    }

    if (pipe(p) == -1) {
        perror("pipe");
        exit(2);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(3);
    }
    if (pid == 0) {   /* fils  */
        d = open(argv[1], O_RDONLY);
        if (d == -1) {
            fprintf(stderr, "Impossible d'ouvrir le fichier ");
            perror(argv[1]);
            exit(4);
        }

        close(p[0]); /* pour finir malgré tout, avec sigpipe */
        while (true) {
            while ((nlus = read(d, buf, BUFSIZE)) > 0) {
                /* read peut lire moins que le nombre d'octets demandes, en
                 * particulier lorsque la fin du fichier est atteinte. */
                write(p[1], buf, nlus);
                sleep(5);
            }
            sleep(5);
            printf("on recommence...\n");
            lseek(d, (off_t)0, SEEK_SET);
        }

    } else {   /* père */
        close(p[1]);
        system("stty -icanon min 1"); // saisie entrées clavier sans tampon

        /* a completer */

        while (true) {
            FD_ZERO(&rfds);
            FD_SET(0, &rfds); // Descripteur de fichier standard d'entrée
            FD_SET(p[0], &rfds); // Descripteur de fichier du pipe

            maxfd = (p[0] > 0) ? p[0] : 0; // Trouver le descripteur de fichier maximum

            if (select(maxfd + 1, &rfds, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(5);
            }

            if (FD_ISSET(0, &rfds)) { // Entrée clavier prête
                read(0, &commande, sizeof(char));
                printf("-->%c\n", commande);
            }

            if (FD_ISSET(p[0], &rfds)) { // Données du pipe prêtes
                bzero(buf, BUFSIZE); // Nettoyage
                if ((nlus = read(p[0], buf, BUFSIZE)) > 0) {
                    traiter(buf, commande, nlus);
                }
            }

            sleep(1);
        }
    }
    return 0;
}
