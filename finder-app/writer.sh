#!/bin/bash
#Assignment-1 - writer.sh
#Author: Darshit Nareshkumar Agrawal
#Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn't exist. Exits with value 1 and error print statement if the file could not be created.
#References : https://stackoverflow.com/questions/6121091/how-to-extract-directory-path-from-file-path

#Check if the number of arguments are correct.
if [ $# -ne 2 ]
then
	echo "Please specify correct number of arguments."
	exit 1
fi

#Load the variables with the respective arguments.
WRITEFILE="$1"
WRITESTR="$2"

#Create directory if does not exist.
mkdir -p "$(dirname "$WRITEFILE")"

#Checking if the directory successfully created.
if [ $? -eq 0 ]
then
	touch "$WRITEFILE"
	
	#Checking if file successfully created.
	if [ $? -eq 0 ]
	then 
		echo "File successfully created."
	else
		echo "File can not be created."
		exit 1
	fi
fi

#Writing the string contents to the file created.	
echo "$WRITESTR" > "$WRITEFILE"

exit 0

