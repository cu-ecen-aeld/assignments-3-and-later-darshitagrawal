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
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>

#define BACKLOG 5
#define PORT_NUM "9000"

int cfd;
int sfd = -1;
struct addrinfo *result;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
bool signal_indication = false;

struct thread_data
{
    pthread_t thread_id; 
    int cfd;
    pthread_mutex_t* mutex;
    struct sockaddr_in addr; 
    bool thread_completion;
}; 

struct slist_data_s
{
    struct thread_data thread_params;
    SLIST_ENTRY(slist_data_s) entries;
};

typedef struct slist_data_s slist_data_t;

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
    pthread_mutex_destroy(&lock);
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
    signal_indication = true;
    graceful_exit();
    exit(0);
}

void* start_routine(void* thread_params)
{
    int lock_status, fd, bytes_read, bytes_write;
    char buf[1024];

    struct thread_data* params = (struct thread_data*)thread_params;
    
    while(1)
    {
        bytes_read = read(params->cfd, buf, (1024));
        if (bytes_read < 0) 
        {
            syslog(LOG_USER | LOG_ERR, "Error in reading from the socket. Error: %s", strerror(errno));
            params->thread_completion = true;
            pthread_exit(NULL);
        }

        if (bytes_read == 0)
        {
            continue;
        }

        if (strchr(buf, '\n')) 
        {
           break;
        } 
    }

    fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_APPEND, 0766);
    if (fd == -1)
    {
        syslog(LOG_USER | LOG_ERR, "Error in opening the file. Error: %s", strerror(errno));
    }

    lseek(fd, 0, SEEK_END);
    
    lock_status = pthread_mutex_lock(params->mutex);
    if(lock_status)
    {
        syslog(LOG_USER | LOG_ERR, "Error in locking the mutex. Error: %s", strerror(lock_status));
        params->thread_completion = true;
        pthread_exit(NULL);
    }

    bytes_write = write(fd, buf, bytes_read);
    if(bytes_write == -1)
    {
        syslog(LOG_USER | LOG_ERR, "Error in writing to the file. Error: %s", strerror(errno));
        params->thread_completion = true;
        close(fd);
        pthread_exit(NULL);
    }

    lseek(fd, 0, SEEK_SET);

    lock_status = pthread_mutex_unlock(params->mutex);
    if(lock_status)
    {
        syslog(LOG_USER | LOG_ERR, "Error in unlocking the mutex. Error: %s", strerror(lock_status));
        params->thread_completion = true;
        pthread_exit(NULL);
    }

    close(fd);
    memset(buf,0, 1024);
    int offset = 0;

    while(1) 
    {
        fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_APPEND, 0766);
        if(fd < 0)
        {
            syslog(LOG_USER | LOG_ERR, "Error in opening the file. Error: %s", strerror(errno));
            continue; 
        }

        lseek(fd, offset, SEEK_SET);

        lock_status = pthread_mutex_lock(params->mutex);
        if(lock_status)
        {
            syslog(LOG_USER | LOG_ERR, "Error in locking the mutex. Error: %s", strerror(errno));
            params->thread_completion = true;
            pthread_exit(NULL);
        }
        
        bytes_read = read(fd, buf, 1024);

        lock_status = pthread_mutex_unlock(params->mutex);   
        if(lock_status)
        {
            syslog(LOG_USER | LOG_ERR, "Error in unlocking the mutex. Error: %s", strerror(lock_status));
            params->thread_completion = true;
            pthread_exit(NULL);
        }
        
        close(fd);
        if(bytes_read == -1)
        {
            syslog(LOG_USER | LOG_ERR, "Error in reading from the file. Error: %s", strerror(errno));
            continue;
        }

        if(bytes_read == 0)
        {
            break;
        }

        bytes_write = write(params->cfd, buf, bytes_read);

        if(bytes_write == -1)
        {
            syslog(LOG_USER | LOG_ERR, "Error in writing to the file. Error: %s", strerror(errno));
            continue;
        }
        offset += bytes_write;
    }
    params->thread_completion = true;
    pthread_exit(NULL);
}

void *timer_routine(void *args)
{
  struct tm *localTime;
  struct timespec tp = {0, 0};
  size_t length;
  time_t timep;
  int time_count = 10;
  int bytes_write;

  while (signal_indication == false)
  {
    if (clock_gettime(CLOCK_MONOTONIC, &tp), 0)
    {
      syslog(LOG_USER | LOG_ERR, "Error: %s", strerror(errno));
      continue;
    }

    if ((--time_count) <= 0)
    {
        char buf[100] = {0};
        time(&timep);         
        localTime = localtime(&timep); 
        length = strftime(buf, 100, "timestamp:%a, %d %b %Y %T %z\n", localTime);

        int fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_APPEND, 0766);
        if (fd == -1)
        {
            syslog(LOG_USER | LOG_ERR, "Error in opening the file. Error: %s", strerror(errno));
        }

        int lock_status = pthread_mutex_lock(&lock);
        if(lock_status)
        {
            syslog(LOG_USER | LOG_ERR, "Error in locking the mutex. Error: %s", strerror(lock_status));
            close(fd);
        }
        lseek(fd, 0, SEEK_END);

        bytes_write = write(fd, buf, length);
        if (bytes_write < 0)
        {
            syslog(LOG_USER | LOG_ERR, "Error in writing to the given file. Error: %s",strerror(errno));
        }
        syslog(LOG_USER | LOG_INFO, "Timestamp : %s", buf);

        lock_status = pthread_mutex_unlock(&lock);
        if(lock_status)
        {
            syslog(LOG_USER | LOG_ERR, "Error in unlocking the mutex. Error: %s", strerror(lock_status));
            close(fd);
        }
        close(fd);
        time_count = 10;
    }

    tp.tv_sec += 1; 
    tp.tv_nsec += 1000000;
    if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) != 0)
    {
        if (errno == EINTR)
        {
            break;
        }
    }     
  }
  pthread_exit(NULL);
}

