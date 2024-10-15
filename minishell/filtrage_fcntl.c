#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

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
            printf("Commande invalide: %c\n", cde);
    }
}

int main(int argc, char *argv[]) {
    int p[2];
    pid_t pid;
    int d, nlus;
    char buf[BUFSIZE];
    char commande = 'R'; /* mode normal */
    fd_set readfds;
    struct timeval timeout;

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

    if (pid == 0) {  /* fils  */
        d = open(argv[1], O_RDONLY);
        if (d == -1) {
            fprintf(stderr, "Impossible d'ouvrir le fichier ");
            perror(argv[1]);
            exit(4);
        }

        close(p[0]); /* pour finir malgré tout, avec sigpipe */
        while (true) {
            while ((nlus = read(d, buf, BUFSIZE)) > 0) {
                write(p[1], buf, nlus);
                sleep(2); // Temporisation pour visualiser le découplage
            }
            sleep(2); 
            printf("Le fichier est lu en entier, retour au début...\n");
            lseek(d, (off_t) 0, SEEK_SET);
        }
    } else {  /* pere */
        close(p[1]);

        // Rendre l'entrée standard non bloquante
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        // Rendre la lecture du tube non bloquante
        flags = fcntl(p[0], F_GETFL, 0);
        fcntl(p[0], F_SETFL, flags | O_NONBLOCK);

        while (true) {
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(p[0], &readfds);

            timeout.tv_sec = 1; // Attente de 1 seconde
            timeout.tv_usec = 0;

            int ret = select(p[0] + 1, &readfds, NULL, NULL, &timeout);
            if (ret == -1) {
                perror("select");
                exit(5);
            } else if (ret > 0) {
                if (FD_ISSET(STDIN_FILENO, &readfds)) {
                    read(STDIN_FILENO, &commande, sizeof(char));
                    printf("Commande reçue: %c\n", commande);
                }
                if (FD_ISSET(p[0], &readfds)) {
                    bzero(buf, BUFSIZE);
                    nlus = read(p[0], buf, BUFSIZE);
                    if (nlus > 0) {
                        traiter(buf, commande, nlus);
                    } 
                }
            } else {
                 // Timeout : aucune donnée disponible pour le moment
            }
        }
    }
    return 0;
}
