		     	    Homework 3  [CORRECTED:  Was "Homework 4"]
			Due Tues., Nov. 25

[ This assignment adds to the shell that you
  built in prog-hw1.  There are two associated files, hw3-extra-short.c
  and hw3-extra.c .  hw3-extra-short.c is a short program showing how
  to set up a pipe for "ls | wc" in UNIX.  hw3-extra.c is the same
  program, but written with more complete error checking.  As you look
  over the examples, be sure to often check the UNIX man pages:
    man pipe, man dup, man dup2, man perror, man fprintf, etc.]

Add pipes and stdin/stdout redirection and background jobs to myshell.
You should be able to do things like:
  ls | wc
  < myfile wc
  wc < myfile
  ls > FILES.txt
  ls &

COMMENT:  Hopefully, this will not affect you.  But be aware that UNIX/Linux
  supports two different kinds of I/O:  the section 2 low-level man pages
  and section 3 high-level man pages.  Normally, you should not mix the two.
  The suggestions at the end use the low level version.  This is
  done in a child process of myshell.  After this is done, execvp()
  will be called, thus causing the process to execute a new program.
  For this reason, there should be no conflict between the low-level I/O
  and any high-level I/O used elsewhere.

Under Linux,  man pipe    provides this example for the pipe command.

EXAMPLE
       The  following  program  creates  a pipe, and then fork(2)s to create a
       child process.  After the fork(2), each process closes the  descriptors
       that  it  doesn't  need  for  the  pipe (see pipe(7)).  The parent then
       writes the string contained in the program's command-line  argument  to
       the  pipe,  and  the  child reads this string a byte at a time from the
       pipe and echoes it on standard output.

       #include <sys/wait.h>
       #include <assert.h>
       #include <stdio.h>
       #include <stdlib.h>
       #include <unistd.h>
       #include <string.h>

       int
       main(int argc, char *argv[])
       {
           int pfd[2];
           pid_t cpid;
           char buf;

           assert(argc == 2);

           if (pipe(pfd) == -1) { perror("pipe"); exit(EXIT_FAILURE); }

           cpid = fork();
           if (cpid == -1) { perror("fork"); exit(EXIT_FAILURE); }

           if (cpid == 0) {    /* Child reads from pipe */
               close(pfd[1]);          /* Close unused write end */

               while (read(pfd[0], &buf, 1) > 0)
                   write(STDOUT_FILENO, &buf, 1);

               write(STDOUT_FILENO, "\n", 1);
               close(pfd[0]);
               _exit(EXIT_SUCCESS);

           } else {            /* Parent writes argv[1] to pipe */
               close(pfd[0]);          /* Close unused read end */
               write(pfd[1], argv[1], strlen(argv[1]));
               close(pfd[1]);          /* Reader will see EOF */
               wait(NULL);             /* Wait for child */
               exit(EXIT_SUCCESS);
           }
       }

=======================================================================
SUGGESTIONS FOR IMPLEMENTATION:
(These are suggestions only, not the spec.  If you prefer another way to
 do this, you may do so.)

MODIFY getargs():
  Change parsing in getargs() to recognize pipe character '|',
    and stdin and stdout characters, '<' and '>'.
  The characters '<' and '>' can occur anywhere, and the next word to be read
    is saved as file_input and file_output.

PIPE EXAMPLE:
  Create the pipe before fork().  Then both parent and children will share it.
  FIRST PROCESS AND SECOND PROCESS WILL BOTH BE CHILDREN OF myshell PROCESS.
  DO THE STUFF BELOW AFTER fork(), BUT BEFORE execvp().
FIRST PROCESS:
  close(STDOUT_FILENO);
  dup2(pdf[1], STDOUT_FILENO);  /* Now any output to stdout goes to pipe. */
SECOND PROCESS:
  close(STDIN_FILENO);
  dup2(pdf[0], STDIN_FILENO);  /* Now any read from stdin reads from pipe. */

FILE OUTPUT REDIRECT:
  DO THE STUFF BELOW AFTER fork(), BUT BEFORE execvp().
  close(STDOUT_FILENO);
  /* This causes any output to stdout goes to output_file: see 'man open' */
  fd = open("output_file", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) perror("open for writing");

FILE INPUT REDIRECT:
  DO THE STUFF BELOW AFTER fork(), BUT BEFORE execvp().
  close(STDIN_FILENO);
  /* This causes any input from stdin to comre from input_file: see 'man open' */
  fd = open("input_file", O_RDONLY);  /* Now any read from stdin reads from input_file. */
  if (fd == -1) perror("open for reading");
