#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>


/*handler for the SIGCHLD signal , killing of zombie processes, in a loop*/
void handle_zombies(int sig) {
    while (waitpid(-1,NULL,WNOHANG)>0);
}


int prepare(void) {
    struct sigaction sa_chld,sa_int;
    /*set the SIGCHLD for handeling the zombies*/
    sa_chld.sa_handler = handle_zombies;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD,&sa_chld,NULL)==-1) {
        perror("failed to set SIGCHLD handler");
        return 1;
    }

    /*ignore SIGINT in parent*/
    sa_int.sa_handler=SIG_IGN;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags=0;
    if (sigaction(SIGINT,&sa_int,NULL)==-1) {
        perror("failed to ignore SIGINT");
        return 1;
    }
    return 0;
}


/*function for restore default SIGINT in child processes*/
void restore_default_sigint(void) {
    struct sigaction sa;
    sa.sa_handler=SIG_DFL;
    sa.sa_flags=SA_RESTART;
    if (sigaction(SIGINT,&sa,NULL)==-1) {
        perror("failed to restore SIGINT default handler");
    }
}


/*function to ignore SIGINT in background*/
void ignore_sigint(void) {
    struct sigaction sa;
    sa.sa_handler=SIG_IGN;
    sa.sa_flags=SA_RESTART;
    if (sigaction(SIGINT,&sa,NULL)==-1) {
        perror("falied to ignore SIGINT in background process");
    }
}


/*function to execute command in the background*/
int execute_in_background(char **arglist) {
    pid_t pid =fork();
    if (pid==-1) {
        perror("failed to fork");
        return 0;
    }
    else if (pid==0) {
        ignore_sigint(); /*ignore SIGINT in background processes*/
        execvp(arglist[0],arglist);
        perror("execvp failed");
        exit(1);
    }
    return 1;
}


/*function for executing a command using a pipe*/
int execute_with_pipe(char **arglist,int pipe_index) {
    arglist[pipe_index]=NULL;
    char **second_command=&arglist[pipe_index+1];
    int pipefd[2];
    if (pipe(pipefd)==-1) {
        perror("failed to create pipe");
        return 0;
    }
    pid_t pid1=fork();
    if (pid1==-1) {
        perror("failed to fork");
        return 0;
    }
    else if (pid1==0) {
        restore_default_sigint(); /*restoring the default sigint in the child*/
        close(pipefd[0]);
        dup2(pipefd[1],STDOUT_FILENO); /*redirecting stdout for the pipe*/
        close(pipefd[1]);
        execvp(arglist[0],arglist);
        perror("execvp failed");
        exit(1);
    }

    pid_t pid2=fork();
    if (pid2==-1) {
        perror("failed to fork");
        return 0;
    }
    else if (pid2==0) {
        restore_default_sigint(); /*restoring the default sigint in the child*/
        close(pipefd[1]);
        dup2(pipefd[0],STDIN_FILENO);/*redirecting stdin for the pipe*/
        close(pipefd[0]);
        execvp(second_command[0],second_command);
        perror("execvp failed");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1,NULL,0);/*wait first command to finish*/
    waitpid(pid2,NULL,0);/*wait second command to finish*/
    return 1;
}


/*regular command functioning*/
int execute_command(char **arglist) {
    pid_t pid=fork();
    if (pid==-1) {
        perror("failed to fork");
        return 0;
    }
    else if (pid==0) {
        restore_default_sigint(); /*restoring default sigint in child*/
        execvp(arglist[0],arglist);
        perror("execvp failed");
        exit(1);
    }
    else {
        waitpid(pid,NULL,0);/*wait until command is finish*/
    }
    return 1;
}


