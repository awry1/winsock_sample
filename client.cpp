#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int main(int argc, char** argv)
{
    printf("Client starting...\n");

    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    int iResult;

    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server\n");
        /* 
        printf("⠀⣞⢽⢪⢣⢣⢣⢫⡺⡵⣝⡮⣗⢷⢽⢽⢽⣮⡷⡽⣜⣜⢮⢺⣜⢷⢽⢝⡽⣝\n");
        printf("⠸⡸⠜⠕⠕⠁⢁⢇⢏⢽⢺⣪⡳⡝⣎⣏⢯⢞⡿⣟⣷⣳⢯⡷⣽⢽⢯⣳⣫⠇\n");
        printf("⠀⠀⢀⢀⢄⢬⢪⡪⡎⣆⡈⠚⠜⠕⠇⠗⠝⢕⢯⢫⣞⣯⣿⣻⡽⣏⢗⣗⠏⠀\n");
        printf("⠀⠪⡪⡪⣪⢪⢺⢸⢢⢓⢆⢤⢀⠀⠀⠀⠀⠈⢊⢞⡾⣿⡯⣏⢮⠷⠁⠀⠀⠀\n");
        printf("⠀⠀⠀⠈⠊⠆⡃⠕⢕⢇⢇⢇⢇⢇⢏⢎⢎⢆⢄⠀⢑⣽⣿⢝⠲⠉⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⠀⠀⡿⠂⠠⠀⡇⢇⠕⢈⣀⠀⠁⠡⠣⡣⡫⣂⣿⠯⢪⠰⠂⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⠀⡦⡙⡂⢀⢤⢣⠣⡈⣾⡃⠠⠄⠀⡄⢱⣌⣶⢏⢊⠂⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⠀⢝⡲⣜⡮⡏⢎⢌⢂⠙⠢⠐⢀⢘⢵⣽⣿⡿⠁⠁⠀⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⠀⠨⣺⡺⡕⡕⡱⡑⡆⡕⡅⡕⡜⡼⢽⡻⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⠀⣼⣳⣫⣾⣵⣗⡵⡱⡡⢣⢑⢕⢜⢕⡝⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⣴⣿⣾⣿⣿⣿⡿⡽⡑⢌⠪⡢⡣⣣⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⡟⡾⣿⢿⢿⢵⣽⣾⣼⣘⢸⢸⣞⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀⠀⠁⠇⠡⠩⡫⢿⣝⡻⡮⣒⢽⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
        printf("⠀⠀⠀no connection?⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
         */
        WSACleanup();
        return 1;
    }

    // Receive client ID from the server
    char recvbuf[DEFAULT_BUFLEN];
    iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
    if (iResult > 0) {
        recvbuf[iResult] = '\0';
        printf("Connected with ID: %s\n", recvbuf);
    }
    else {
        printf("Failed to receive ID\n");
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    // Menu
    int option;
    do {
        printf("\nMenu:\n");
        printf("1 - start counting\n");
        printf("2 - close connection\n");
        printf("3 - shutdown server\n");
        printf("Option: ");
        scanf("%d", &option);

        switch (option) {
        case 1:
            // Send "count" message to the server
            iResult = send(connectSocket, "count", strlen("count"), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }
            printf("Sent: 'count'\n");

            // Receive response from the server
            iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0) {
                if (strncmp(recvbuf, "awaiting input", strlen("awaiting input")) == 0) {
                    printf("Recv: 'awaiting input'\n");

                    // Take input from the user
                    unsigned int input;
                    printf("\nEnter input: ");
                    scanf("%u", &input);

                    // Send input to the server
                    char inputStr[10];
                    sprintf(inputStr, "%u", input);
                    iResult = send(connectSocket, inputStr, strlen(inputStr), 0);
                    if (iResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(connectSocket);
                        WSACleanup();
                        return 1;
                    }
                    printf("Sent: '%s'\n", inputStr);

                    // Receive response from the server
                    iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                    if (iResult > 0) {
                        if (strncmp(recvbuf, "counting", strlen("counting")) == 0) {
                            printf("Recv: 'counting'\n");

                            // Receive response from the server
                            iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                            if (iResult > 0) {
                                if (strncmp(recvbuf, "finished counting", strlen("finished counting")) == 0) {
                                    printf("Recv: 'finished counting'\n");
                                }
                                else {
                                    printf("Recv: '%s' (unsupported response)\n", recvbuf);
                                }
                            }
                            else if (iResult == 0) {
                                printf("Disconnected by server\n");
                                closesocket(connectSocket);
                                WSACleanup();
                                return 1;
                            }
                            else {
                                printf("recv failed with error: %d\n", WSAGetLastError());
                                closesocket(connectSocket);
                                WSACleanup();
                                return 1;
                            }
                        }
                        else {
                            printf("Recv: '%s' (unsupported response)\n", recvbuf);
                        }
                    }
                    else if (iResult == 0) {
                        printf("Disconnected by server\n");
                        closesocket(connectSocket);
                        WSACleanup();
                        return 1;
                    }
                    else {
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(connectSocket);
                        WSACleanup();
                        return 1;
                    }
                }
                else {
                    printf("Recv: '%s' (unsupported response).\n", recvbuf);
                }
            }
            else if (iResult == 0) {
                printf("Disconnected by server\n");
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }
            else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }
            break;

        case 2:
            // Disconnect from the server
            iResult = shutdown(connectSocket, SD_SEND);
            if (iResult == SOCKET_ERROR) {
                printf("shutdown failed with error: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }
            printf("Disconnected\n");
            break;

        case 3:
            // Send "shutdown" message to the server
            iResult = send(connectSocket, "shutdown", 8, 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                return 1;
            }
            printf("Sent 'shutdown'\n");
            printf("Disconnected\n");
            break;

        default:
            printf("Invalid option\n");
            break;
        }
    } while (option != 2 && option != 3);

    // cleanup
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}