int main(int argc, char **argv) 
{
    slist_data_t *thread_node = NULL;
    char dst_str[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    struct addrinfo hints;
    socklen_t addr_len = sizeof(addr);
    bool daemonize = false;
    
    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);
    
    openlog("aesdsocket", 0, LOG_USER);
    
    sig_t rs = signal(SIGINT, signal_handler);
    if (rs == SIG_ERR) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in registering SIG_ERR.");
        graceful_exit();
        exit(1);
    }

    rs = signal(SIGTERM, signal_handler);
    if (rs == SIG_ERR) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in registering SIGTERM.");
        graceful_exit();
        exit(1);
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
    
    int fd = creat("/var/tmp/aesdsocketdata", 0766);
    if(fd == -1)
    {
        syslog(LOG_USER | LOG_ERR, "Error in creating the file. Error: %s", strerror(errno));
        graceful_exit();
        exit(1);
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
        freeaddrinfo(result);
        graceful_exit();
        return -1;
    }

    sfd = socket(result->ai_family, SOCK_STREAM, 0);
    if (sfd == -1) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in creating the socket. Error: %s", strerror(errno));
        freeaddrinfo(result);
        graceful_exit();
        return -1;
    }

    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in setting up socket options. Error: %s", strerror(errno));
        freeaddrinfo(result);
        graceful_exit();
        return -1;
    }

    if(bind(sfd, result->ai_addr, result->ai_addrlen) == -1) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in binding the socket. Error: %s", strerror(errno));
        freeaddrinfo(result);
        graceful_exit();
        return -1;
    }
    
    //freeaddrinfo(result);

    if(listen(sfd, BACKLOG) == -1) 
    {
        syslog(LOG_USER | LOG_ERR, "Error in making the socket passive. Error: %s", strerror(errno));
        graceful_exit();
        return -1;
    }

    if(daemonize == true) 
    {
        int daemon_result = daemon(0,0);
        if(daemon_result)
        {
            syslog(LOG_USER | LOG_ERR, "Error in daemonizing. Error: %s", strerror(errno));
            graceful_exit();
            return -1;
        }
    }
    
    pthread_t tid; 
    pthread_create(&tid, NULL, timer_routine, NULL);

    while(signal_indication == false) 
    {
        cfd = accept(sfd, (struct sockaddr*)&addr, &addr_len);

        if(signal_indication)
        {
            break;
        }

        if(cfd == -1)
        {
            syslog(LOG_USER | LOG_ERR, "Error in accepting new connection. Error: %s", strerror(errno));
            graceful_exit();
            exit(1);
        }
        
        inet_ntop(AF_INET, &(addr.sin_addr), dst_str, INET_ADDRSTRLEN);

        syslog(LOG_USER | LOG_INFO, "Accepted Connection from %s",dst_str);
        
        thread_node = (slist_data_t *) malloc(sizeof(slist_data_t));
        SLIST_INSERT_HEAD(&head, thread_node, entries);

        thread_node->thread_params.cfd = cfd;
        thread_node->thread_params.addr = addr;
        thread_node->thread_params.mutex = &lock;
        thread_node->thread_params.thread_completion = false;

        pthread_create(&(thread_node->thread_params.thread_id), NULL, start_routine, (void*)&thread_node->thread_params);

        SLIST_FOREACH(thread_node,&head,entries)
        {    
            if(thread_node->thread_params.thread_completion == true)
            {
                pthread_join(thread_node->thread_params.thread_id,NULL);
                shutdown(thread_node->thread_params.cfd, SHUT_RDWR);
                close(thread_node->thread_params.cfd);
                syslog(LOG_USER | LOG_INFO, "Thread with tid %d has closed its connection.", (int)thread_node->thread_params.thread_id);
            }
        }
    }

    pthread_join(tid, NULL);

    while (!SLIST_EMPTY(&head)) 
    {
        thread_node = SLIST_FIRST(&head);
        pthread_cancel(thread_node->thread_params.thread_id);
        SLIST_REMOVE_HEAD(&head, entries);
        free(thread_node); 
        thread_node = NULL;
    }

    unlink("/var/tmp/aesdsocketdata");
    if (access("/var/tmp/aesdsocketdata", F_OK) == 0)  
    {
       remove("/var/tmp/aesdsocketdata");
    }
    graceful_exit();

    exit(0);
}

