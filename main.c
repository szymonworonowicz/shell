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
void pipeline(char ***cmd)
{
    int fd[2];
    pid_t pid;
    int fdd = 0; /* Backup */
    sigmask(0);
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
                dup2(fd[1], 1);
            }
            close(fd[0]);
            execvp((*cmd)[0], *cmd);
            exit(1);
        }
        else
        {
            int status;
            //while (wait(&status) != pid)
            wait(NULL); /* Collect childs */
            close(fd[1]);
            fdd = fd[0];
            cmd++;
        }
    }
}
int parse(char *line, char **argv)
{
    int count = 0;
    while (*line != '\0')
    { /* if not the end of line ....... */
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0'; /* replace white spaces with 0    */
        if (strcmp(line, "") == 0)
            break;
        *argv++ = line;
        count++; /* save the argument position     */
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n')
            line++; /* skip the argument until ...    */
    }
    *argv = '\0'; /* mark the end of argument list  */

    return count;
}
void printpath()
{
    char path[128];
    getcwd(path, sizeof(path));
    printf("%s: ", path);
}
int main(int argc, char *argv[])
{
    signal(SIGQUIT, handler);
    char bufor[1024];
    char *task;
    int countoftasks;
    char *pipes[100];
    printpath();
    while ((task = fgets(bufor, sizeof(bufor), stdin)) != 0)
    {
        int redirect = splittoTask(task, &countoftasks, &pipes);
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
                    fprintf(stderr,"%s \n", "niepoprawne polecenie");
                }
                cmda[i] = (char **)malloc(sizeof(char *) * 64);
                memcpy(cmda[i], args, sizeof(args));
            }

            // if (i == countoftasks - 1)
            // {
            //     int j, enter = 0;
            //     for (j = 0; pipes[i][j] != '\0'; j++)
            //     {
            //         if (pipes[i][j] == '\n')
            //         {
            //             enter = 1;
            //             break;
            //         }
            //     }
            //     if (enter == 1)
            //     {
            //         pipes[i][j] = '\0';
            //     }
            // }
            // char *command = strtok(pipes[i], " ");
            // char *args = strtok(NULL, " ");
            // if(strcmp(command,"cd")==0 && args!=NULL)
            // {
            //     if(strcmp(args,"~")==0)
            //     {
            //         char* user = getenv("USER");
            //         char arg[15] = "/home/";
            //         strcat(arg,user);
            //         strcpy(args,arg);
            //     }
            //     if(chdir(args)==-1)
            //     {
            //         perror("cd blad");
            //     }
            // }
            // else if (args == NULL)
            // {
            //     char *comm[] = {command, NULL};
            //     cmda[i] = (char **)malloc(sizeof(char *) * 2);
            //     memcpy(cmda[i], comm, sizeof(comm));
            // }
            // else
            // {
            //     char *comm[] = {command, args, NULL};
            //     cmda[i] = (char **)malloc(sizeof(char *) * 3);
            //     memcpy(cmda[i], comm, sizeof(comm));
            // }
        }
        if (cdflag == 0)
        {
            cmda[i] = NULL;
            pipeline(cmda);
        }
        printpath();
    }
}