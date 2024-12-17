// Since we're so special, we define macros for STDIN and STDOUT
#define STDOUT 1
#define STDIN 0
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int specialChar, pipeIndex;

// Check for special characters in the arglist, and set flags accordingly
void checkSpecials(int count, char** arglist) 
{
    specialChar = 0;
    if (strcmp(arglist[count - 1], "&") == 0)
    {
        specialChar = 1;
    }
    if (strcmp(arglist[count - 2], "<") == 0)
    {
        specialChar = 2;
    }
    if (strcmp(arglist[count - 2], ">") == 0)
    {
        specialChar = 3;
    }
    for (int i = 0; i < count; i++)
    {
        if (strcmp(arglist[i], "|") == 0){
            specialChar = 4;
            pipeIndex = i;
        }
    }

}

int prepare(void)
{
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    {
        fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
        return 1;
    }
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
    {
        fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}
/* 
* I know I could've split the process in the main function and then spread every option to functions,
* but I only thought of this after I had already written the code. I'm sorry for the mess and weird commenting. 
*/
int process_arglist(int count, char** arglist) 
{
    checkSpecials(count, arglist);
    int dupStatus, returnVal = 1;
    pid_t pid_child;

    fprintf(stdout, "Special char: %d\n", specialChar);

    switch (specialChar)
    {
    case 1: // Background
        arglist[count - 1] = NULL; // Replace the & character with NULL for execvp
        pid_child = fork();
        switch (pid_child)
        {
        case 0:
            execvp(arglist[0], arglist);
            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno));
            exit(1);
            break;
        
        case -1:
            fprintf(stderr, "fork failed, error: %s\n", strerror(errno));
            returnVal = 0;
            break;

        default:
            break;
        }
        break;
    
    case 2: // Input redirection
        int fd_in;
        arglist[count - 2] = NULL; // Replace the < character with NULL for execvp
        pid_child = fork();

        if (pid_child == 0)
        {
            if (signal(SIGINT, SIG_DFL) == SIG_ERR) // Allow child process to be terminated with SIGINT
            {
                fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
                exit(1);
            }
            fd_in = open(arglist[count - 1], O_RDONLY);
            if (fd_in == -1)
            {
                fprintf(stderr, "open failed while opening the file, error: %s\n", strerror(errno));
                exit(1);
            }
            dupStatus = dup2(fd_in, STDIN);
            if (dupStatus == -1)
            {
                fprintf(stderr, "dup2 failed while redirecting file to stdin, error: %s\n", strerror(errno));
                exit(1);
            }
            
            close(fd_in); // Redirect file to stdin and close the file descriptor
            execvp(arglist[0], arglist); // Execute the command

            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno)); // Error handling if execvp fails
            exit(1);
        }
        else
        {
            if (pid_child == -1)
            {
                fprintf(stderr, "fork failed, error: %s\n", strerror(errno));
                returnVal = 0;
            }
            if (waitpid(pid_child, NULL, 0) == -1 && errno != ECHILD && errno != EINTR)
            {
                fprintf(stderr, "waitpid failed, error: %s\n", strerror(errno));
                returnVal = 0;
            }
        }
        
        break;

    case 3: // Output redirection
        int fd_out;
        arglist[count - 2] = NULL; // Replace the > character with NULL for execvp
        pid_child = fork();

        if (pid_child == 0)
        {
            if (signal(SIGINT, SIG_DFL) == SIG_ERR) // Allow child process to be terminated with SIGINT
            {
                fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
                exit(1);
            }
            fd_out = open(arglist[count - 1], S_IRUSR | S_IWUSR, 0600);
            if (fd_out == -1)
            {
                fprintf(stderr, "open failed while opening the file, error: %s\n", strerror(errno));
                exit(1);
            }
            dupStatus = dup2(STDOUT, fd_out);
            if (dupStatus == -1)
            {
                fprintf(stderr, "dup2 failed while redirecting file to stdin, error: %s\n", strerror(errno));
                exit(1);
            }
            close(fd_out); // Redirect stdout to file and c lose the file descriptor
            execvp(arglist[0], arglist); // Execute the command

            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno)); // Error handling if execvp fails
            exit(1);
        }
        else
        {
            if (pid_child == -1)
            {
                fprintf(stderr, "fork failed, error: %s\n", strerror(errno));
                returnVal = 0;
            }
            if (waitpid(pid_child, NULL, 0) == -1 && errno != ECHILD && errno != EINTR)
            {
                fprintf(stderr, "waitpid failed, error: %s\n", strerror(errno));
                returnVal = 0;
            }
        }
        

        break;

    case 4: // Pipe
        pid_t pid_w, pid_r;
        int pfds[2], pipeStatus;
        pipeStatus = pipe(pfds);
        if (pipeStatus == -1)
        {
            fprintf(stderr, "pipe failed, error: %s\n", strerror(errno));
            returnVal = 0;
        }
        arglist[pipeIndex] = NULL; // Replace the pipe character with NULL

        // Split the arglist into two arrays for the two commands - execvp uses NULL terminated arrays
        char **firstCommand = arglist;
        char **secondCommand = &arglist[pipeIndex + 1];

        pid_w = fork();
        if (pid_w == 0)
        {
            if (signal(SIGINT, SIG_DFL) == SIG_ERR) // Allow child process to be terminated with SIGINT
            {
                fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
                exit(1);
            }
            dupStatus = dup2(pfds[1], STDOUT); // Redirect stdout to the write end of the pipe
            if (dupStatus == -1)
            {
                fprintf(stderr, "dup2 failed while redirecting file to stdin, error: %s\n", strerror(errno));
                exit(1);
            }
            // Close the read and write ends of the pipe
            close(pfds[0]); 
            close(pfds[1]); 
            execvp(firstCommand[0], firstCommand);
            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno)); // Error handling if execvp fails
            exit(1);
        }
        else
        {
            pid_r = fork();
            if (pid_r == 0)
            {
                if (signal(SIGINT, SIG_DFL) == SIG_ERR) // Allow child process to be terminated with SIGINT
                {
                    fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
                    exit(1);
                }
                dupStatus = dup2(pfds[0], STDIN); // Redirect the read end of the pipe to stdin
                if (dupStatus == -1)
                {
                    fprintf(stderr, "dup2 failed while redirecting file to stdin, error: %s\n", strerror(errno));
                    exit(1);
                }
                // Close the read and write ends of the pipe
                close(pfds[0]); 
                close(pfds[1]); 
                execvp(secondCommand[0], secondCommand);
                fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno)); // Error handling if execvp fails
                exit(1);
            }
            else
            {
                if (pid_w == -1 || pid_r == -1)
                {
                    fprintf(stderr, "one or more fork failed, error: %s\n", strerror(errno));
                    returnVal = 0;
                }
                
                close(pfds[0]);
                close(pfds[1]);
                
                if ((waitpid(pid_w, NULL, 0) == -1 || waitpid(pid_r, NULL, 0) == -1) && errno != ECHILD && errno != EINTR)
                {
                    fprintf(stderr, "waitpid failed, error: %s\n", strerror(errno));
                    returnVal = 0;
                }
            }
        }
        break;
    
    default:
        pid_child = fork();
        if (pid_child == 0)
        {
            if (signal(SIGINT, SIG_DFL) == SIG_ERR) // Allow child process to be terminated with SIGINT
            {
                fprintf(stderr, "signal failed, error: %s\n", strerror(errno));
                exit(1);
            }
            if (execvp(arglist[0], arglist) == -1){
                fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno));
                exit(1);
            }
        }
        else
        {
            
            if (waitpid(pid_child, NULL, 0) == -1 && errno != ECHILD && errno != EINTR)
            {
                fprintf(stderr, "waitpid failed, error: %s\n", strerror(errno));
                exit(1);
            }
        }
        
        break;
    }
    
    return returnVal;
}

int finalize(void)
{
    return 0;
}
