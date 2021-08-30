#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>

#define PS1 "> "
#define bsize 1024

//GLOBALS
char *args[512];
int n=0;
char line[bsize];

//builtins
int cd(char **args) {
    struct passwd *pw = getpwuid(getuid());
    char *home = pw->pw_dir;

    if(args[1] == NULL || args[1][0]  ==  '~') chdir(home);
    else if(chdir(args[1]) != 0) puts(strerror(errno)); 

    return 1;
}

char *built_ins[] = {
    "cd",
};

int (*built[])(char**) = {
    &cd,
};

int num_built(void) {
    return sizeof(built_ins) / sizeof(char*);
};

//splitting
char *skipspace(char *spacecount) {
    while(isspace(*spacecount)) ++spacecount;
    return spacecount;
}
void split(char*cmd) {

    cmd=skipspace(cmd);
    char *token=strchr(cmd,' ');
    int i=0;

    while(token != NULL) {
        token[0]='\0';
        args[i]=cmd;
        ++i;
        cmd=skipspace(token+1);
        token=strchr(cmd,' ');
    }
    if(cmd[0] != '\0') {
        args[i]=cmd;
        token=strchr(cmd,'\n');
        token[0]='\0';
        ++i;
    }
    args[i]=NULL;
}

// fd[0] is write end
// fd[1] is read end
int execute(int input,int first,int last) {
    
    int fd[2];

    pipe(fd);
    pid_t pid=fork();
     
    if(pid == 0) {
        if(first == 1 && last == 0 && input == 0) {
            dup2(fd[1],STDOUT_FILENO);
        } else if(first == 0 && last == 0 && input != 0) {
            dup2(input,STDIN_FILENO);
            dup2(fd[1],STDOUT_FILENO);
        } else dup2(input,STDIN_FILENO);

        if(execvp(args[0],args) == -1) {
            puts("command not found");
            exit(EXIT_FAILURE);
        }
    } 

    if(input != 0) close(input);

    close(fd[1]);

    if(last == 1) close(fd[0]);

    return fd[0];

}

int launch(char *cmd,int input,int first,int last) {
    split(cmd);
    for(int i=0; i < num_built(); i++) {
    if(args[0] != NULL) {
            if(strcmp(args[0],"exit") == 0) {
                exit(0);
            } else if(strcmp(args[0],built_ins[i]) == 0) { 
                first = (*built[i])(args); 
            }
        }
        n+=1;
        return execute(input,first,last);

    }
    return 0;
}

//clearing
void flush(int n) {
    for(int i = 0; i<n; ++i) wait(NULL);
}


int main(void) {
    while(1) {
        printf(PS1);
        fflush(NULL);

        //getting user input
        if(!fgets(line,bsize,stdin)) return 0;

        int input=0, first_cmd=1;
        char*cmd=line;
        char*token=strchr(cmd,'|');

        while(token != NULL) {
            *token='\0';
            input=launch(cmd,input,first_cmd,0);

            cmd=token+1;
            token=strchr(cmd,'|');
            first_cmd=0;
        }
        input=launch(cmd,input,first_cmd,1);
        flush(n);
        n=0;

    }
    return 0;
} 
