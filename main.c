//  nyush
//
//  Created by Minghao Sun on 9/16/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

int childPids[100];
char* childNames[100];
int childCount = 0;



void handlerOne(int sig) {}
void handlerTwo(int sig) {}
void handlerThree(int sig) {}

void updateChild(int pid, char* name){
    char* noChanging = malloc(strlen(name) * sizeof(char));
    strcpy(noChanging, name);
    childPids[childCount] = pid;
    childNames[childCount] = noChanging;
    childCount++;
}

int hasMany(char* value, char** arr){
    int i = 0;
    int count = 0;
    while (arr[i] != NULL){
        if (strcmp(value, ">") == 0 || strcmp(value, ">>") == 0){
            if (strcmp(arr[i], ">") == 0 || strcmp(arr[i], ">>" ) == 0){
                count++;
            }
        }else if (strcmp(value, arr[i]) == 0){
            count++;
        }
        i++;
    }
    return count;
}

int isThisInArray(char* value, char** arr){
    int index = 0;
    int found = 0;
    char* current = arr[0];
    while (current != NULL){
        if (strcmp(value, current) == 0){
            found = 1;
            break;
        }
        index++;
        current = arr[index];
    }
    if (found){
        return index;
    }
    return -1;
}


void mySystemCall(char** args){
    //Here is the usual form of exec

    int status = -1;
    if (args[0][0] == '/' || args[0][0] == '.'){
        int status = execv(args[0], args);
    }else if (strchr(args[0], '/') != NULL){
        char dir[100];
        dir[0] = '.';
        dir[1] = '/';
        int index;
        for (index = 0; index < strlen(args[0]); index++){
            dir[index + 2] = args[0][index];
        }
        args[0] = dir;
        status = execv(args[0], args);
    }else{
        status = execvp(args[0], args);
    }
    if (status == -1){
        fprintf(stderr, "Error: invalid program\n");
    }
    exit(-1);

    
}


