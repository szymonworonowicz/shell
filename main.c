#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
void printpath()
{
    char path[128];
    getcwd(path, sizeof(path));
    printf("%s: ", path);
}
void handler(int sig)
{
    char *task[] = {"tail", "-20", "/home/szymon/shell/history.txt", NULL};
    char **cmd[] = {task, NULL};
    pipeline(cmd);
}
int parse(char *line, char **argv)
{
    int count = 0;
    while (*line != '\0')
    {   
        // if(strcmp(line,"\"\n")==0)
        //     break;
        /* if not the end of line ....... */
        while (*line == ' ' || *line == '\t' || *line =='\"' || *line == '\n') //92 - \\/
            *line++ = '\0'; /* replace white spaces with 0    */
        if (strcmp(line, "") == 0)
            break;
        *argv++ = line;
        count++; /* save the argument position     */
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n' && *line !='\"')
            line++; /* skip the argument until ...    */
    }
    *argv = '\0'; /* mark the end of argument list  */

    return count;
}

void writehistory(char *line)
{
    FILE *fp;
    fp = fopen("/home/szymon/shell/history.txt", "a");

    fprintf(fp, "%s", line);

    fclose(fp);
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
        writehistory(task);
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
        if (cdflag == 0)
        {
            cmda[i] = NULL;
            pipeline(cmda);
        }
        printpath();
    }
}