#!/bin/bash
#Assignment-1 - finder.sh
#Author: Darshit Nareshkumar Agrawal
#Accepts two arguments. The first argument is a path to the directory on the filesystem and the second argument is a text string which will be searched within these files.


#Check if number of arguments are correct.
if [ $# -eq 0 ]
then
  echo "No arguments present."
  exit 1
elif [ $# -ne 2 ]
then
  echo "Please give correct number of arguments." 
  exit 1 
fi 

#Load the variables with the respective arguments.
FILESDIR=$1
SEARCHSTR=$2

#Check if the file directory exists.
if [ ! -d $FILESDIR ]
then
  echo "The entered directory does not represent a directory on the filesystem."
  exit 1 
fi 

#Calculating number of files.
#https://unix.stackexchange.com/questions/4105/how-do-i-count-all-the-files-recursively-through-directories/541267
NUM_FILES=$(find "$FILESDIR" -type f | wc -l)

#Calculate number of matching lines
NUM_MATCH_LINES=$(grep -r "$SEARCHSTR" "$FILESDIR" | wc -l)
echo "The number of files are $NUM_FILES and the number of matching lines are $NUM_MATCH_LINES"
exit 0
