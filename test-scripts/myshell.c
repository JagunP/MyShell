#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <fcntl.h>

//batch mode and redirection

void myPrint(char *msg){
    write(STDOUT_FILENO, msg, strlen(msg));
}

void errorMsg(){
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

void run(char** cmd, int redirect, int fd){
    pid_t pid = fork();


    if(pid == 0){
        if(redirect == 0){
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if(execvp(cmd[0], cmd) < 0){
            errorMsg();
            exit(0);
        }
        
    } else if(pid < 0){
        errorMsg();
        exit(0);
    } else {
        wait(NULL);
    }
}

char** ElimExcessSpace(char* in, int* cnt){
    char** expCmd = malloc(sizeof(char*)*256);

    int c = 0;
    char* new = strtok(in, " \t\n");
    
    if(!expCmd){
        errorMsg();
        exit(0);
    }

    while(new){
        expCmd[c] = new;
        c++;
        new = strtok(NULL, " \t\n");
    }
    expCmd[c] = NULL;
    *cnt = c;
    return expCmd;
}


//Not working :((
int Redirection(char** cmd){
    int a = 0, arr = 0;

    //check if redirection is present
    while(cmd[a]){
        for(int x = 0; x<strlen(cmd[a]); x++){
            if(cmd[a][x] == '>'){
                arr++;
            }
        }
        a++;
    }

    //rejoin the strings
    //add space first
    if(arr>0){
        /*int x = 0;
        while(cmd[x]){
            strcat(cmd[x], " ");
            x++;
        }*/

        char res[] = "\0";
        for(int y = 0; y<a; y++){
            strcat(res, cmd[y]);    
        }

    //break it up by >
        char* newCmd = strtok(res, ">");
        int b = 0;
        for(b = 0; b<2; b++){
            cmd[b] = newCmd;
            newCmd = strtok(NULL, ">");
        }
        cmd[b] = NULL;
    } else {
        return 0;
    }

    //double check to make sure its not built in
    if((strcmp(cmd[0], "pwd") == 0) || (strcmp(cmd[0], "cd") == 0)){
        errorMsg();
        return 0;
    } 


    int len = strlen(cmd[0]);
    for(int f = 0; f<len-2; f++){
        if((cmd[0][f] == 'c') && (cmd[0][f+1] == 'd')){
            errorMsg();
            return 0; 
        }
    }

    //check if nothing after >
    if(cmd[1] == NULL){
        errorMsg();
        return 0;//should this exit or continue
    }


    //should only have 2 inputs (0, 1) everything before > and after

    int output = open(cmd[1], O_CREAT|O_EXCL, 0666 ); 
    if(output == -1){ //if the file exists
        errorMsg();
        close(output);
        return 0;
    }
    close(output);


    //ls -l
    int fd = open(cmd[1], O_RDWR|O_CREAT|O_APPEND, 0666); 
    cmd[1] = NULL;

    dup2(fd, STDOUT_FILENO);
    close(fd);

    run(cmd, 0, fd);

    /*int test;
    char** row = ElimExcessSpace(cmd[0], &test);

    // while(cmd[q]){
    // char* testCmd = strtok(cmd[0], " ");
    // int f = 0;
    // while(testCmd){
        // cmd[f] = testCmd;
        // f++;
        // testCmd = strtok(NULL, " ");
    // }
    // cmd[f] = NULL;
    
    //basically re running
    pid_t pid = fork();

    if(pid == 0){
        dup2(fd, STDOUT_FILENO);
        close(fd);
        if(execvp(row[0], row) < 0){
            errorMsg();
            // return 0;
            exit(0);
        }
    } else if(pid < 0){
        errorMsg();
        exit(0);
    } else {
        wait(NULL);
        close(fd);
    }*/
    return 1;
}

char** multipleCmds(char* input){
    char** totalCmd = malloc(sizeof(char*)*256);
    int m = 0;
    char* new = strtok(input, ";");

    
    if(!totalCmd){
        errorMsg();
        free(totalCmd);
        exit(0);
    }

    while(new){
        totalCmd[m] = new;
        m++;
        new = strtok(NULL, ";");
    }
    totalCmd[m] = NULL;
    return totalCmd;

}


int builtInCmd(char** totalCmd, int arg_cnt){
    char buffer[2024];
    char* path;

        if(totalCmd[0] == NULL){
            return 1;
        }
        if(strcmp(totalCmd[0], "exit") == 0){
            if(arg_cnt != 1){
                errorMsg();
                return 1;
            }
            free(totalCmd);
            exit(0);
        } else if(strcmp(totalCmd[0],"pwd") == 0 ){//print current path
            if(arg_cnt != 1){
                errorMsg();
                return 1;
            }
            if(totalCmd[1] != NULL){
                errorMsg();
                return 1;
            } else {
                path = getcwd(buffer, sizeof(buffer));
                myPrint(path);
                myPrint("\n");
                return 1;
            }
        } else if(strcmp(totalCmd[0],"cd") == 0){//go to home or specified space
             if(arg_cnt > 2){
                errorMsg();
                return 1;
            }
            if (totalCmd[1] == NULL){
                chdir(getenv("HOME"));//cd
                return 1;
            } else {
                int work = chdir(totalCmd[1]); //cd test
                if(work != 0){
                    errorMsg();
                }
                return 1;
            }
        } else {
            return 0;
        }
    
    return 0;
}

int extraStuff(char* input){
    for(int k = 0; k<strlen(input); k++){ //if line is not empty
        if((input[k] != ' ') && (input[k] != '\t') && (input[k] != '\n') &&
        (input[k] != '\0')){
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    char cmd_buff[1024]; 
    char *pinput;
    int batch = 0, interactive = 0;
    FILE* file; 

    
    if(argc == 1){//interactive mode
        interactive = 1;
        file = stdin;
    } else if(argc == 2){//batch mode
        batch = 1;
        file = fopen(argv[1], "r");
    } else {
        errorMsg();
        exit(0); 
    }
    if(file == NULL){
        errorMsg();
        exit(0);
    }


    if(interactive){
        while (1) { //the infinite loop
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 1024, file); //loads 100characters of stdin to space in cmd_buff
        
            //is there an error?
            if (!pinput) {//if there troubles with reading the file.
                errorMsg();
                free(pinput);
                exit(0);
            } 
        
            //is the input too long?
            if(strlen(pinput) >= 514){//if the input is too long
                errorMsg();
                continue; 
            }

            //is the input empty?
            if(pinput[0] == '\n'){//if the input is just new line or empty(eof)  
                continue;
            }

            char** cmd;
            int b = 0 , res2, arr=0;
            int arg_cnt;
            char** hehe = multipleCmds(pinput); //breaks it up into the diff cmds using ;
            while(hehe[b]){
                cmd = ElimExcessSpace(hehe[b], &arg_cnt); //removes spaces
                res2 = builtInCmd(cmd, arg_cnt); //checks if bulit in
                int a =0;
                while(cmd[a]){
                    for(int x = 0; x<strlen(cmd[a]); x++){
                        if(cmd[a][x] == '>'){
                        arr++;
                    }
                }
                a++;
            }
            if(arr>0){
                Redirection(cmd);
            } else if(res2 == 0){
                run(cmd, 1, 1);
            }
                /*if((Redirection(cmd) == 0) || 
                // int redirect = Redirection(cmd);
                (res2 == 0)){
                    run(cmd);
                }*/
                b++;
            }
        }
    }   


//check if new line tab or apace return 1 otherwise 0; 
    if(batch){
        while (1) {
            pinput = fgets(cmd_buff, 1024, file);
            if (!pinput) {//if there troubles with reading the file.
                fclose(file);
                exit(0);
            } 
    
            if(extraStuff(cmd_buff)){
                continue;
            }
            myPrint(pinput);

            //is the input too long?
            if(strlen(pinput) >= 514){//if the input is too long
                errorMsg();
                continue; 
            }

            //is the input empty?
            if(pinput[0] == '\n'){//if the input is just new line or empty(eof)  
                continue;
            }

            if((strcmp(pinput, " ") == 0) || (strcmp(pinput, "\t") == 0) || (strcmp(pinput, "\n") == 0)){
                continue;
            }

            char** cmd;
            int arg_cnt;
            int b = 0, res2;
            char** hehe = multipleCmds(pinput); //breaks it up into the diff cmds using ;
            while(hehe[b]){
                if(extraStuff(hehe[b]) == 1){
                    b++;
                    continue;
                }
                cmd = ElimExcessSpace(hehe[b], &arg_cnt); //removes spaces
                res2 = builtInCmd(cmd, arg_cnt); //checks if bulit in
                if (res2 == 0){   
                    run(cmd, 1, 1);
                }
                b++;
            }
        }
    }
}


