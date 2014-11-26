/* See Chapter 5 of Advanced UNIX Programming:  http://www.basepath.com/aup/
 *   for further related examples of systems programming.  (That home page
 *   has pointers to download this chapter free.
 *
 * Copyright (c) Gene Cooperman, 2006; May be freely copied as long as this
 *   copyright notice remains.  There is no warranty.
 */

/* To know which "includes" to ask for, do 'man' on each system call used.
 * For example, "man fork" (or "man 2 fork" or man -s 2 fork") requires:
 *   <sys/types.h> and <unistd.h>
 */
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <errno.h> /*to use errno*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#define MAXLINE 200  /* This is how we declare constants in C */
#define MAXARGS 20

// "ls | wc"
static char * getword(char * begin, char **end_ptr) {
	char * end = begin;
	char * paver = NULL;
	while ( *begin == ' ' )
		begin++;  /* Get rid of leading spaces. */

	end = begin;


	char c =*end;
	/* '#' is for the beginning fo comment, which should be treated as
	   end of the command line.*/
	while ( c != '\0' && c != ' ' && c != '\n' && c !='#' )
		c = *(++end);  /* Keep going. */

	if(*end == '#') { // pave all chars afterward as '\0' BUG1 done.
		paver = end;
		while(*paver != '\0') *(paver++) = '\0';
	}

	if ( end == begin )
		return NULL;  /* if no more words, return NULL */

	*end = '\0';  /* else put string terminator at end of this word. */
	*end_ptr = end;
	if (begin[0] == '$') { /* if this is a variable to be expanded */
		begin = getenv(begin+1); /* begin+1, to skip past '$' */
		if (begin == NULL) {
			perror("getenv");
			begin = "UNDEFINED";
		}
	}
	return begin; /* This word is now a null-terminated string.  return it. */
}

/* In C, "int" is used instead of "bool", and "0" means false, any
 * non-zero number (traditionally "1") means true.
 */
/* argc is _count_ of args (*argcp == argc); argv is array of arg _values_*/
static void getargs(char cmd[], int *argcp, char *argv[])
{
	char *cmdp = cmd;
	char *end;
	int i = 0;

	/* fgets creates null-terminated string. stdin is pre-defined C constant
	 *   for standard intput.  feof(stdin) tests for file:end-of-file.
	 */
	if (fgets(cmd, MAXLINE, stdin) == NULL && feof(stdin)) {
		printf("Couldn't read from standard input. End of file? Exiting ...\n");
		exit(1);  /* any non-zero value for exit means failure. */
	}
	while ( (cmdp = getword(cmdp, &end)) != NULL ) { /* end is output param */
		/* getword converts word into null-terminated string */
		argv[i++] = cmdp;
		/* "end" brings us only to the '\0' at end of string */
		cmdp = end + 1;
	}
	argv[i] = NULL; /* Create additional null word at end for safety. */
	*argcp = i;
}

// only parent should call this.
static void one_child(int argc, char *argv[], int shallwait){
	pid_t childpid; /* child process ID */
	childpid = fork();
	if (childpid == -1) { /* in parent (returned error) */
		perror("fork"); /* perror => print error string of last system call */
		printf("  (failed to execute command)\n");
	}
	if (childpid == 0) { /* child:  in child, childpid was set to 0 */
		if (-1 == execvp(argv[0], argv)) {
			perror("execvp");
			printf("  (couldn't find command)\n");
		}
		/* NOT REACHED unless error occurred */
		exit(1);
	} else{ /* parent:  in parent, childpid was set to pid of child process */
		if(shallwait) waitpid(childpid, NULL, 0);  /* wait until child process finishes */
	}
}

