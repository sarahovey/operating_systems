#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>

#define MAX_COMMAND_ARGS 512
#define MAX_COMMAND_CHARS 2048

int foregroundMode = 0;

void sigHandler(int incomingSignal)
{
  switch(incomingSignal) {
    case 2:
      signal(SIGINT, sigHandler);
      printf("terminated by signal %d\n", incomingSignal);
      fflush(stdout);
      break;
    case 20:
      signal(SIGTSTP, sigHandler);
      if (foregroundMode) {
        printf("Exiting foreground-only mode\n");
        foregroundMode = 0;
      } else {
        printf("Entering foreground-only mode (& is now ignored)\n");
        foregroundMode = 1;
      }
      fflush(stdout);
      break;
  }

}

/* Linear search for space character */
int findSpace(char* str, int pos) {
  int i;
  for (i = pos; i < strlen(str) - 1; i++) {
    if (str[i] == ' ') {
      return i;
    }
  }
  return -1;
}

/*************************************************
 * Given an array of pointers to strings, and the 
 * given command as a string, build the array with
 * each argument of the command
 * so 'mkdir newdir' in the array would be pointers to
 * 'mkdir' and 'newdir'
***************************************************/
void populateCommandArr(char** args, char* command) {
    //Get the length of the command
    int commandLength = strlen(command);
    
    //Set up some tracking variables
    int curArg = 0;
    int prev = 0;
    
    //Start at the index of the first occurance of a space
    int curPos = findSpace(command, 0);
    
    //Iterate through the string
    while(prev != -1) {
        int endPos;
        //If there is a space
        if(curPos != -1){
            endPos = curPos;
        }
        else{
            //the case where there are no spaces found
            endPos = commandLength;
        }
        if (prev != 0){
            prev++;
        } 
        char * s = (char *) malloc((endPos - prev) * sizeof(char));
        int i;
        for(i = 0; i < endPos - prev; i++) {
          s[i] = command[prev + i];
        }
        args[curArg] = s;
        curArg++;
        prev = curPos;
        //Look for the next space
        //This will be set to -1 if there is no more space to be found after
        //the given index
        curPos = findSpace(command, curPos + 1);
    }
}

int commandLen(char* cmd) {

  /* Get command length */
  /* Count the number of arguments */
  int n = 0;
  int i = 0;
  for (i = 0; i < strlen(cmd); i++) {
    if ((int)cmd[i] == 32) {
      n++;
    }
  }
  n += 2; /* There is always 1 NULL argument that is appended, and 1 uncounted
             argument */
  return n;
}

/*************************************************
 * Handle file redirection, c indicates if the
 * redirection is headed in or out
 * return -1 if there is no redirection for the given
 * direction; ie return -1 if there is no occurance
 * of '<' when asking for 'in'
***************************************************/
int searchPipe(int c, char* cmd) {
    //set the redirection symbol
    char pipe;
    if(c == 0){
      pipe = '>';
    }
    else{
      pipe = '<';
    }
    
    int i = 0;
    
    //Get the index of where the redirection symbol occurs
    while (i < strlen(cmd) - 1) {
        if (cmd[i] == pipe){
           return i; 
        } 
        i++;
    }
    //Return -1 if there is no redirection to be had
    return -1;
}

/*************************************************
 * Given a command, tokenize it and execute it
***************************************************/
void executeCommand(int endCmd, char*cmd){
    if(endCmd<INT_MAX){
        endCmd --;
    }
    else{
        endCmd = strlen(cmd)-1;
    }
    
    char* command = (char*) malloc(endCmd * sizeof(char));
    memcpy(command, cmd, endCmd);
    
    //Set up an array with the commands in it
    int n = commandLen(command);
    char* argv[n];
    populateCommandArr(argv, command);
    //Get rid of the last one since it's going to be 0
    argv[n-1] = NULL;
    
    //Now we actually execute the command
    if (execvp(argv[0], argv) == -1) {
        //Error
        printf("%s %s\n", argv[0], strerror(errno));
        fflush(stdout);
        exit(1);
    }
    exit(1);
}

