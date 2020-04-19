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
    int fdd = 0; /* Backup */
    sigmask(0);
    int file;
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
            dup2(fdd, 0);

            if (*(cmd + 1) != NULL)
            {
                dup2(fd[1], STDOUT_FILENO);
            }
            if (redirect == 1 && *(cmd + 2) == NULL)
            {
                if ((file = open(*(cmd + 1)[0], O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0)
                {
                    perror("open error");
                    //return -1;
                }

                dup2(file, 1);
                close(file);
            }
            else
            {
                close(fd[0]);
            }

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
void printpath()
{
    char path[128];
    getcwd(path, sizeof(path));
    printf("%s: ", path);
}

char *generatehistorypath()
{
    char *user = getenv("USER");
    char *arg = (char *)malloc(sizeof(char) * 25);
    strcat(arg, "/home/");
    strcat(arg, user);
    strcat(arg, "/history.txt");
    return arg;
}
void handler(int sig)
{
    char *path = generatehistorypath();
    printf("\n");
    char *task[] = {"tail", "-20", path, NULL};
    char **cmd[] = {task, NULL};
    pipeline(cmd, 0, 0);
}
int parse(char *line, char **argv)
{
    int count = 0;
    while (*line != '\0')
    {
        // if(strcmp(line,"\"\n")==0)
        //     break;
        /* if not the end of line ....... */
        while (*line == ' ' || *line == '\t' || *line == '\"' || *line == '\n') //92 - \\/
            *line++ = '\0';                                                     /* replace white spaces with 0    */
        if (strcmp(line, "") == 0)
            break;
        *argv++ = line;
        count++; /* save the argument position     */
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n' && *line != '\"')
            line++; /* skip the argument until ...    */
    }
    *argv = '\0'; /* mark the end of argument list  */

    return count;
}
void pipelineBackground(char ***cmd, int redirect, int countoftasks)
{
    pid_t pid;

    if ((pid = fork()) == -1)
    {
        perror("fork error");
        exit(EXIT_FAILURE);
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
    char *path = generatehistorypath();
    fp = fopen(path, "a");

    fprintf(fp, "%s", line);

    fclose(fp);
}
int main(int argc, char *argv[])
{
    signal(SIGQUIT, handler);
    char bufor[256];
    char *task;
    int countoftasks, redirect = 0, background = 0;
    char *pipes[100];
    fprintf("%s \n",argv[1],stdout);
    if(argc == 2)
    {
        fprintf("wbilo mnie",stdout);
        sleep(3);
        FILE* fp;
        if((fp = fopen(argv[1],"r"))==NULL)
        {
            perror("blad odczytu pliku");
            exit(EXIT_FAILURE);
        }
        
        while((task = fgets(bufor,sizeof(bufor),fp))!= 0)
        {
            writehistory(task);
            splittoTask(task, &countoftasks, &pipes, &redirect, &background);
            int i = 0, cdflag = 0;
            char **cmda[countoftasks + 1];
            for (i = 0; i < countoftasks; i++)
            {

                char *args[64]; // = (char **)malloc(sizeof(char*)*64);
                int cmdcounts = parse(pipes[i], args);
                if (strcmp(args[0], "cd") == 0 && args[1] != NULL)
                {
                    if (strcmp(args[1], "~") == 0)
                    {
                        char *user = getenv("USER");
                        char arg[15] = "/home/";
                        strcat(arg, user);
                        strcpy(args[1], arg);
                    }
                    if (chdir(args[1]) == -1)
                    {
                        perror("cd blad");
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
        } 
        fclose(fp);
    }
    else if (argc == 1)
    {
        printpath();
        while ((task = fgets(bufor, sizeof(bufor), stdin)) != 0)
        {
            writehistory(task);
            splittoTask(task, &countoftasks, &pipes, &redirect, &background);
            int i = 0, cdflag = 0;
            char **cmda[countoftasks + 1];
            for (i = 0; i < countoftasks; i++)
            {

                char *args[64]; // = (char **)malloc(sizeof(char*)*64);
                int cmdcounts = parse(pipes[i], args);
                if (strcmp(args[0], "cd") == 0 && args[1] != NULL)
                {
                    if (strcmp(args[1], "~") == 0)
                    {
                        char *user = getenv("USER");
                        char arg[15] = "/home/";
                        strcat(arg, user);
                        strcpy(args[1], arg);
                    }
                    if (chdir(args[1]) == -1)
                    {
                        perror("cd blad");
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
}