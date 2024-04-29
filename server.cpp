#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define MAX_CLIENTS 5

struct ClientInfo {
    SOCKET socket;
    int id;
};

std::vector<ClientInfo> clients;

CRITICAL_SECTION csFileAccess;

SOCKET findClientSocketByID(int clientID) {
    // unused
    SOCKET socket = INVALID_SOCKET;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->id == clientID) {
            socket = it->socket;
            break;
        }
    }
    return socket;
}

void removeSocket(SOCKET socket) {
    closesocket(socket);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->socket == socket) {
            clients.erase(it);
            break;
        }
    }
}

void writeResultToFile(const char* message, int clientID) {
    EnterCriticalSection(&csFileAccess);
    FILE* file = fopen("Results.log", "a");
    if (file != NULL) {
        fprintf(file, "%s", message);
        fclose(file);
    } else {
        printf("Error opening file\n");
    }
    LeaveCriticalSection(&csFileAccess);
}

bool isPrime(unsigned int num) {
    if (num <= 1) return 0;
    if(num == 4294967291) return 1;     // prime but overflows uint
    for (unsigned int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return 0;
        }
    }
    return 1;
}

void count(int clientID, unsigned int start, unsigned int end) {
    char buffer[DEFAULT_BUFLEN];
    end = end == -1u ? -1u - 1 : end;   // dont overflow uint
    for (unsigned int i = start; i <= end; i++) {
        if (isPrime(i)) {
            sprintf(buffer, "Client %d: %u\n", clientID, i);
            writeResultToFile(buffer, clientID);
            Sleep(1);
        }
    }
}

DWORD WINAPI clientHandler(LPVOID lpParam) {
    SOCKET clientSocket = ((ClientInfo*)lpParam)->socket;
    int clientID = ((ClientInfo*)lpParam)->id;
    char recvbuf[DEFAULT_BUFLEN];

    do {
        int iResult = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            if (strncmp(recvbuf, "count", strlen("count")) == 0) {
                // Send "awaiting input" message to the client
                int iSendResult = send(clientSocket, "awaiting input", strlen("awaiting input"), 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("Client %d: send failed with error: %d\n", clientID, WSAGetLastError());
                    removeSocket(clientSocket);
                    return 1;
                }
                printf("Client %d: Awaiting input\n", clientID);

                // Receive input from the client
                unsigned int input;
                iResult = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
                if (iResult > 0) {
                    recvbuf[iResult] = '\0';
                    sscanf(recvbuf, "%u", &input);
                }
                else if (iResult == 0) {
                    printf("Client %d: Disconnected\n", clientID);
                    removeSocket(clientSocket);
                    return 0;
                }
                else {
                    printf("Client %d: recv failed with error: %d\n", clientID, WSAGetLastError());
                    removeSocket(clientSocket);
                    return 1;
                }

                // Send "counting" message to the client
                iSendResult = send(clientSocket, "counting", strlen("counting"), 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("Client %d: send failed with error: %d\n", clientID, WSAGetLastError());
                    removeSocket(clientSocket);
                    return 1;
                }
                printf("Client %d: Counting started\n", clientID);

                // Count prime numbers
                unsigned int start = 0;
                count(clientID, start, input);

                // Send "finished counting" message to the client
                iSendResult = send(clientSocket, "finished counting", strlen("finished counting"), 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("Client %d: send failed with error: %d\n", clientID, WSAGetLastError());
                    removeSocket(clientSocket);
                    return 1;
                }
                printf("Client %d: Counting finished\n", clientID);
            }
            else if (strncmp(recvbuf, "shutdown", 8) == 0) {
                // Shutdown server and disconnect all clients
                printf("Client %d: < Server shutdown >\n", clientID);

                for (auto it = clients.begin(); it != clients.end(); ++it) {
                    shutdown(it->socket, SD_SEND);
                    closesocket(it->socket);
                }
                clients.clear();

                printf("Server shutting down...\n");
                exit(0); // Terminate server
            }
            else {
                printf("Client %d: '%s' (unsupported response)\n", clientID, recvbuf);
            }
        }
        else if (iResult == 0) {
            printf("Client %d: Disconnected\n", clientID);
            removeSocket(clientSocket);
            return 0;
        }
        else {
            printf("Client %d: recv failed with error: %d\n", clientID, WSAGetLastError());
            removeSocket(clientSocket);
            return 1;
        }

    } while (true);
}

int main(void)
{
    printf("Server starting...\n\n");

    WSADATA wsaData;
    int iResult;

    SOCKET listenSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    InitializeCriticalSection(&csFileAccess);

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Clear Results.log
    FILE* file = fopen("Results.log", "w");
    if (file != NULL) {
        fprintf(file, "%s", "");
        fclose(file);
    } else {
        printf("Error opening file\n");
    }

    int clientID = 0;
    while (true) {
        if (clients.size() < MAX_CLIENTS) {
            // Accept a client socket
            SOCKET clientSocket = accept(listenSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(listenSocket);
                WSACleanup();
                return 1;
            }

            printf("Client %d: Connected\n", clientID);

            // Send client ID to the client
            char clientIDStr[10];
            sprintf(clientIDStr, "%d", clientID);
            iResult = send(clientSocket, clientIDStr, strlen(clientIDStr), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                continue;
            }

            // Store information about the client
            ClientInfo newClient;
            newClient.socket = clientSocket;
            newClient.id = clientID;
            clients.push_back(newClient);

            // Create thread for the client
            HANDLE hThread = CreateThread(NULL, 0, clientHandler, &clients.back(), 0, NULL);
            if (hThread == NULL) {
                printf("CreateThread failed with error: %d\n", WSAGetLastError());
                removeSocket(clientSocket);
                continue;
            }
            CloseHandle(hThread);

            clientID++;
        }
    }

    // cleanup
    closesocket(listenSocket);
    WSACleanup();

    DeleteCriticalSection(&csFileAccess);

    return 0;
}