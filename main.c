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
        if (countoftasks == 1)
        {
            char *one[2];
            one[0] = strtok(pipes[i], " ");
            one[1] = strtok(NULL, " ");
            one[1] = (one[1] == NULL) ? " " : one[1];
            pid_t pid;

            pid = fork();
            if (pid == 1)
            {
                perror("fork bad");
                _exit(EXIT_FAILURE);
            }
            else if (pid > 0)
            {
                execlp(one[0], one[0], one[1], NULL);
            }
        }
        for (i = 0; i < countoftasks - 1; i++)
        {
            char *one[2];
            char *two[2];
            one[0] = strtok(pipes[i], " ");
            one[1] = strtok(NULL, " ");
            two[0] = strtok(pipes[i + 1], " ");
            two[1] = strtok(NULL, " ");
            pid_t parent;
            parent = fork();
            if (parent > 0)
            {
                pid_t pid;
                int fps[2];
                pipe(fps);
                pid = fork();

                if (pid == -1)
                {
                    perror("fork bad");
                    _exit(EXIT_FAILURE);
                }
                else if (pid > 0)
                {
                    close(fps[0]);
                    dup2(fps[1], 1);
                    if (one[1] != NULL)
                        execlp(one[0], one[0], one[1], NULL);
                    else
                        execlp(one[0], one[0], NULL);
                }
                else
                {
                    close(fps[1]);
                    dup2(fps[0], 0);
                    if (two[1] != NULL)
                        execlp(two[0], two[0], two[1], NULL);
                    else
                        execlp(two[0], two[0], NULL);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}