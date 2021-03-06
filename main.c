#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#define TASKLEN 256

char *history[20];
int historycounter;

void splittoTask(char *tasks, int *countoftasks, char **pipes, int *redirectcount, int *backgroundcount)
{
    char *pipesep = "|";
    char *redirectsep = ">>";
    char *backgroundseparator = "&";
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
        if (pipes[*countoftasks] != NULL)
        {
            (*countoftasks)++;
            (*redirectcount) = 1;
        }
        else
        {
            (*redirectcount) = 0;
        }
    }
    char *background = strtok(pipes[*countoftasks - 1], backgroundseparator);
    if (background != NULL)
    {
        pipes[*countoftasks - 1] = background;
        char *sep = strtok(NULL, backgroundseparator);
        if (sep != NULL)
        {
            (*backgroundcount) = 1;
        }
        else
        {
            (*backgroundcount) = 0;
        }
    }
}
void pipeline(char ***cmd, int redirect)
{
    int fd[2];
    pid_t pid;
    int fdd = 0;
    sigmask(0);
    int file;
    while (*cmd != NULL)
    {
        if (pipe(fd) == -1)
        {
            fprintf(stderr, "%s \n", strerror(errno));
            return;
        }
        if ((pid = fork()) == -1)
        {
            fprintf(stderr, "%s \n", strerror(errno));
            return;
        }
        else if (pid == 0)
        {
            if (dup2(fdd, 0) == -1)
            {
                fprintf(stderr, "%s \n", strerror(errno));
                _exit(EXIT_FAILURE);
            }

            if (*(cmd + 1) != NULL)
            {
                if (dup2(fd[1], STDOUT_FILENO) == -1)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }
            if (redirect == 1 && *(cmd + 2) == NULL)
            {
                if ((file = open(*(cmd + 1)[0], O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
                if (dup2(file, 1) == -1)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                    _exit(EXIT_FAILURE);
                }

                if (close(file) == -1)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (close(fd[0]) == -1)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }
            if (!(*(cmd + 1) == NULL && redirect == 1))
            {
                if (execvp((*cmd)[0], *cmd) == -1)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                }
            }

            _exit(EXIT_FAILURE);
        }
        else
        {
            if (wait(NULL) == -1)
            {
                fprintf(stderr, "%s \n", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            if (close(fd[1]) == -1)
            {
                fprintf(stderr, "%s \n", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            fdd = fd[0];
            cmd++;
        }
    }
}
void printPath()
{
    char path[128];
    if (getcwd(path, sizeof(path)) == NULL)
    {
        fprintf(stderr, "blad dostepu do pliku");
    }
    printf("%s: ", path);
}

void generateHistoryPath(char **arg)
{
    char *user = getenv("USER");
    if (user == NULL)
    {
        fprintf(stderr, "blad pobrania nazwy uzytkownika");
        return;
    }
    strcat(*arg, "/home/");
    strcat(*arg, user);
    strcat(*arg, "/history.txt");
}
void handler(int sig)
{
    int i =0;
    printf("\n");
    for(i=0;i<20&& history[i]!=NULL;i++)
    {
        printf("%s",history[i]);
    }
    for(i=0;i<historycounter;i++)
    {
        printf("%s",history[i]);
    }
}
void parse(char *line, char **argv)
{
    while (*line != '\0')
    {
        while (*line == ' ' || *line == '\t' || *line == '\"' || *line == '\n')
            *line++ = '\0';
        if (strcmp(line, "") == 0)
            break;
        *argv++ = line;
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n' && *line != '\"')
            line++;
    }
    *argv = '\0';
}
void pipelineBackground(char ***cmd, int redirect)
{
    pid_t pid;

    if ((pid = fork()) == -1)
    {
        fprintf(stderr, "%s \n", strerror(errno));
        return;
    }
    else if (pid == 0)
    {
        pipeline(cmd, redirect);
    }
    else
    {
        printf("%s %d \n", "uruchomiono w procesie", pid);
    }
}
void writeHistory()
{
    FILE *fp;
    char *path = calloc(25, sizeof(char));
    generateHistoryPath(&path);
    fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "%s \n", strerror(errno));
        return;
    }
    int i;
    for(i=0;i<20 && history[i]!=NULL;i++)
    {
        fprintf(fp,"%s",history[i]);
    }

    if (fclose(fp) == EOF)
    {
        fprintf(stderr, "%s \n", strerror(errno));
    }
    free(path);
}
int getHistory()
{
    char *path = calloc(25, sizeof(char));
    generateHistoryPath(&path);
    char *line = calloc(TASKLEN, sizeof(char));
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "%s \n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fgets(line, TASKLEN, fp);
    int i = 0;
    while (strlen(line) != 0)
    {
        history[i] = calloc(TASKLEN,sizeof(char));
        memcpy(history[i++],line,TASKLEN);
        free(line);
        line = calloc(TASKLEN, sizeof(char));
        fgets(line, TASKLEN, fp);
    }

    if (fclose(fp) == EOF)
    {
        fprintf(stderr, "%s \n", strerror(errno));
    }
    free(path);
    free(line);
    return i;
}
int main(int argc, char *argv[])
{
    if (signal(SIGQUIT, handler) == SIG_ERR)
    {
        fprintf(stderr, "%s \n", strerror(errno));
    }
    historycounter = getHistory();
    char *task;
    int countoftasks, redirect = 0, background = 0;
    char *pipes[100];
    FILE *fp;
    if (argc == 2)
    {
        task = calloc(TASKLEN, sizeof(char));
        fp = fopen(argv[1], "r");
        if (fp == NULL)
        {
            fprintf(stderr, "%s \n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        fgets(task, TASKLEN, fp);
        if (task == NULL)
        {
            fprintf(stderr, "%s \n", "blad odczytu");
        }
        free(task);
    }
    while (1)
    {

        task = calloc(TASKLEN, sizeof(char));
        if (argc == 2)
        {
            fgets(task, TASKLEN, fp);
            if (strlen(task) == 0)
            {
                if (fclose(fp) == EOF)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;
            }
            if (strcmp(task, "\n") == 0)
                continue;
        }
        else if (argc == 1)
        {
            printPath();
            fgets(task, TASKLEN, stdin);
            if (strcmp(task, "exit\n") == 0)
            {
                break;
            }
        }
        if(history[historycounter % 20] != NULL)
        {
            free(history[historycounter % 20]);
        }
        history[historycounter % 20] = calloc(TASKLEN,sizeof(char));
        memcpy(history[historycounter % 20], task, TASKLEN);
        historycounter++;
        splittoTask(task, &countoftasks, pipes, &redirect, &background);
        int i = 0, cdflag = 0;
        char **cmda[countoftasks + 1];

        for (i = 0; i < countoftasks; i++)
        {

            char *args[64];
            parse(pipes[i], args);
            if (strcmp(args[0], "cd") == 0 && args[1] != NULL)
            {
                if (strcmp(args[1], "~") == 0)
                {
                    char *user = getenv("USER");
                    if (user == NULL)
                    {
                        fprintf(stderr, "nie udalo sie pobrac nazwy usera");
                        exit(EXIT_FAILURE);
                    }
                    char home[15] = "/home/";
                    strcat(home, user);

                    strcpy(args[1], home);
                }
                if (chdir(args[1]) == -1)
                {
                    fprintf(stderr, "%s \n", strerror(errno));
                }
                cdflag = 1;
            }
            else
            {
                if (cdflag == 1)
                {
                    fprintf(stderr, "%s \n", "niepoprawne polecenie");
                }
                cmda[i] = (char **)malloc(sizeof(char *) * 64);
                memcpy(cmda[i], args, sizeof(args));
            }
        }
        if (cdflag == 0 && background == 0)
        {
            cmda[i] = NULL;
            pipeline(cmda, redirect);
        }
        else if (cdflag == 0 && background == 1)
        {
            cmda[i] = NULL;
            pipelineBackground(cmda, redirect);
        }
        if (cdflag == 0)
        {
            for (i = 0; i < countoftasks; i++)
            {
                free(cmda[i]);
            }
        }

        free(task);
    }
    free(task);
    writeHistory();
    int i;
    for(i=0;i<20 && history[i]!= NULL;i++)
    {
        free(history[i]);
    }
    exit(EXIT_SUCCESS);
}
