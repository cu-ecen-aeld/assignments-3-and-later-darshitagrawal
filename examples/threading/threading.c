#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    int usleep_result;
    int mx_result;
    
    struct thread_data* thread_func_args = (struct thread_data*)thread_param;
    usleep_result = usleep(thread_func_args->wait_to_obtain_ms * 1000);
    if(usleep_result == -1)
    {
        ERROR_LOG("\nusleep() execution failed. Value returned is %d",usleep_result);
        perror("Error:");
    }
    mx_result = pthread_mutex_lock(thread_func_args->mutex);
    if(mx_result)
    {
        errno = mx_result;
        ERROR_LOG("\nMutex could not be locked. Error: %s", strerror(errno));
    }
    usleep_result = usleep(thread_func_args->wait_to_release_ms * 1000);
    if(usleep_result == -1)
    {
        ERROR_LOG("\nusleep() execution failed. Value returned is %d",usleep_result);
        perror("Error:");
    }
    mx_result = pthread_mutex_unlock(thread_func_args->mutex);
    if(mx_result)
    {
        errno = mx_result;
        ERROR_LOG("\nMutex could not be unlocked. Error: %s", strerror(mx_result));
    }
    thread_func_args->thread_complete_success = true;
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    bool create_result;
    struct thread_data* t_data = (struct thread_data*)malloc(sizeof(struct thread_data));
    t_data->mutex = mutex;
    t_data->wait_to_obtain_ms = wait_to_obtain_ms;
    t_data->wait_to_release_ms = wait_to_release_ms;
    t_data->thread_complete_success = false;
    
    create_result = pthread_create(thread, NULL, threadfunc, (void*)t_data);
    if(!create_result)
    {
        DEBUG_LOG("\nThread created.");
        return true;
    }
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
    errno = create_result;
    ERROR_LOG("\nThread cannot be created. Error: %s",strerror(errno));
    return false;
}

