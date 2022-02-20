/**
 * Filename: aesdsocket.c
 * Author: Darshit Nareshkumar Agrawal
 * Compiler: gcc
 * Date: 02/20/2022
 * References: 1) https://www.geeksforgeeks.org/socket-programming-cc/
 		2) https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
 		3) Linux Programming Interface by Michael Kerrisk.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

#define BACKLOG 5
#define PORT_NUM "9000"

int cfd; 
int sfd;
struct addrinfo  *result;

void graceful_exit()
{
    if(cfd > -1)
    {
        shutdown(cfd, SHUT_RDWR);
        close(cfd);
    }
    if(result != NULL)
    {
        freeaddrinfo(result);
    }
    if(sfd > -1)
    {
        shutdown(sfd, SHUT_RDWR);
        close(sfd);
    }
    unlink("/var/tmp/aesdsocketdata");
    if (access("/var/tmp/aesdsocketdata", F_OK) == 0)  
    {
       remove("/var/tmp/aesdsocketdata");
    }
    closelog();
}

static void signal_handler(int signal)
{
    switch(signal)
    {
        case SIGINT:
          syslog(LOG_USER  | LOG_INFO, "SIGINT Signal Detected.");
          break;
          
        case SIGTERM:
          syslog(LOG_USER | LOG_INFO, "SIGTERM Signal Detected.");
          break;
          
        default:
          syslog(LOG_USER | LOG_INFO, "Some Signal Detected.");
          break;
          
     }
    graceful_exit();
    exit(0);
}

int main(int argc, char **argv) 
{
    char dst_str[INET_ADDRSTRLEN];
    char buf[1024];
    struct sockaddr_in addr;
    struct addrinfo hints;
    socklen_t addr_len = sizeof(addr);
    bool daemonize = false;
    int bytes_read, bytes_write;
    
    openlog("aesdsocket", 0, LOG_USER);

    sig_t rs = signal(SIGINT, signal_handler);
    if (rs == SIG_ERR) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in registering SIG_ERR.");
        graceful_exit();
        return -1;
    }

    rs = signal(SIGTERM, signal_handler);
    if (rs == SIG_ERR) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in registering SIGTERM.");
        graceful_exit();
        return -1;
    }

    if (argc == 2) 
    {
        if (!strcmp(argv[1], "-d")) 
        {
            daemonize = true;
        } 
        else 
        {
            syslog(LOG_USER | LOG_ERR, "Invalid Arguments.");
            return (-1);
        }
    }
    
    int fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_CREAT, 0766);
    if(fd < 0)
    {
        syslog(LOG_USER | LOG_ERR, "Error in opening/ creating the file. Error: %s", strerror(errno));
        graceful_exit();
        return -1;
    }
    close(fd);

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int getaddrinfo_result = getaddrinfo(NULL, (PORT_NUM), &hints, &result);
    if (getaddrinfo_result != 0) 
    {
        syslog(LOG_USER | LOG_ERR, "ERROR: %s\n", gai_strerror(getaddrinfo_result));
        graceful_exit();
        return -1;
    }

    sfd = socket(result->ai_family, SOCK_STREAM, 0);
    if (sfd < 0) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in creating the socket. Error: %s", strerror(errno));
        graceful_exit();
        return -1;
    }

    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in setting up socket options. Error: %s", strerror(errno));
        graceful_exit();
        return -1;
    }

    if (bind(sfd, result->ai_addr, result->ai_addrlen) < 0) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in binding the socket. Error: %s", strerror(errno));
        graceful_exit();
        return -1;
    }

    if (listen(sfd, BACKLOG)) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in making the socket passive. Error: %s", strerror(errno));
        graceful_exit();
        return -1;
    }

    if (daemonize == true) 
    {
        int daemon_result = daemon(0,0);
        if(daemon_result)
        {
            syslog(LOG_USER | LOG_ERR, "Error in daemonizing. Error: %s", strerror(errno));
            graceful_exit();
            return -1;
        }
      }

    while(1) 
    {
        cfd = accept(sfd, (struct sockaddr*)&addr, &addr_len);
        if(cfd < 0)
        {
            syslog(LOG_USER | LOG_ERR, "Error in accepting new connection. Error: %s", strerror(errno));
            graceful_exit();
            return -1;
        }

        inet_ntop(AF_INET, &(addr.sin_addr), dst_str, INET_ADDRSTRLEN);

        syslog(LOG_USER | LOG_INFO, "Accepted Connection from %s",dst_str);

        while(1)
        {
            bytes_read = read(cfd, buf, 1024);
            if (bytes_read < 0) 
            {
                syslog(LOG_USER | LOG_ERR, "Error in reading from the socket. Error: %s", strerror(errno));
                continue; 
            }
            if (bytes_read == 0)
            {
                continue;
            }
            
            int fd = open("/var/tmp/aesdsocketdata",O_RDWR);
            if (fd < 0)
            {
                syslog(LOG_USER | LOG_ERR, "Error in opening the file. Error: %s", strerror(errno));
            }

            lseek(fd, 0, SEEK_END);

            bytes_write = write(fd, &buf[0], bytes_read);
            if(bytes_write < 0)
            {
                syslog(LOG_USER | LOG_ERR, "Errror in writing to the file. Error: %s", strerror(errno));
                close(fd);
                continue;
            }

            close(fd);

            if (strchr(&buf[0], '\n')) 
            { 
               break;
            } 
        }
        
        int offset = 0;
        
        while(1) 
        {
            int fd = open("/var/tmp/aesdsocketdata", O_RDWR);
            if(fd < 0)
            {
                syslog(LOG_USER | LOG_ERR, "Error in opening the file. Error: %s", strerror(errno));
                continue; 
            }

            lseek(fd, offset, SEEK_SET);
            bytes_read = read(fd, buf, 1024);
            
            close(fd);
            if(bytes_read < 0)
            {
                syslog(LOG_USER | LOG_ERR, "Error in reading from the file. Error: %s", strerror(errno));
                continue;
            }

            if(bytes_read == 0)
            {
                break;
            }

            bytes_write = write(cfd, &buf[0], bytes_read );

            if(bytes_write < 0)
            {
                syslog(LOG_USER | LOG_ERR, "Error in writing to the file. Error: %s", strerror(errno));
                continue;
            }

            offset += bytes_write;
        }
        close(cfd);
        syslog(LOG_USER | LOG_INFO, "Closed connection from %s", dst_str);
    }
    graceful_exit();
    return 0;

}