void outputRedirectFn(char** args, int index, int mode){
    if (index == 0){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }

    int pid = fork();
    if (pid == 0){
        int file = -1;
        if (mode == 1){
            file = open(args[index + 1], O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
            if (file == -1){
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            }
            dup2(file, 1);
            close(file);
        }else{
            file = open(args[index + 1], O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR);
            if (file == -1){
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            }
            dup2(file, 1);
            close(file);
        }
        
        char* subArr[100];
        int i;
        for (i = 0; i < index; i++){
            subArr[i] = args[i];
        }
        subArr[i] = NULL;
        mySystemCall(subArr);
    }
    int childStatus;
    waitpid(pid, &childStatus, WUNTRACED);
    if (!WIFEXITED(childStatus)){
        updateChild(pid, args[0]);
    }

}


void inputRedirectFn(char** args, int index){
    if (index == 0){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }
    
    int pid = fork();
    if (pid == 0){
        int file = open(args[index + 1], O_RDONLY);
        if (file == -1){
            fprintf(stderr, "Error: invalid file\n");
            exit(-1);
        }
        
        dup2(file, 0);
        close(file);
        char* subArr[100];
        int i;
        for (i = 0; i < index; i++){
            subArr[i] = args[i];
        }
        subArr[i] = NULL;
        mySystemCall(subArr);
    }
    int childStatus;
    waitpid(pid, &childStatus, WUNTRACED);
    if (!WIFEXITED(childStatus)){
        updateChild(pid, args[0]);
    }
}


void inputOurputRedirect(char** args, int inputIndex, int outputIndex, int mode){
    if (args[inputIndex + 1] == NULL || args[outputIndex + 1] == NULL){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }else if (inputIndex == 0 || outputIndex == 0){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }
    int pid = fork();
    if (pid == 0){
        int outputFile = -1;
        if (mode == 1){
            outputFile = open(args[outputIndex + 1], O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
            if (outputFile == -1){
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            }
            dup2(outputFile, 1);
            close(outputFile);
        }else{
            outputFile = open(args[outputIndex + 1], O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR);
            if (outputFile == -1){
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            }
            dup2(outputFile, 1);
            close(outputFile);
        }

        int inputFile = open(args[inputIndex + 1], O_RDONLY);
        if (inputFile == -1){
            fprintf(stderr, "Error: invalid file\n");
            exit(-1);
        }
        dup2(inputFile, 0);
        close(inputFile);
        char* subArr[100];
        int i;
        int first = inputIndex < outputIndex ? inputIndex : outputIndex;
        for (i = 0; i < first; i++){
            subArr[i] = args[i];
        }
        subArr[i] = NULL;
        mySystemCall(subArr);
    }
    int childStatus;
    waitpid(pid, &childStatus, WUNTRACED);
    if (!WIFEXITED(childStatus)){
        updateChild(pid, args[0]);
    }
}

void job(){
    if (childNames[0] != NULL){
        int i = 0;
        while (childNames[i] != NULL){
            printf("[%d] %s\n", i + 1, childNames[i]);
            i++;
        }
    }
}

void fg(char** args){
    if (childCount == 0){
        fprintf(stderr, "Error: invalid job\n");
        return;
    }
    int index = 0;
    if (args[1] == NULL){
        index = childCount-1;
    }else{
        index = atoi(args[1]) - 1;
        if (index < 0 || index >= childCount){
            fprintf(stderr, "Error: invalid job\n");
            return;
        }
    }
    int pid = childPids[index];
    kill(pid, SIGCONT);
    int status;
    waitpid(pid, &status, WUNTRACED);
    if (WIFEXITED(status)){
        int j;
        free(childNames[index]);
        for (j = index; j < childCount-1; j++){
            childPids[j] = childPids[j+1];
            childNames[j] = childNames[j+1];
        }
        childCount--;
        childPids[j] = 0;
        childNames[j] = NULL;
        //this means the child exit normally, we need to remove it from the list
    }
}


void myPipe(char** args, int index, int previousFd){
    if (isThisInArray("<", args) != -1){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }
    if (index == 0 || args[index + 1] == NULL){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }

    if (isThisInArray("|", args) == -1){
        
        
        int outputRedirect = isThisInArray(">", args);
        int doubleOutputRedirect = isThisInArray(">>", args);

        

        int pid = fork();
        if (pid == 0){
            dup2(previousFd, 0);
            // close(previousFd);

            if (outputRedirect != -1 || doubleOutputRedirect != -1){
                printf("Here");
                int file = 0;
                if (outputRedirect != -1){
                    file = open(args[outputRedirect + 1],  O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
                    if (file == -1){
                        fprintf(stderr, "Error: invalid file\n");
                        exit(-1);
                    }
                    dup2(file, 1);
                    close(file);
                }else{
                    file = open(args[doubleOutputRedirect + 1],  O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
                    if (file == -1){
                        fprintf(stderr, "Error: invalid file\n");
                        exit(-1);
                    }
                    dup2(file, 1);
                    close(file);
                }
                
                char* subArr[100];
                int i;
                int subIndex = outputRedirect > doubleOutputRedirect ? outputRedirect : doubleOutputRedirect;
                for (i = 0; i < subIndex; i++){
                    subArr[i] = args[i];
                }
                subArr[i] = NULL;
                mySystemCall(subArr);
            }else{
                mySystemCall(args);
            }
        }
        int childStatus;
        waitpid(pid, &childStatus, WUNTRACED);
        if (!WIFEXITED(childStatus)){
            updateChild(pid, args[0]);
        }
        return;
        
    }


    char* psOneArgs[100];
    char* psTwoArgs[100];
    int i;


    for (i = 0; i < index; i++){
        psOneArgs[i] = args[i];
    }
    psOneArgs[i] = NULL;

    
    int j = 0;
    i = index + 1;
    while (args[i] != NULL){
        psTwoArgs[j] = args[i];
        j++;
        i++;
    }
    psTwoArgs[j] = NULL;

    if (isThisInArray(">", psOneArgs) != -1 || isThisInArray(">>", psOneArgs) != -1){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }

    

    int fd[2];
    if (pipe(fd) == -1){
        fprintf(stderr, "Error: something wrong with pipe\n");
        return;
    }

    int pid1 = fork();
    if (pid1 == 0){
        close(fd[0]);
        dup2(previousFd, 0); //read from previousFd
        dup2(fd[1], 1);    //write to fd[1]
        close(previousFd);
        close(fd[1]);
        mySystemCall(psOneArgs);
    }

    
    int childStatus;
    waitpid(pid1, &childStatus, WUNTRACED);
    if (!WIFEXITED(childStatus)){
        updateChild(pid1, psOneArgs[0]);
    }
    int anotherPipeIndex = isThisInArray("|", psTwoArgs);
    close(fd[1]);
    myPipe(psTwoArgs, anotherPipeIndex, fd[0]);   //now pass fd[0]
    
    close(previousFd);
    close(fd[0]);

}

void handleArgs(char** args){
    if (isThisInArray("<<", args) != -1){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }


    if (hasMany(">", args) > 1 || hasMany("<", args) > 1){
        fprintf(stderr, "Error: invalid command\n");
        return;
    }

    int inputRedirect = isThisInArray("<", args);

    //Here I check if there's pipe
    int pipeIndex = isThisInArray("|", args);
    if (pipeIndex != -1){
        //we use save_in to save the stdin because we are using
        //it in myPipe, and will have it closed by the end of
        //the pipe. This will lead the whole shell to end
        int save_in = dup(STDIN_FILENO);

        //we are passing in the index of the first pipe and start
        //with stdin
        if (inputRedirect != -1 && inputRedirect < pipeIndex){
            int file = open(args[inputRedirect + 1], O_RDONLY);
            if (file == -1){
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            }
            dup2(file, 0);
            close(file);
            char* newArgs[100];
            int i = 0;
            int j = 0;
            while(args[i] != NULL){
                if (i != inputRedirect){
                    newArgs[j] = args[i];
                    j++;
                }
                i++;
            }
            newArgs[j] = NULL;
            myPipe(newArgs, pipeIndex - 1, 0);
        }else{
            myPipe(args, pipeIndex, 0);
        }
        
        //so here we restore the stdin from the save_in we previously
        //saved
        dup2(save_in, STDIN_FILENO);
        return;
    }
    

    //This part checks if there is output redirection
    int outputRedirect = isThisInArray(">", args);
    int doubleOutputRedirect = isThisInArray(">>", args);
    

    //This is the only case where two redirections are allowed
    //So I take care of this first
    if (hasMany(">", args) == 1 && hasMany("<", args) == 1){
        if (outputRedirect != -1){
            inputOurputRedirect(args, inputRedirect, outputRedirect, 1);
        }else{
            inputOurputRedirect(args, inputRedirect, doubleOutputRedirect, 2);
        }
        return;
    }

    //The rest of redirect can be easier
    if (inputRedirect != -1){
        if (args[inputRedirect + 1] == NULL){
            fprintf(stderr, "Error: invalid command\n");
        }else if (args[inputRedirect + 2] != NULL || strcmp(args[inputRedirect + 2], ">") != 0 || strcmp(args[inputRedirect + 2], ">>") != 0){
            fprintf(stderr, "Error: invalid command\n");
        }else{
            inputRedirectFn(args, inputRedirect);
        }
        return;
    }else if (outputRedirect != -1){
        if (args[outputRedirect + 1] == NULL){
            fprintf(stderr, "Error: invalid command\n");
        }else if (args[outputRedirect + 2] != NULL){
            fprintf(stderr, "Error: invalid command\n");
        }else{
            outputRedirectFn(args, outputRedirect, 1);
        }
        return;
    }else if (doubleOutputRedirect != -1){
        if (args[doubleOutputRedirect + 1] == NULL){
            fprintf(stderr, "Error: invalid command\n");
        }else if (args[doubleOutputRedirect + 2] != NULL){
            fprintf(stderr, "Error: invalid command\n");
        }else{
            outputRedirectFn(args, doubleOutputRedirect, 2);
        }
        return;
    }
    int pid = fork();
    if (pid == 0){
        mySystemCall(args);
    }
    int childStatus;
    waitpid(pid, &childStatus, WUNTRACED);
    if (!WIFEXITED(childStatus)){
        updateChild(pid, args[0]);
    }
}


int main(int argc, const char * argv[]) {
    signal(SIGINT, handlerOne);
    signal(SIGQUIT, handlerTwo);
    signal(SIGTSTP, handlerThree);
    childPids[0] = 0;
    childNames[0] = NULL;
    for (;;){
        //default baseNames
        //set up a new array and string to
        //catch user input at each loop
        char baseName[100];
        getcwd(baseName, sizeof(baseName));
        //copied from https://en.cppreference.com/w/c/string/byte/strrchr
        char *pLastSlash = strrchr(baseName, '/');
        char *pszBaseName = pLastSlash ? pLastSlash + 1 : baseName;
        char** inputArr = malloc(sizeof(char*) * 100);
        int index = 0;
        char userInput[1000];
        printf("[nyush %s]$ ", pszBaseName);
        fflush(stdout);
        if (fgets(userInput, sizeof(char) * 100, stdin) == NULL){
            // printf("Bye\n");
            exit(-1);
        }
        if (strcmp(userInput, "\n") == 0){
            continue;
        }
        //Here I removed the last char of the input which is '\n'
        char sub[1000];
        int j = 0;
        while (j < strlen(userInput) - 1){
            sub[j] = userInput[j];
            j++;
        }
        sub[j] = '\0';

        //split the substring of the input by space, and put the
        //output into an array of string
        char * piece = strtok(sub, " ");
        while (piece != NULL){
            inputArr[index] = piece;
            index++;
            piece = strtok(NULL, " ");
        }
        inputArr[index + 1] = NULL;
        

        //put the array of string into handleArgs to exec process
        if (strcmp(inputArr[0], "cd") == 0){
            if (index != 2){
                fprintf(stderr, "Error: invalid command\n");
            }else{
                int status = chdir(inputArr[1]);
                if (status == -1){
                    fprintf(stderr, "Error: invalid directory\n");
                }
            }
        }else if (strcmp(inputArr[0], "exit") == 0){
            if (index != 1){
                fprintf(stderr, "Error: invalid command\n");
            }else if(childCount != 0){
                fprintf(stderr, "Error: there are suspended jobs\n");
            }else{
                // printf("Bye\n");
                exit(-1);
            }
        }else if (strcmp(inputArr[0], "job") == 0){
            if (index != 1){
                fprintf(stderr, "Error: invalid command\n");
            }else{
                job(inputArr);
            }
        }else if (strcmp(inputArr[0], "fg") == 0){
            if (index > 2){
                fprintf(stderr, "Error: invalid command\n");
            }else{
                fg(inputArr);
            }
        }else{
            handleArgs(inputArr);
        }
    }
    return 0;
}
