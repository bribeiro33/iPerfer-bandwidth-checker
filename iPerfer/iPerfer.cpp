#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>

#define BACKLOG 1
#define MAXSIZE 1000
#define MIN_PORT 1024
#define MAX_PORT 65535
#define FIN_MSG "FIN"
#define ACK_MSG "ACK"

// Function Declarations
int ParseArguments(int argc, char** argv, int* port, std::string* host, double* timer, bool* isServer);
int InitializeServer(const int port);
int AcceptConnection(const int sockfd);
int InitializeClient(const std::string host, const int port);
void SendData(const int sockfd, const double timer, const bool isServer);
void ReceiveData(const int sockfd, const bool isServer);
void CalculateRate(const long totalBytes, const struct timeval start, const struct timeval end, const bool isServer);
void PrintError(const char* msg, bool perrorFlag = false);

int main(int argc, char** argv) {
    int port;
    std::string host;
    double timer;
    bool isServer;
    int sockfd;
    
    if (ParseArguments(argc, argv, &port, &host, &timer, &isServer) != 0) {
        return 1;
    }

    // serverside: socket, bind, listen, accept, recv, (send ack), close
    if (isServer) {
        sockfd = InitializeServer(port);
        if (sockfd < 0) return 1;

        int connectfd = AcceptConnection(sockfd);
        if (connectfd < 0) return 1;

        ReceiveData(connectfd, isServer);
        close(connectfd);
    } 
    // clientside: socket, connect, send, (recv fin), close
    else {
        sockfd = InitializeClient(host, port);
        if (sockfd < 0) return 1;

        SendData(sockfd, timer, isServer);
    }

    close(sockfd);
    return 0;
}

// parse command line arguments
int ParseArguments(int argc, char** argv, int* port, std::string* host, double* timer, bool* isServer) {

    // in server mode
    // ./iPrefer -s -p <listen_port>
    if (strcmp(argv[1], "-s") == 0) {
        *isServer = true;
        
        if (argc != 4) {
            PrintError("Error: missing or extra arguments");
            return 1;
        }
        
        *port = std::atoi(argv[3]);
        if (*port < MIN_PORT || *port > MAX_PORT) {
            PrintError("Error: port number must be in the range of [1024, 65535]");
            return 1;
        } 
    } 

    // in client mode
    // ./iPerfer -c -h <server_hostname> -p <server_port> -t <time>
    else if (strcmp(argv[1], "-c") == 0) {
        *isServer = false;
        *host = argv[3];
        *port = std::atoi(argv[5]);
        *timer = atof(argv[7]);
        
        // TODO: check if more than error should be printed if multiple exist
        if (argc != 8) {
            PrintError("Error: missing or extra arguments");
            return 1;
        }
        
        if (*port < MIN_PORT || *port > MAX_PORT) {
            PrintError("Error: port number must be in the range of [1024, 65535]");
            return 1;
        }
        
        if (*timer <= 0) {
            PrintError("Error: time argument must be greater than 0");
            return 1;
        }
    }
    
    // no parsing error
    return 0;
}

// Initialize Server Socket
int InitializeServer(const int port) {
    int sockfd; // socket file descriptor
    struct sockaddr_in addr; // struct for IPv4 address
    int yes = 1;

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPv4, TCP
    if (sockfd == -1) {
        PrintError("server socket", true);
        return -1;
    }

    // enable reuse of port number
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        PrintError("server setsockopt", true);
        close(sockfd);
        return -1;
    }
    
    // Initialize address for socket
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // bind address (unique local name) to a socket (0 if successful, -1 if error)
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        PrintError("server bind", true);
        close(sockfd);
        return -1;
    }

    // listen for connections on socket (0 if successful, -1 if error)
    if (listen(sockfd, BACKLOG) == -1) {
        PrintError("server listen", true);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

// Accept Connection on Server
int AcceptConnection(const int sockfd) {
    struct sockaddr_in their_addr;
    socklen_t addr_len = sizeof(their_addr);
    int connectfd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_len);
    if (connectfd == -1) {
        PrintError("server accept", true);
        return -1;
    }

    return connectfd;
}

// // Receive Data on Server
// void ReceiveData(const int connectfd, const bool isServer) {
//     char buffer[MAXSIZE];
//     int bytes_recv;
//     long total_bytes = 0;
//     std::string data_received;
//     struct timeval start_time, end_time;

//     gettimeofday(&start_time, NULL);

//     while(true) {
//         bytes_recv = recv(connectfd, buffer, MAXSIZE, MSG_WAITALL);
//         if (bytes_recv == -1) {
//             PrintError("server recv", true);
//             break;
//         }
//         else if (bytes_recv == 0) {
//             // client has closed connection
//             gettimeofday(&end_time, NULL);
//             break;
//         }

//         std::cout << "Buffer contains: ";
//         for (int i = 0; i < bytes_recv; ++i) {
//             std::cout << buffer[i] << " " << i;
//         }
//         std::cout << std::endl;