/*handeling input redirection*/
int handle_input_redirection(char **arglist,int redirect_index) {
    arglist[redirect_index]=NULL;
    pid_t pid =fork();
    if (pid==-1) {
        perror("failed to fork");
        return 0;
    }
    else if ( pid==0) {
        restore_default_sigint(); /*restoring default sigint in child*/
        int fd=open(arglist[redirect_index+1],O_RDONLY);
        if (fd ==-1) {
            perror("failed opening file for input redireciton");
            exit(1);
        }
        dup2(fd,STDIN_FILENO);/*redirecting stdin to file*/
        close(fd);
        execvp(arglist[0],arglist);
        perror("execvp failed");
        exit(1);
    }
    else {
        waitpid(pid,NULL,0);/*waiting for the commands to finish*/
    }
    return 1;
}

/*handeling output redirection with appending*/
int handle_output_append(char **arglist,int append_index) {
    arglist[append_index]=NULL;
    pid_t pid=fork();
    if (pid==-1) {
        perror("failed to fork");
        return 0;
    }
    else if (pid==0) {
        restore_default_sigint(); /*resting default sigint in child*/
        int fd=open(arglist[append_index+1],O_WRONLY|O_CREAT|O_APPEND,0644);
        if (fd==-1) {
            perror("failed to open for output append");
            exit(1);
        }
        dup2(fd,STDOUT_FILENO); /*redirecting stdout for the file*/
        close(fd);
        execvp(arglist[0],arglist);
        perror("execvp failed");
        exit(1);
    }
    else {
        waitpid(pid,NULL,0); /*wait for the command to finish*/
    }
    return 1;
}



/*handeling the output redirection overwrite*/
int handle_output_redirection(char **arglist,int redirect_index) {
    arglist[redirect_index]=NULL;
    pid_t pid=fork();
    if (pid==-1) {
        perror("failed to fork");
        return 0;
    }
    else if (pid==0) {
        restore_default_sigint(); /*restoring default sigint in child*/
        int fd=open(arglist[redirect_index+1],O_WRONLY|O_CREAT|O_TRUNC , 0644);
        if (fd==-1) {
            perror("failed to open file for output redirection");
            exit(1);
        }
        dup2(fd,STDOUT_FILENO); /*redirect stdout for the file*/
        close(fd);
        execvp(arglist[0],arglist);
        perror("failed to execvp");
        exit(1);
    }
    else {
        waitpid(pid,NULL,0); /*wait for the command to finish*/
    }
    return 1;
}



/*process the command line args*/
int process_arglist(int count,char **arglist) {
    int background=0;
    int pipe_pos=-1;
    int input_redirect_pos=-1;
    int output_append_pos=-1;
    int output_redirect_pos=-1;
    



    /*checking for background execute*/
    if (strcmp(arglist[count-1],"&")==0) {
        background=1;
        arglist[count-1]=NULL;
        count--;
    }



    /*checking for special chars and also their positions*/
    for (int i=0;i<count;i++) {
        if (strcmp(arglist[i],"|")==0) {
            pipe_pos=i;
            break;
        }
        else if (strcmp(arglist[i],"<")==0) {
            input_redirect_pos=i;
            break;
        }
        else if (strcmp(arglist[i],">>")==0) {
            output_append_pos=i;
            break;
        }
        else if (strcmp(arglist[i],">")==0) {
            output_redirect_pos=i;
            break;
        }
    }

    /*choosing the right function for the special char*/
    if (pipe_pos!=-1) {
        return execute_with_pipe(arglist,pipe_pos);
    }
    else if (input_redirect_pos!=-1) {
        return handle_input_redirection(arglist,input_redirect_pos);
    }
    else if (output_append_pos!=-1) {
        return handle_output_append(arglist,output_append_pos);
    }
    else if (output_redirect_pos!=-1) {
        return handle_output_redirection(arglist,output_redirect_pos);
    }
    else if (background) {
        return execute_in_background(arglist);
    }
    else {
        return execute_command(arglist);
    }
}

/*finalize all the shell*/
int finalize(void) {
    return 0;
}


























































































































































