#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> 

#define MAX_CMD_SIZE 64

typedef struct {
    char *name;
    char **args;
    int argCount;
} Command;

int parse_command(char *line, Command *cmd){
    char **tokens = malloc(MAX_CMD_SIZE * sizeof(char *)); // array of parsed arguments
    char *token = strtok(line, "  \t\n"); // tokenize command
    int idx = 0;

    while (token != NULL){
        tokens[idx] = token;
        idx++;
        token = strtok(NULL, "  \t\n");
    }

    tokens[idx] = NULL; // mark as end
    cmd->name = tokens[0];
    cmd->argCount = idx;
    cmd->args = tokens;
    return 0;

}

int main(){
    /*
    Large parts of this logic pulled from in class server.c
    */
   int welcomeSocket, newSocket;
   char buffer[1024];
   struct sockaddr_in serverAddr;
   socklen_t addrSize;
   struct sockaddr_storage serverStorage;

   // create socket with TCP
   welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (welcomeSocket < 0){
        perror("Socket creation failure!\n");
        exit(1);
    }

   // configure server address settings
   serverAddr.sin_family = AF_INET;
   serverAddr.sin_port = htons(0);
   serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

   // set all padding to 0
   memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

   // bind address struct to socket
    bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    socklen_t addrLen = sizeof(serverAddr);
    if (getsockname(welcomeSocket, (struct sockaddr *)&serverAddr, &addrLen) == -1){
        perror("Server failed\n");
        exit(1);
    }

    if (listen(welcomeSocket, 5) == 0){
        printf("Listening on port %d\n", ntohs(serverAddr.sin_port));
    }else{
        perror("Listening failure\n");
        close(welcomeSocket);
        exit(1);
    }

    while(1){
        addrSize = sizeof(serverStorage);
        newSocket = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addrSize);

        if (newSocket < 0) {
            perror("Accept failed\n");
            continue;
        }

        pid_t pid;
        pid = fork();
        if (pid == -1){
            perror("fork failed\n");
            exit(1);
        }

        if (pid == 0){
            Command cmd;
            while (1){
                // handle listening and forking here
                int commandSize = recv(newSocket, buffer, sizeof(buffer) - 1, 0);
                if (commandSize <= 0){
                    printf("Disconnected\n");
                    close(newSocket);
                    break;
                }
                buffer[commandSize] = '\0'; // done to NULL terminate the string

                parse_command(buffer, &cmd);

                pid_t executePid = fork();

                if (executePid == -1){
                    perror("fork failed\n");
                    exit(1);
                }
                if (executePid == 0){

                    if (dup2(newSocket, STDERR_FILENO) == -1 || dup2(newSocket, STDOUT_FILENO) == -1){
                        perror("Couldn't duplicate files\n");
                        exit(1);
                    }
                    fflush(stdout);
                    execvp(cmd.name, cmd.args);
                    if (send(newSocket, buffer, strlen(buffer), 0) == -1){
                        perror("Couldn't send to client\n");
                        exit(1);
                    }
                }
                int status;
                while (!WIFEXITED(executePid) && !WIFSIGNALED(executePid)){
                    waitpid(executePid, &status, WUNTRACED);
                    }
                }

                free(cmd.args);

            }

        }

   close(welcomeSocket);
   return 0;
}