//         //Check if the end of the buffer contains the FIN message
//         int fin_len = static_cast<int>(strlen(FIN_MSG));
//         if ((bytes_recv >= fin_len) && 
//             (strncmp(buffer + bytes_recv - fin_len, FIN_MSG, fin_len) == 0)) {
//                 gettimeofday(&end_time, NULL);
//                 // Send ACK message
//                 if (send(connectfd, ACK_MSG, strlen(ACK_MSG), 0) == -1) {
//                     PrintError("server send", true);
//                 }
//                 break;
//         }
//         total_bytes += bytes_recv;
//     }

//     CalculateRate(total_bytes, start_time, end_time, isServer);
// }

// Receive Data on Server
void ReceiveData(const int connectfd, const bool isServer) {
    char buffer[MAXSIZE];
    int bytes_recv;
    long total_bytes = 0;
    std::string data_received;
    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    while(true) {
        bytes_recv = recv(connectfd, buffer, MAXSIZE, MSG_PEEK);
        // Check if the buffer is the size of the FIN message
        int fin_len = static_cast<int>(strlen(FIN_MSG));
        if (bytes_recv == fin_len){
            // if it's the right size, actually receive the message and compare
            bytes_recv = recv(connectfd, buffer, MAXSIZE, 0);
            if (strncmp(buffer + bytes_recv - fin_len, FIN_MSG, fin_len) == 0){
                gettimeofday(&end_time, NULL);
                // Send ACK message
                if (send(connectfd, ACK_MSG, strlen(ACK_MSG), 0) == -1) {
                    PrintError("server send", true);
                }
                break;
            }
            // more of a backup, if it's not the right size, add the bytes to the total
            else {
                total_bytes += bytes_recv;
            }
        }
        // if it's not FIN, keep loading in the chunks
        else {
            bytes_recv = recv(connectfd, buffer, MAXSIZE, MSG_WAITALL);
            if (bytes_recv == -1) {
                PrintError("server recv", true);
                break;
            }
            else if (bytes_recv == 0) {
                // client has closed connection
                gettimeofday(&end_time, NULL);
                break;
            }
            total_bytes += bytes_recv;
        }
    }

    CalculateRate(total_bytes, start_time, end_time, isServer);
}

// Calculate and Print Data Transfer Rate
void CalculateRate(const long total_bytes, const struct timeval start, const struct timeval end, const bool isServer) {
    // calculate duration of transfer in seconds (takes into account microseconds)
    double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    // convert total bytes to bits
    double total_bits = total_bytes * 8.0;
    // calculate rate in Mbps
    double rateMbps = total_bits / (1000000.0 * duration);
    // convert total bytes to KB
    long total_kilobytes = total_bytes / 1000;

    if (isServer) {
        printf("Received=%ld KB", total_kilobytes);
    } else {
        printf("Sent=%ld KB", total_kilobytes);
    }
    // print the rate with three digits after decimal
    printf(", Rate=%.3lf Mbps\n", rateMbps);
}

// Initialize Client Socket
int InitializeClient(const std::string hostname, const int server_port) {
    int sockfd;

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPv4, TCP
    if (sockfd == -1) {
        PrintError("client socket", true);
        return -1;
    }

    // set up server address
    const char* host_temp = hostname.c_str(); // convert host string to const char*
    struct hostent* server_host = gethostbyname(host_temp); // server hostname
    struct sockaddr_in server_addr; // struct for IPv4 address
    server_addr.sin_family = AF_INET;
    memcpy(&(server_addr.sin_addr), server_host->h_addr, server_host->h_length);
    server_addr.sin_port = htons(server_port);

    // connect to server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        PrintError("client connect", true);
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Send Data from Client
void SendData(const int sockfd, const double timer, const bool isServer) {
    char bufsend[MAXSIZE] = {"0"};
    char bufrecv[MAXSIZE];
    int bytes_sending;
    long total_bytes = 0;
    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    while(true) {
        // update end time and calculate if time is up
        gettimeofday(&end_time, NULL);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
        if (elapsed >= timer) break;

        // send data
        bytes_sending = send(sockfd, bufsend, MAXSIZE, 0);
        if (bytes_sending == -1) {
            PrintError("client send", true);
            break;
        }
        total_bytes += bytes_sending;
    }

    // Send termination signal
    int send_result = send(sockfd, FIN_MSG, strlen(FIN_MSG), 0);
    if (send_result == -1) {
        PrintError("client send", true);
    }

    CalculateRate(total_bytes, start_time, end_time, isServer);
    
    // Wait for ACK
    int bytes_recieving;
    while (true) {
        bytes_recieving = recv(sockfd, bufrecv, MAXSIZE, 0);
        if (bytes_recieving == -1) {
            PrintError("client recv", true);
            break;
        }
        else if (bytes_recieving == 0) {
            // Server has closed connection
            break;
        }

        bufrecv[bytes_recieving] = '\0';  // Null-terminate received data
        if (strcmp(bufrecv, ACK_MSG) == 0) {
            // ACK received
            break;
        }
    }
    
}


// Print Error Messages
void PrintError(const char* msg, bool perrorFlag) {
    if (perrorFlag) {
        perror(msg);
    } else {
        std::cerr << msg << std::endl;
    }
}