/*************************************************
 * If there is to be redirection, this function
 * sets up an FD for the file(s) specified in
 * the command
***************************************************/
void setInOut(char* cmd, int reDirOut, int reDirIn) {
    //Mark the index of the end of the command
    int endCmd = INT_MAX;
    
    //If there is redirection of stdin
    if (reDirIn != -1) {
        endCmd = reDirIn;
        reDirIn += 2;
        //Get the index of the end of the command
        int end;
        if(findSpace(cmd, reDirIn)!= -1){
            end = findSpace(cmd, reDirIn);
        }
        else{
            end = strlen(cmd) - 1;
        }
        //With the index of the beginning and end of the command, we can 
        //isolate the file name in the string
        //give it a size
        char* name = (char*) malloc((end - reDirIn) * sizeof(char));
        //put the name in it
        memcpy(name, &cmd[reDirIn], (end - reDirIn));
        
        //make a new FD out of the filename
        int targetFD1 = open(name, O_RDONLY);
        if (targetFD1 < 0) {
          fprintf(stderr, "cannot open %s for input\n", name);
          exit(1);
        }
        if(dup2(targetFD1, STDIN_FILENO) == -1) {
          printf("Error!");
          fflush(stdout);
        }
        
        free(name);
    }
    
    //If there is output redirection
    if (reDirOut != -1) {
        endCmd = reDirOut < endCmd ? reDirOut : endCmd;
        if(reDirOut<endCmd){
            endCmd = reDirOut;
        }
        else{
            //do nothing, keep its value
            endCmd = endCmd;
        }
        //to adjust for zero-indexing
        reDirOut += 2;
        
        //get the name of the file 
        int end = findSpace(cmd, reDirOut) != -1 ? findSpace(cmd, reDirOut) : strlen(cmd) - 1;
        char* name = (char*) malloc((end - reDirOut) * sizeof(char));
        memcpy(name, &cmd[reDirOut], (end - reDirOut));
        
        /* Create FD and re-assign input */
        int targetFD2 = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (targetFD2 < 0) {
          fprintf(stderr, "cannot open %s for input\n", name);
          fflush(stdout);
          exit(1);
        }
        if(dup2(targetFD2, STDOUT_FILENO) == -1) {
          printf("Error!");
          fflush(stdout);
        }
        
        free(name);
    }
    
    //Execute the command
    executeCommand(endCmd, cmd);
    
}


void nullOutput() {
  //redirect to dev/null
  //https://stackoverflow.com/questions/19955260/what-is-dev-null-in-bash
  int targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  
  //check if the fd was successfully created
  if (targetFD < 0) {
    fprintf(stderr, "unable to open/dev/null\n");
    fflush(stdout);
    exit(1);
  }
  //use dup2 to create a copy of the FD
  //dup2 allows us to specify the fd
  //https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
  if(dup2(targetFD, STDOUT_FILENO) == -1) {
    printf("Unable to copy file descriptor!");
    fflush(stdout);
  }
}

/*************************************************
 * start a child process for a given command
 * handles file redirection and putting a process
 * in the background
***************************************************/
int startProcess(char* cmd, int status, int childStatus, pid_t child){
    /* Call another process and manage it */
    int bg = 0;
    
    //Check if the character before the null terminator is a '&'
    //and if so, enter background mode
    if(cmd[strlen(cmd) - 2] == '&') {
        if (!foregroundMode){
            bg = 1;
        }
        //chop the '&' off the end
        cmd[strlen(cmd) - 2] = '\0';
    }
    //start the new process
    child = fork();
    //check for an error
    if(child == -1) {
        perror("Hull Breach!");
        status = -1;
    } 
    //if the child successfully forked
    else if (child == 0) {
        //check for background mode
        if(bg == 1){
            //go into dev/null
            nullOutput();
        }
        
        //figure out if there is file redirection in the command
        //check for out
        int reDirOut = searchPipe(0, cmd);
        //check for in
        int reDirIn = searchPipe(1, cmd);
        
        //Now that we know if there is redirection or not,
        
        setInOut(cmd, reDirOut, reDirIn);
        
    } 
    else {
        if(bg) {
            //If we're in background mode, wait for the child to finish
            printf("background pid is %i\n", (int) child);
            waitpid(child, &childStatus, WNOHANG);
        } 
        else {
          waitpid(child, &status, 0);
        }
    }
    return status;
}


