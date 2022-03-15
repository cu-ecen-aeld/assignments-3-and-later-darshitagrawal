/**
 * Filename: writer.c
 * Description: Copies a string provided in the argument by the user into a file, name of which is provided by the user. 
 * Author: Darshit Nareshkumar Agrawal
 * Compiler: gcc
 * Date: 01/20/2022
 * References: 1) https://man7.org/linux/man-pages/man3/basename.3.html
 		2) https://www.tutorialspoint.com/cprogramming/c_error_handling.htm*
 		3) Linux System Programming by Robert Love.
 */

#include <stdio.h>										/*Header Files*/
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	openlog(NULL, 0, LOG_USER);								/*Open connection to system logger*/

	if(argc != 3)										/*Check if correct number of arguments are provided*/
	{											/*Logging error and diagnostic messages*/
		syslog(LOG_USER | LOG_ERR, "Please enter correct number of arguments. Entered arguments is %d. Required arguments is 3.", argc);
		syslog(LOG_USER | LOG_INFO, "First argument should be the full path to a file, including filename, on the filesystem.");
		syslog(LOG_USER | LOG_INFO, "Second argument should be the text string which will be written within this file.");
		exit(1);
	}

	int fd;
	ssize_t write_return;
	char *writestr = argv[2];
	int str_len = strlen(writestr);
	char *write_file = basename(argv[1]);							/*Extracting the filename from the argument*/
	syslog(LOG_USER | LOG_INFO, "Filename is %s", write_file);				/*Logging info message*/
	fd = open(argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);				/*Opening/ Creating a file*/
	if(fd == -1)
	{
		syslog(LOG_USER | LOG_ERR,"ERROR: %s",strerror(errno));			/*Log error message if fail to open*/
		exit(1);
	}

	while(str_len != 0 && (write_return = write(fd, writestr, str_len)) != 0) 		/*Writing to a file and taking care of partial write*/
	{
		if(write_return == -1)
		{
			if(errno == EINTR)
			{
				continue;							/*Continue if signal occured before any byte write*/
			}
			syslog(LOG_USER | LOG_ERR, "Error: %s", strerror(errno));		/*Log error message if fail to write*/
			exit(1);
		}
		str_len -= write_return;							/*Update string-length after partial-write*/
		writestr += write_return;							/*Update string pointer after partial-write*/
	}
													
	if(strlen(writestr) >= SSIZE_MAX)							
	{
		write_return = SSIZE_MAX;							/*Returning total bytes write equal to Max. size if string length is beyond store capacity*/
	}

	syslog(LOG_USER | LOG_DEBUG, "Writing %s to %s", argv[2], basename(argv[1]));	/*Log debug message*/

	if(close(fd) == -1)									/*Close the opened file*/
	{
		syslog(LOG_USER | LOG_ERR, "Error: %s", strerror(errno));
		exit(1);
	}
	return 0;
}


