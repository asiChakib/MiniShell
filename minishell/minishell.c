#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "readcmd.h"

void masquer_sig_principal() {
    sigset_t masque;
    sigemptyset(&masque);
    sigaddset(&masque, SIGINT);
    sigaddset(&masque, SIGTSTP);
    sigprocmask(SIG_BLOCK, &masque, NULL);
}

void demasquer_signaux_enfant() {
    sigset_t masque;
    sigemptyset(&masque);
    sigaddset(&masque, SIGINT);
    sigaddset(&masque, SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &masque, NULL);
}

void sigchld_handler(int signum __attribute__((unused))) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            printf("Le processus fils %d s'est terminé avec le code %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Le processus fils %d a été tué par le signal %d\n", pid, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("Le processus fils %d a été suspendu par le signal %d\n", pid, WSTOPSIG(status));
        } else if (WIFCONTINUED(status)) {
            printf("Le processus fils %d a été repris\n", pid);
        }
    }
}

void sigint_handler(int signum __attribute__((unused))) {
    printf("\nReceived SIGINT (Ctrl-C)\n> ");
    fflush(stdout);
}

void sigtstp_handler(int signum __attribute__((unused))) {
    printf("\nReceived SIGTSTP (Ctrl-Z)\n> ");
    fflush(stdout);
}

void setup_sig_hdl1() {
    struct sigaction sa;

    // SIGINT
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // SIGTSTP
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTSTP, &sa, NULL);

    //SIGCHLD
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);
}

void setup_sig_hdl2() {
    struct sigaction sa;

    // SIGINT
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // SIGTSTP
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTSTP, &sa, NULL);

    // SIGCHLD
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);
}


void setup_sig_hdl3() {
    struct sigaction sa;
    // SIGCHLD
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);
}

void executerCommande(char **cmd, bool tacheFond, char* in, char* out) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Erreur fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        demasquer_signaux_enfant();
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if (tacheFond) {
            setpgrp();
        }

        // Redirections d'entrée et de sortie
        if (in != NULL) {
            if (dup2(open(in, O_RDONLY), STDIN_FILENO) == -1) {
                perror("Erreur de redirection d'entrée");
                exit(EXIT_FAILURE);
            }
        }
        if (out != NULL) {
            if (dup2(open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666), STDOUT_FILENO) == -1) {
                perror("Erreur de redirection de sortie");
                exit(EXIT_FAILURE);
            }
        }

        if (execvp(cmd[0], cmd) == -1) {
            perror("Erreur execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        if (!tacheFond) {
            waitpid(pid, NULL, 0);
        }
    }
}

void executerCmdPipe(char ***cmd) {
    int p[2];
    int fd_in = 0; 
    pid_t pid;

    while (*cmd != NULL) {
        pipe(p);
        if ((pid = fork()) == -1) {
            perror("Erreur fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // fils
            dup2(fd_in, 0); // Change l'entrée du processus courant
            if (*(cmd + 1) != NULL) {
                dup2(p[1], 1); //Change la sortie du processus si ce n'est pas la dernière commande
            }
            close(p[0]);
            if (execvp((*cmd)[0], *cmd) == -1) {
                perror("Erreur execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent
            wait(NULL); // Attente de la fin du processus enfant
            close(p[1]);
            fd_in = p[0]; // Sauvegarde l'entrée pour la prochaine commande
            cmd++;
        }
    }
}

int main(void) {
    // Choisissez une méthode de gestion des signaux ici
    //setup_sig_hdl1();
    //setup_sig_hdl2();
    //masquer_signaux_principal();
    //setup_sig_hdl3();

    bool fini = false;

    while (!fini) {
        printf("> ");
        struct cmdline *commande = readcmd();
        if (commande == NULL) {
            perror("Erreur lecture commande \n");
            continue;
        }

        if (commande->err) {
            printf("Erreur saisie de la commande : %s\n", commande->err);
        } else {
            bool tacheFond = commande->backgrounded != NULL;

            if (commande->seq[1] == NULL) {
                // Pas de tube, exécute la commande normalement
                for (int indexseq = 0; commande->seq[indexseq] != NULL; indexseq++) {
                    char **cmd = commande->seq[indexseq];
                    if (cmd[0]) {
                        if (strcmp(cmd[0], "exit") == 0) {
                            fini = true;
                            printf("Au revoir ...\n");
                            break;
                        } else {
                            executerCommande(cmd, tacheFond, commande->in, commande->out);
                        }
                    }
                }
            } else {
                // Utilise les tubes pour gérer les pipelines
                executerCmdPipe(commande->seq);
            }
        }
    }
    return EXIT_SUCCESS;
}
