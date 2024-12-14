// Since we're so special, we define macros for STDIN and STDOUT
#define STDOUT 1
#define STDIN 0
#include "shell.c"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>

int specialChar, pipeIndex;

void SIGCHLD_handler(int signo)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

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
        if (strcmp(arglist[i], "|") == 0)
            specialChar = 4;
            pipeIndex = i;
    }

}

int prepare(void)
{
    return 0;
}

int process_arglist(int count, char** arglist)
{
    signal(SIGINT, SIG_IGN);

    checkSpecials(count, arglist);

    switch (specialChar)
    {
    case 1:
        /* code */
        break;
    
    case 2:
        /* code */
        break;

    case 3:
        /* code */
        break;

    case 4: // Pipe
        pid_t pid_w, pid_r;
        int pfds[2];
        pipe(pfds);
        arglist[pipeIndex] = NULL; // Replace the pipe character with NULL

        // Split the arglist into two arrays for the two commands
        char **firstCommand = arglist;
        char **secondCommand = &arglist[pipeIndex + 1];

        pid_w = fork();
        if (pid_w == 0)
        {
            dup2(pfds[1], STDOUT); // Redirect the write end of the pipe to stdout
            // Close the read and write ends of the pipe
            close(pfds[0]); 
            close(pfds[1]); 
            execvp(firstCommand[0], firstCommand);
            exit(1); // check if error handling is correct
        }
        else
        {
            pid_r = fork();
            if (pid_r == 0)
            {
                dup2(pfds[0], STDIN); // Redirect the read end of the pipe to stdin
                // Close the read and write ends of the pipe
                close(pfds[0]); 
                close(pfds[1]); 
                execvp(secondCommand[0], secondCommand);
                exit(1); // check if error handling is correct
            }
            else
            {
                close(pfds[0]);
                close(pfds[1]);
                waitpid(pid_w, NULL, 0);
                waitpid(pid_r, NULL, 0);
            }
        }
        break;
    
    default:
        break;
    }
    
}

int finalize(void)
{
    return 0;
}
