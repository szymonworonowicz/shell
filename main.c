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
static void pipeline(char ***cmd)
{
    int fd[2];
    pid_t pid;
    int fdd = 0; /* Backup */

    while (*cmd != NULL)
    {
        pipe(fd);
        if ((pid = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }
        else if (pid == 0)
        {
            umask(0);
            dup2(fdd, 0);
            if (*(cmd + 1) != NULL)
            {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            //execvp((*cmd)[0],(*cmd)[1]);
            execvp((*cmd)[0], *cmd);
            exit(1);
        }
        else
        {
            wait(NULL); /* Collect childs */
            close(fd[1]);
            fdd = fd[0];
            cmd++;
        }
    }
}

int main(int argc, char *argv[])
{
    signal(SIGQUIT, handler);
    char bufor[1024];
    char *task;
    int countoftasks;
    char *pipes[100];
    //https://gist.github.com/iomonad/a66f6e9cfb935dc12c0244c1e48db5c8
    char *ls[] = {"ls", "-al", NULL};
    char *rev[] = {"rev", NULL};
    char *nl[] = {"nl", NULL};
    char *cat[] = {"cat", "-e", NULL};
    char **cmd[] = {ls, rev, nl, cat, NULL};

    pipeline(cmd);

    while ((task = fgets(bufor, sizeof(bufor), stdin)) != 0)
    {
        int redirect = splittoTask(task, &countoftasks, &pipes);
        int i = 0;
        char **cmda[countoftasks + 1];
        for (i = 0; i < countoftasks; i++)
        {
            if (i == countoftasks - 1)
            {
                int j, enter = 0;
                for (j = 0; pipes[i][j] != '\0'; j++)
                {
                    if (pipes[i][j] == '\n')
                    {
                        enter = 1;
                        break;
                    }
                }
                if (enter == 1)
                {
                    pipes[i][j] = '\0';
                }
            }
            char *command = strtok(pipes[i], " ");
            char *args = strtok(NULL, " ");
            if (args == NULL)
            {
                char *comm[] = {command, args};
                *cmda[i] = *comm;
            }
            else
            {
                char *comm[] = {command, args, NULL};
                *cmda[i] = *comm;
            }
        }
        cmda[i] = NULL;
        pipeline(cmda);
    }
}