#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>


#define PORT "8000"
#define HOST "127.0.0.1"
#define MAX_LISTEN 5
#define SIZE 512


void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int send_sock_msg(int sock_fd){
    struct sockaddr_in receiver_addr;

    char line[15] = "Hello World!";
    struct msghdr msg;
    struct iovec iov;

    //sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    receiver_addr.sin_port = htons(8000);
    
    msg.msg_name = &receiver_addr;
    msg.msg_namelen = sizeof(receiver_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_iov->iov_base = line;
    msg.msg_iov->iov_len = 13;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    return sendmsg(sock_fd, &msg, 0);
}

int main(void){

    int sd, new_sd;
    int yes = 1;
    int rv;
    int ttl = 8;
    char s[INET6_ADDRSTRLEN];

    struct sockaddr_in addr;
    struct addrinfo hints, *serverinfo, *p;
    struct sockaddr_storage their_addr; // COnnectors address info.
    socklen_t sin_size;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &serverinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through different results and pick up the first one.
    for( p = serverinfo; p != NULL; p = p->ai_next){

        if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("server: socket");
            continue;
        }

        if((fcntl(sd, F_SETFL, O_NONBLOCK)) < 0){
            perror("error:fnctl");
            exit(EXIT_FAILURE);
        }

        if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        if(bind(sd, p->ai_addr, p->ai_addrlen) == -1){
            close(sd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL){
        fprintf(stderr, "server: failed to bind \n");
        return 2;
    }

    freeaddrinfo(serverinfo); // No need for it further.
    
    if(listen(sd, MAX_LISTEN) == -1){
        perror("listen");
        exit(1);
    }


    printf("server: waiting for connections...\n");

    while(1){
        sin_size = sizeof their_addr;
        new_sd  = accept(sd, (struct sockaddr *)&their_addr, &sin_size);

        if(new_sd == -1){
            perror("accept ...");
            sleep(1);
            continue;
        }

        inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s);

        printf("server: got connection from %s \n", s);

        if(!fork()){ // This is child process.
            close(sd); // Child doesn't need the listner socket. I'm not sure thought.
            int count = 1;
            FILE *logfile = fopen("server.log", "w");

            if(logfile == NULL){
                perror("logfile open error");
                return(-1);
            }

            /*
             * Change the new client socket non-blocking.
             *
             * This will simulte the server scenario by writing the send buffer
             * as much as it can, and returns -1 if it's full. So that point the
             * server is reset the connection by closing the socket.
             *
             * So that's the current server behaviour in my environment. Check
             * the python client side, how it handling to avoid the connection
             * reset by the server.
             */
            if((fcntl(new_sd, F_SETFL, O_NONBLOCK)) < 0){
                perror("error:fnctl");
                exit(EXIT_FAILURE);
            }

            while(1){
                int numbytes;

                sleep(0.1);

                if((numbytes = send_sock_msg(new_sd)) < 0){
                    fprintf(logfile, "Kernel sending buffer is full. Closing the connection.: %d %d\n", count,numbytes);
                    close(new_sd);
                    exit(EXIT_FAILURE);
                }
                fprintf(logfile, "Server sending the msg no: %d %d\n", count,numbytes);
                count ++;
            }
            close(new_sd);
            exit(0);
        }
        close(new_sd); // Parent doesn't need child's socket.
    }

    return 0;
}
