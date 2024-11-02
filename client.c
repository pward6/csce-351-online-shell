#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char **argv){
    /*
    Used substantial parts of the code from in-class client.c
    */
    int clientSocket;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int port;

    if (argc < 2){
        printf("Invalid args\n");
        return 1;
    }

    port = atoi(argv[1]);
    // create socket with TCP
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0){
        perror("Socket creation failure!\n");
        exit(1);
    }
    // configure client socket settings
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // set all padding in server address to 0
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    /*---- Connect the socket to the server using the address struct ----*/
    addr_size = sizeof(serverAddr);
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, addr_size) < 0) {
        perror("Connection failed");
        close(clientSocket);
        exit(1);
    }

    char *command = NULL;
    size_t commandSize = 0;
    while (1){
        memset(buffer, 0, sizeof(buffer)); // clear buffer
        printf(" x > "); // simple prompt
        getline(&command, &commandSize, stdin);

        // quit if user types quit\n
        if (strcmp(command, "quit\n") == 0){
            printf("Quitting...\n");
            break;
        }

        // try sending command to server via clientSocket
        if (send(clientSocket, command, strlen(command), 0) == -1) {
            perror("Send failed");
            free(command);
            break;
        }

        // receive and print and data returned from server
        recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("%s", buffer);
        //free(command);
    }

    // clean up clientSocket
    free(command);
    close(clientSocket);
    return 0;
}