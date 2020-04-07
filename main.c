#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void handler(int sig)
{
    FILE *fp;
    char c;
    fp = fopen("history", "r");

    if (fp != NULL)
    {
        while ((c = getc(fp)) != EOF)
            printf("%c", c);
    }
    else
    {
        fprintf(stderr, "%s \n", "Błąd odczytu histori");
        exit(EXIT_FAILURE);
    }

    if (fclose(fp) != EOF)
    {
        fprintf(stderr, "%s\n", "Blad zamkniecia pliku historia");
        exit(EXIT_FAILURE);
    }
}

int splittoTask(char *tasks, int *countoftasks, char *pipes[100])
{
    char *pipesep = "|";
    char *redirectsep = ">>";
    *countoftasks = 0;
    pipes[*countoftasks] = strtok(tasks, pipesep);
    while (pipes[*countoftasks] != NULL)
    {
        pipes[++*countoftasks] = strtok(NULL, pipesep);
    }
    char *redirect = strtok(pipes[*countoftasks - 1], redirectsep);
    if (redirect != NULL)
    {
        pipes[*countoftasks - 1] = redirect;
        pipes[*countoftasks] = strtok(NULL, redirectsep);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    signal(SIGQUIT, handler);
    char bufor[1024];
    char *task;
    int countoftasks;
    char *pipes[100];
    while ((task = fgets(bufor, sizeof(bufor), stdin)) != 0)
    {
        int redirect = splittoTask(task, &countoftasks, &pipes);
        int i = 0;
        for (i = 0; i < countoftasks - 1; i++)
        {
            char *one[2];
            char *two[2];
            one[0] = strtok(pipes[i], " ");
            one[1] = strtok(NULL, " ");
            two[0] = strtok(pipes[i + 1], " ");
            two[1] = strtok(NULL, " ");

            pid_t pid;

            pid = fork();

            if (pid == 0)
            {
                int fds1[2];
                int fds2[2];
                pipe(fds1);
                pipe(fds2);
                
                pid_t pid2 = fork();

                if(pid2 == 0)
                {
                    dup2(fds1[1],1);
                    dup2(fds2[0],0);
                    close(fds1[0]);
                    close(fds2[1]);
                    //execlp(one[0], one[0], one[1], NULL);
                }

                // dup2(fds2[0], STDIN_FILENO);
                // dup2(fds2[1], STDOUT_FILENO);
                execlp(two[0], two[0], two[1], NULL);
            }
            // else
            // {

            //     close(fds1[0]);

            //     FILE *stream = fdopen(fds1[1], "w");

            //     pid_t pid1 = fork();
            //     if (pid1 == 0)
            //     {
            //         dup2(fds1[1], STDOUT_FILENO);
            //         execlp(one[0], one[0], one[1], NULL);
            //     }

            //     close(fds1[1]);

            //     waitpid(pid, NULL, 0);
            // }
        }
    }
}