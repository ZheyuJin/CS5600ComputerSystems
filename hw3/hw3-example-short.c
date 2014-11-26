/* For those who would like to continue to learn more about UNIX programming,
 *  see the FAQ:      http://www.erlenstar.demon.co.uk/unix/faq_toc.html
 *  or the YoLinux tutorial (e.g. Fork/Exec):
 *                    http://www.yolinux.com/TUTORIALS/ForkExecProcesses.html
 *  for the many details of UNIX programming.
 * This version does no error checking, but it is shorter to read.
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>    /* 'man fprintf' says we need this. */
#include <sys/wait.h> /* 'man waitpid' says we need this. */


/* Just in case these aren't already defined. */
#define STDIN 0
#define STDOUT 1

int main() {
    int pipe_fd[2];   /* 'man pipe' says its arg is this type */
    pid_t child1, child2; /* 'man fork' says it returns type 'pid_t' */
    char * argvChild[2];

    printf("Executing \"ls | wc\";\n");
    printf("Number of files in curr dir is (first # is answer): ");
    fflush(stdout);  /* Force printing to complete, before continuing. */

    pipe(pipe_fd);
    if (fork() == 0) { /* if we are child1, do:  "ls | ..." */
        close(STDOUT);
        dup(pipe_fd[1]); /* set up empty STDOUT to be pipe_fd[1] */
        close(pipe_fd[1]); /* let child2 know we won't write to pipe here */
        argvChild[0] = "ls"; argvChild[1] = NULL;
        execvp(argvChild[0], argvChild);
    } else if (fork() == 0) { /* if we are child2, do:  "... | wc" */
        close(STDIN);
        dup(pipe_fd[0]); /* set up empty STDIN to be pipe_fd[0] */
        close(pipe_fd[1]); /* let child2 know we won't write to pipe */
        argvChild[0] = "wc"; argvChild[1] = NULL;
        execvp(argvChild[0], argvChild);
    } else { /* else we're parent */
        int status;
        close(pipe_fd[1]); /* let child2 know we won't write to pipe */
        wait(NULL);        /* wait on any child. ignore status */
        wait(NULL);        /* wait on any child. ignore status */
    }
    return 0;  /* returning 0 from main() means success. */
}