// only parent should call this.
static void pipe_child(int argc, char *argv[]){
	int my_num =0; // child will look into this to find out his number.
	pid_t pids[2];
	int pfd[2];

	if (pipe(pfd) == -1) { perror("pipe"); exit(EXIT_FAILURE);}
	
	int i=1;
	for(; i<=2; i++){
		pid_t child_pid = fork();
		if(child_pid > 0){ // parent
			pids[my_num++] = child_pid;// my child will see it.											
		}
		else if(child_pid ==0){ // child
			if(my_num ==0){ // left child
				/* pipe setup*/
				close(pfd[0]); // close read side
				close(STDOUT_FILENO);

				if(dup(pfd[1]) == -1){ //redirect
					perror("cannot redirect output to pipe.");
					perror(strerror(errno));
					exit(errno);
				}

				// ignore other commands
				argv[1] = NULL;
				argv[2] = NULL;
				if (-1 == execvp(argv[0], argv)) {
					perror("execvp");
					printf("  (couldn't find command)\n");
				}
				/* NOT REACHED unless error occurred */
				exit(1);
			}
			else{// right child
				/* pipe setup*/
				close(pfd[1]); 
				close(STDIN_FILENO);
				if(dup(pfd[1]) == -1){ // redirect
					perror("cannot redirect output to pipe.");
					perror(strerror(errno));
					exit(errno);
				}

				// ignore previous words
				argv++ ;
				argv++ ;
				if (-1 == execvp(argv[0], argv)) {
					perror("execvp");
					printf("  (couldn't find command)\n");
				}
				/* NOT REACHED unless error occurred */
				exit(1);				
			}
		}
		else{// error		
			perror(strerror(errno));
			eixt(errno);
		}
	}

	/* parent pipe close*/
	close(pfd[0]);
	close(pfd[1]);

	// wait for child
	waitpid(pids[0], NULL, 0);
	waitpid(pids[1], NULL, 0);

}

//e.g. "< hi.txt wc", "> hi.txt wc", "wc < hi.txt". "wc > hi.txt"
static void redir_one_child(int argc, char *argv[]){
	pid_t pid = fork();

	if(pid <0){ //err
		perror(strerror(errno));
		eixt(errno);
	}else if(pid >0){ // parent
		waitpid(pid, NULL, 0);
		return;
	}

	/* child logic*/
	char* filename;
	char* progname;
	char redir ;

	// get each component
	if(*argv[0] == '<' || *argv[0] == '>'){ // move < to center
		redir = *argv[0];
		filename = argv[1];
		progname = argv[2];
	}else{
		progname = argv[0];	
		redir = *argv[1];
		filename = argv[2];		
	}

	int fd;

	if(redir = '<'){ // stdin redir
		close(STDIN_FILENO);
		fd = open(*filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR); // reusing STDIN_FILENO
		if (fd == -1) perror("open for writing");		
	}else{ //stdout redir
		close(STDOUT_FILENO);
		fd = open(*filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR); // reusing STDIN_FILENO
		if (fd == -1) perror("open for writing");		
	}
	
	// dirty hack.
	argv[0] = progname;
	argv[1] = NULL;
	argv[2] = NULL;
	if (-1 == execvp(argv[0], argv)) {
		perror("execvp");
		printf("  (couldn't find command)\n");
	}
	/* NOT REACHED unless error occurred */
	exit(1);

}

static void execute(int argc, char *argv[]){
	switch(argc){
		case 1: // simple  case: e.g. "ls"
			one_child(argc, argv,1);
			break;

		case 2: // 2 parts e.g. "ls &". run child process in backgroud, no wait().
			one_child(argc, argv,0);
			break;

		case 3: // 3 parts e.g. "ls | wc", "< hi.txt wc", "> hi.txt wc", "wc < hi.txt" "wc > hi.txt"
			if('|' == *argv[1]) // pipe
				pipe_child(argc, argv);
			else //e.g. "< hi.txt wc", "> hi.txt wc", "wc < hi.txt". "wc > hi.txt"
				redir_one_child(argc, argv);
			break;

		default:
			perror("arg count abnormal! please check.\n");
			break;
	}
}

void interrupt_handler(int signum){
	fprintf(stderr, "singla %d captured. current task will be aborted.\n",signum);
}

int main(int argc, char *argv[])
{
	char cmd[MAXLINE];
	char *childargv[MAXARGS];
	int childargc;

	//signal handler registering. BUG3 done.
	if( SIG_ERR == signal(SIGINT, interrupt_handler))
		fprintf(stderr, "singla handler registeration failed: %s\n",strerror(errno));

	if (argc >1){ // assosicate stdin  to the file. BUG2 done.
		stdin = freopen(argv[1], "r", stdin);
		if(stdin == NULL) {
			perror("stdin is null!");
			exit(1);
		}
	}

	while (1) {
		printf("%% "); /* printf uses %d, %s, %x, etc.  See 'man 3 printf' */
		fflush(stdout); /* flush from output buffer to terminal itself */
		getargs(cmd, &childargc, childargv); /* childargc and childargv are
							output args; on input they have garbage, but getargs sets them. */
		/* Check first for built-in commands. */
		if ( childargc > 0 && strcmp(childargv[0], "exit") == 0 )
			exit(0);
		else if ( childargc > 0 && strcmp(childargv[0], "logout") == 0 )
			exit(0);
		else
			execute(childargc, childargv);
	}
	/* NOT REACHED */
}
