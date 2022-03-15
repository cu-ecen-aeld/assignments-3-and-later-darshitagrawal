/**
 * Filename: systemcalls.c
 * Author: Darshit Nareshkumar Agrawal
 * Compiler: gcc
 * Date: 01/26/2022
 * References: 1) https://stackoverflow.com/questions/14543443/in-c-how-do-you-redirect-stdin-stdout-stderr-to-files-when-making-an-execvp-or
 		2) Linux Programming Interface by Michael Kerrisk
 		3) Linux System Programming by Robert Love.
 		
 */
 
#include <stdio.h>													/*Header Files*/
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{
    openlog(NULL, 0, LOG_USER);
    int system_return;
    system_return = system(cmd);
    if(cmd == NULL)													/*Check the return value for cmd == NULL*/						
    {
        if(system_return == 0)
	{
	    syslog(LOG_USER | LOG_ERR, "\nNo shell is available.");
	    return false;
	}
	else if(system_return != 0)
	{
	    syslog(LOG_USER | LOG_ERR, "\nShell is available.");
	    return false;
	}
    }
    if(system_return == -1)												/*Error*/
    {
        syslog(LOG_USER | LOG_ERR, "Child process could not be created or its termination process could not be retrieved. Error: %s",strerror(errno));
	return false;
    }
    if(system_return > 0)												/*Error*/
    {
        syslog(LOG_USER | LOG_ERR, "Call has returned non-zero value.");
	return false;
    }

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    
    openlog(NULL, 0, LOG_USER);
    
    pid_t pid, result_wait;
    int status;
    pid = fork();
    if(pid == -1)														/*Error*/
    {
        syslog(LOG_USER | LOG_ERR, "Child process is not created. Error: %s",strerror(errno));
        return false;
    }
    if(!pid)															/*Running execv() under child process*/
    {
    	execv(command[0], command);
    	exit(EXIT_FAILURE);
    }
    
    result_wait = waitpid(pid, &status, 0);
    if(result_wait == -1)													/*Error*/
    {
        syslog(LOG_USER | LOG_ERR, "Error in waiting for the termiantion process. Error: %s", strerror(errno));
        return false;
    }
    
    if(WIFEXITED(status))													/*Checking if child process terminated normally*/
    {
        syslog(LOG_USER | LOG_DEBUG, "Child exited, status: %d", WEXITSTATUS(status));
        if(WEXITSTATUS(status) != 0)
        {
            return false;
        }
    }
    else if(WIFSIGNALED(status))												/*Checking if the child process was terminated by the signal*/
    {
        syslog(LOG_USER | LOG_DEBUG, "Child killed by signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
#ifdef WCOREDUMP
    if(WCOREDUMP(status))													/*Checking if the child process produced a core-dump*/
    {
        syslog(LOG_USER | LOG_DEBUG, "Core Dumped.");
    }
#endif
    else if(WIFSTOPPED(status))												/*Checking if the child process was stopped by a signal*/
    {
        syslog(LOG_USER | LOG_DEBUG, "Child stopped by signal %d (%s)", WSTOPSIG(status), strsignal(WSTOPSIG(status)));
    }
#ifdef WIFCONTINUED
    if(WIFCONTINUED(status))													/*Checking if the child process was resumed by a signal*/
    {
        syslog(LOG_USER | LOG_DEBUG, "Child continued.");
    }
#endif

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/

    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    
    openlog(NULL, 0, LOG_USER);
    int fd, result_dup, status;
    pid_t pid, result_wait;
    fd = open(outputfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(fd == -1)												
    {
        syslog(LOG_USER | LOG_ERR, "File cannot be opened. Error: %s", strerror(errno));
        return false;
    }
    pid = fork();
    if(pid == -1)
    {
        syslog(LOG_USER | LOG_ERR, "Child process is not created. Error: %s",strerror(errno));
        return false;
    }
    else if(!pid)
    {
        result_dup = dup2(fd, STDOUT_FILENO);												/*Redirecting stdout to current file descriptor*/
        if(result_dup == -1)
        {
            syslog(LOG_USER | LOG_ERR, "File duplication failed.");
            return false;
        }
        close(fd);															/*Closing the file descriptor before execv()*/
        execv(command[0], command);
        exit(EXIT_FAILURE);
    }
    
    close(fd);
    result_wait = waitpid(pid, &status, 0);
    if(result_wait == -1)									
    {
        syslog(LOG_USER | LOG_ERR, "Error in waiting for the termiantion process. Error: %s", strerror(errno));
        return false;
    }
    
    if(WIFEXITED(status))
    {
        syslog(LOG_USER | LOG_DEBUG, "Child exited, status: %d", WEXITSTATUS(status));
        if(WEXITSTATUS(status) != 0)
        {
            return false;
        }
    }
    else if(WIFSIGNALED(status))
    {
        syslog(LOG_USER | LOG_DEBUG, "Child killed by signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
#ifdef WCOREDUMP
    if(WCOREDUMP(status))
    {
        syslog(LOG_USER | LOG_DEBUG, "Core Dumped.");
    }
#endif
    else if(WIFSTOPPED(status))
    {
        syslog(LOG_USER | LOG_DEBUG, "Child stopped by signal %d (%s)", WSTOPSIG(status), strsignal(WSTOPSIG(status)));
    }
#ifdef WIFCONTINUED
    if(WIFCONTINUED(status))
    {
        syslog(LOG_USER | LOG_DEBUG, "Child continued.");
    }
#endif

       
    
    
/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/

    va_end(args);
    
    return true;
}
