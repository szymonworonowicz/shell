#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

void splittoTask(char *tasks, int *countoftasks, char *pipes[100], int *redirectcount, int *backgroundcount)
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
void pipeline(char ***cmd, int redirect, int countoftasks)
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

            if (execvp((*cmd)[0], *cmd) == -1)
            {
                fprintf(stderr, "%s \n", strerror(errno));
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
void printpath()
{
    char path[128];
    if (getcwd(path, sizeof(path)) == NULL)
    {
        fprintf(stderr, "blad dostepu do pliku");
    }
    printf("%s: ", path);
}

char *generatehistorypath()
{
    char *user = getenv("USER");
    if (user == NULL)
    {
        fprintf(stderr, "blad pobrania nazwy uzytkownika");
        return NULL;
    }
    char *arg = (char *)malloc(sizeof(char) * 25);
    strcat(arg, "/home/");
    strcat(arg, user);
    strcat(arg, "/history.txt");
    return arg;
}
void handler(int sig)
{
    char *path = generatehistorypath();
    if (path == NULL)
    {
        return;
    }
    printf("\n");
    char *task[] = {"tail", "-20", path, NULL};
    char **cmd[] = {task, NULL};
    pipeline(cmd, 0, 0);
    printpath();
}
int parse(char *line, char **argv)
{
    int count = 0;
    while (*line != '\0')
    {
        while (*line == ' ' || *line == '\t' || *line == '\"' || *line == '\n')
            *line++ = '\0';
        if (strcmp(line, "") == 0)
            break;
        *argv++ = line;
        count++;
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n' && *line != '\"')
            line++;
    }
    *argv = '\0';

    return count;
}
void pipelineBackground(char ***cmd, int redirect, int countoftasks)
{
    pid_t pid;

    if ((pid = fork()) == -1)
    {
        fprintf(stderr, "%s \n", strerror(errno));
        return;
    }
    else if (pid == 0)
    {
        pipeline(cmd, redirect, countoftasks);
    }
    else
    {
        printf("%s %d \n", "uruchomiono w procesie", pid);
    }
}
void writehistory(char *line)
{
    FILE *fp;
    char *path = malloc(25 * sizeof(char));
    strcpy(path, generatehistorypath());
    fp = fopen(path, "a");

    if (fp == NULL)
    {
        fprintf(stderr, "%s \n", strerror(errno));
        return;
    }

    fprintf(fp, "%s", line);

    if (fclose(fp) == EOF)
    {
        fprintf(stderr, "%s \n", strerror(errno));
    }
}
int main(int argc, char *argv[])
{
    if (signal(SIGQUIT, handler) == SIG_ERR)
    {
        fprintf(stderr, "%s \n", strerror(errno));
    }
    char bufor[256];
    char *task;
    int countoftasks, redirect = 0, background = 0;
    char *pipes[100];
    if (argc == 2)
    {
        FILE *fp;
        task = calloc(256, sizeof(char));
        if ((fp = fopen(argv[1], "r")) == NULL)
        {
            perror("blad odczytu pliku");
            exit(EXIT_FAILURE);
        }
        fgets(task, 256, fp);
        if (task == NULL)
        {
            fprintf(stderr, "%s \n", "blad odczytu");
        }
        task = calloc(256, sizeof(char));
        while (fgets(task, 256, fp))
        {
            if (task == NULL)
                exit(EXIT_SUCCESS);
            if (strcmp(task, "\n") == 0)
                continue;
            writehistory(task);
            splittoTask(task, &countoftasks, &pipes, &redirect, &background);
            int i = 0, cdflag = 0;
            char **cmda[countoftasks + 1];

            for (i = 0; i < countoftasks; i++)
            {

                char *args[64]; 
                int cmdcounts = parse(pipes[i], args);
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
                pipeline(cmda, redirect, countoftasks);
            }
            else if (cdflag == 0 && background == 1)
            {
                cmda[i] = NULL;
                pipelineBackground(cmda, redirect, countoftasks);
            }
            task = calloc(256, sizeof(char));
        }
        if (fclose(fp) == EOF)
        {
            fprintf(stderr, "%s \n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        free(task);
    }
    else if (argc == 1)
    {
        printpath();

        while ((task = fgets(bufor, sizeof(bufor), stdin)) != 0)
        {
            writehistory(task);
            if (strcmp(task, "exit\n") == 0)
            {
                break;
            }
            splittoTask(task, &countoftasks, &pipes, &redirect, &background);
            int i = 0, cdflag = 0;
            char **cmda[countoftasks + 1];
            for (i = 0; i < countoftasks; i++)
            {

                char *args[64];
                int cmdcounts = parse(pipes[i], args);
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
                pipeline(cmda, redirect, countoftasks);
                
            }
            else if (cdflag == 0 && background == 1)
            {
                cmda[i] = NULL;
                pipelineBackground(cmda, redirect, countoftasks);
                
            }
            printpath();
            
        }
    }
    exit(EXIT_SUCCESS);
}