/***************************************************************************
 * if the given string contains '$$' at the end, return a string with
 * '$$' replaced by the pid of the process
***************************************************************************/
char* pidExpansion(char* line){
    int length = strlen(line);
    if(line[length-2] == '$' && line[length-3] == '$'){
        //get rid of the $$ at the end
        line[strlen(line)-3] = 0;
        //get the pid
        int pid = getpid();
        //should be max character
        char buffer[MAX_COMMAND_CHARS];
        //mash em together, the concatenated string lives in buffer now
        sprintf(buffer, "%s%d", line, pid);
        strcpy(line, buffer);
        fflush(stdout);
    }
    
    return line;
}


int parseCommand(char* cmd){
    int status = 0;
    //Look if the line is a comment
    if (cmd[0] == '#' || strlen(cmd) == 1){
        //do nothing
        return 1;
    }
    //Look for builtins
    if (strcmp(cmd, "exit\n") == 0){
      return 2;  
    }
    else if (strncmp(cmd, "status", 6) == 0) {
      printf("exit value %d\n", status / 256);
      fflush(stdout);
      return 1;
    }
    else if(strncmp(cmd, "cd", 2) == 0) {
      char token = '\n';
      strtok(&cmd[3], &token);
      //Change directory to home if no argument given
      if (strlen(&cmd[3]) == 0){
          chdir(getenv("HOME"));
      } 
      else {
        //If there IS a dir given, then attempt to
        //change the cwd to that dir
        if (chdir(&cmd[3]) == -1) {
          printf("%s: %s\n", cmd, strerror(errno));
          fflush(stdout);
          status = -1;
        }
      }
      return 1;
    } 
    else{
        return 0;
    }

}


char* getCommand(){
    printf(": ");
    fflush(stdout);
    char *line = NULL;
    ssize_t bufsize = 0;
    //get line will allocate memory for us bc we've passed it 0
    getline(&line, &bufsize, stdin);
    line = pidExpansion(line);
    return line;
}

int main() {
  signal(SIGINT, sigHandler);
  signal(SIGTSTP, sigHandler);

  //Buffer for intaking user input
  char* cmd;
  size_t bufSize = MAX_COMMAND_CHARS;
  cmd = (char *) malloc(MAX_COMMAND_CHARS * sizeof(char));
  
  
  pid_t child;
  int status = 0;
  int childStatus = -5;

  //Loop until the user quits
  int quit = 0;
  while (!quit) {
      
    /* Check to see if child exited */
    waitpid(-1, &childStatus, WNOHANG);
    if (childStatus != -5 && WIFEXITED(childStatus)) {
      printf("background pid %d is done: exit value %d\n", child, WEXITSTATUS(childStatus));
      childStatus = -5;
      fflush(stdout);
    }
    
    //Prompt user for command
    cmd= getCommand();
    //get a 0 back if there was no builtin or comment
    //get a 1 back if there was
    int isBuiltin = 0;
    isBuiltin = parseCommand(cmd);
    
    if(isBuiltin == 2){
        //exit
        return 0;
    }
    //If we get a 1 back, we can re-do the loop
    else if(isBuiltin == 1){
        continue;
    }
    //All other commands
    else {
        status = startProcess(cmd, status, childStatus, child);
    }
    if (status < 0) {
      printf("Exiting...\n");
      fflush(stdout);
      return status;
    }
  }

  free(cmd);
  return 0;
}