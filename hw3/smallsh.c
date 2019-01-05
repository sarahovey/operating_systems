
/*************************************************
 * small sh
 * Sara Hovey
 * Fall 2018
***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

//Define the max sizes
#define MAX_COMMAND_ARGS 512
#define MAX_COMMAND_CHARS 2048

//Function declarations
int foregroundMode = 0;
void catchSignals(int incomingSignal);
int getSpaceIndex(char* str, int pos);
void populateCommandArr(char** args, char* command);
int wordsInCommand(char* cmd);
int isRedirection(int rdr, char* lineEntered);
void executeCommand(int endCmd, char*cmd);
void setupFileDescriptors(char* cmd, int pipeOut, int pipeIn);
void toDevNull();
int startProcess(char* cmd, int status, int childStatus, pid_t child);
char* pidExpansion(char* line);
int parseCommand(char* lineEntered);
char* getCommand();
//int gotSignal = 0;

int main() {
  
  //Set up signal catching
  signal(SIGINT, catchSignals);
  signal(SIGTSTP, catchSignals);

  //Buffer for intaking user input
  char* lineEntered;
  size_t bufSize = MAX_COMMAND_CHARS;
  lineEntered = (char *) malloc(MAX_COMMAND_CHARS * sizeof(char));
  
  //set up tracking child processes
  pid_t child;
  int status = 0;
  int childStatus = -5;

  //Loop until the user quits
  int quit = 0;
  while (!quit) {
  // Graveyard for my attempt to get a re-entrant signal handler working  
  // I read something that said the safest thing to do is to change a global
  // variable and handle the needed changes after calling the handler function
  //This works just fine with sigint but not with sigstp :/
    // //deal with signals here for re-entrancy
    // if(gotSignal > 0){
    //   if(gotSignal ==2){
    //     printf("terminated by signal %d\n", gotSignal);
    //     // fflush(stdout);
    //   }
    //   else if(gotSignal == 20){
    //     //Catch ctl+z/sigstp, check if we're in foreground mode or not
    //     //signal(SIGTSTP, catchSignals);
    //     if (foregroundMode) {
    //       printf("Exiting foreground-only mode\n");
    //       foregroundMode = 0;
    //     } 
    //     else {
    //       printf("Entering foreground-only mode (& is now ignored)\n");
    //       foregroundMode = 1;
    //     }
    //     fflush(stdout);
    //   }
    //   gotSignal = 0;
    // }
    //See if the child process is finished and print out if it is
    waitpid(-1, &childStatus, WNOHANG);
    if (childStatus != -5 && WIFEXITED(childStatus)) {
      printf("background pid %d is done: exit value %d\n", child, WEXITSTATUS(childStatus));
      childStatus = -5;
      fflush(stdout);
    }
    
    //Get user input
    lineEntered= getCommand();
    //get a 0 back if there was no builtin or comment
    //get a 1 back if there was
    int isBuiltin = 0;
    isBuiltin = parseCommand(lineEntered);
    
    //the case for an exit command
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
        status = startProcess(lineEntered, status, childStatus, child);
    }
    if (status < 0) {
      printf("Exiting...\n");
      fflush(stdout);
      return status;
    }
  }

  free(lineEntered);
  return 0;
}


/*************************************************
 * Catches and handles SIGINT and SIGSTP
***************************************************/
void catchSignals(int incomingSignal)
{
  //https://www.thegeekstuff.com/2012/03/catch-signals-sample-c-code
  if(incomingSignal ==2){
    //Catch ctl+c/sigint
      signal(SIGINT, catchSignals);
      printf("terminated by signal %d\n", incomingSignal);
      fflush(stdout);
  }
  else if(incomingSignal == 20){
     //Catch ctl+z/sigstp, check if we're in foreground mode or not
      signal(SIGTSTP, catchSignals);
      if (foregroundMode) {
        printf("Exiting foreground-only mode\n");
        foregroundMode = 0;
      } 
      else {
        printf("Entering foreground-only mode (& is now ignored)\n");
        foregroundMode = 1;
      }
      fflush(stdout);
  }

}

/*************************************************
 * Given an empty string, and the 
 * given command as a string, build the array with
 * each argument of the command into the empty string
 * so 'mkdir newdir' in the array would be pointers to
 * 'mkdir' and 'newdir'
***************************************************/
void populateCommandArr(char** args, char* command) {
    //Get the length of the command
    int commandLength = strlen(command);
    //Big idea:
    //iterate over the command string, pick individual words
    //out based on the position of spaces, and populate
    //the empty string(args) that we passed in with these
    //isolated strings
    
    //Set up some tracking variables
    int curArg = 0;
    int prev = 0; //this tracks the "first" index of the
    
    //the index of the first occurance of a space
    int curPos = getSpaceIndex(command, 0);
    
    //Iterate through the string until we can't find anymore spaces
    while(prev != -1) {
        int endPos;
        //If there is a space
        if(curPos != -1){
          //set the end point to the end of the first word, as marked by a space
            endPos = curPos;
        }
        else{
            //the case where there are no more spaces
            endPos = commandLength;
        }
        if (prev != 0){
            prev++;
        }
        //make a string the size of the current word we're looking at
        char * argArray = (char *) malloc((endPos - prev) * sizeof(char));
        int i;
        //Set the current index to the value of the command at prev+i, which is a word
        for(i = 0; i < endPos - prev; i++) {
          argArray[i] = command[prev + i];
        }
        //set the curArgth index of the empty string we passed in to the word we
        //just picked out of the command
        args[curArg] = argArray;
        curArg++;
        //set the front of the indeces we're working with to the last space we examined
        prev = curPos;
        
        //Look for the next space
        //This will be set to -1 if there is no more space to be found after
        //the given index, indicating the end of the command
        curPos = getSpaceIndex(command, curPos + 1);
    }
}

/*************************************************
 * Find a space after a given position
 * More string bashing that I'd care to do in C
***************************************************/
int getSpaceIndex(char* command, int index) {
  int i;
  //could have also done something like this
  //https://stackoverflow.com/questions/1479386/is-there-a-function-in-c-that-will-return-the-index-of-a-char-in-a-char-array/1479401
  //by giving the function only the part of the command
  //I want to examine, but this seemed more spelled-out to me
  for (i = index; i < strlen(command) - 1; i++) {
    if (command[i] == ' ') {
      return i;
    }
  }
  return -1;
}


/*************************************************
 * Get the number of words in a command
***************************************************/
int wordsInCommand(char* cmd) {
    int n = 0;
    int i = 0;
    //Iterate through the string and count each
    //occurance of a space
    for (i = 0; i < strlen(cmd); i++) {
        if ((int)cmd[i] == 32) {
            n++;
        }
    }
  //Account for zero indexing and the null terminator
  //this gives us the number of words
  n += 2; 
  return n;
}

/*************************************************
 * Handle file redirection, rdr indicates if the
 * redirection is headed in or out
 * return -1 if there is no redirection for the given
 * rdr; ie return -1 if there is no occurance
 * of '<' when asking for 'in'
***************************************************/
int isRedirection(int rdr, char* lineEntered) {
    //set the redirection symbol
    char direction;
    if(rdr == 0){
      direction = '>';
    }
    else{
      direction = '<';
    }
    
    int i = 0;
    
    //Get the index of where the redirection symbol occurs
    while (i < strlen(lineEntered) - 1) {
        if (lineEntered[i] == direction){
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
    
    //set up a buffer
    char* command = (char*) malloc(endCmd * sizeof(char));
    memcpy(command, cmd, endCmd);
    
    //Set up an array with the commands in it
    int n = wordsInCommand(command);
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
void setupFileDescriptors(char* cmd, int pipeOut, int pipeIn) {
    //Mark the index of the end of the command
    int endOfCommand = INT_MAX;
    
    //If there is redirection of stdin
    if (pipeIn != -1) {
        endOfCommand = pipeIn;
        pipeIn += 2;
        //Get the index of the end of the command
        int end;
        if(getSpaceIndex(cmd, pipeIn)!= -1){
            end = getSpaceIndex(cmd, pipeIn);
        }
        else{
            end = strlen(cmd) - 1;
        }
        //With the index of the beginning and end of the command, we can 
        //isolate the file fileName in the string
        //give it a size
        char* fileName = (char*) malloc((end - pipeIn) * sizeof(char));
        //put the fileName in it
        memcpy(fileName, &cmd[pipeIn], (end - pipeIn));
        
        //make a new FD out of the filename
        int newFD = open(fileName, O_RDONLY);
        if (newFD < 0) {
          //make sure nothing went wrong with creating the new FD
          fprintf(stderr, "cannot open %s for input\n", fileName);
          exit(1);
        }
        if(dup2(newFD, STDIN_FILENO) == -1) {
          printf("ERROR");
          fflush(stdout);
        }
        
        free(fileName);
    }
    
    //If there is output redirection
    if (pipeOut != -1) {
        if(pipeOut<endOfCommand){
            endOfCommand = pipeOut;
        }
        else{
            //do nothing, keep its value
            endOfCommand = endOfCommand;
        }
        //to adjust for zero-indexing
        pipeOut += 2;
        
        //get the fileName of the file 
        //int end = getSpaceIndex(cmd, pipeOut) != -1 ? getSpaceIndex(cmd, pipeOut) : strlen(cmd) - 1;
        int end;
        if(getSpaceIndex(cmd, pipeOut) != -1){
          end = getSpaceIndex(cmd, pipeOut);
        }
        else{
          end = strlen(cmd) -1;
        }
        
        char* fileName = (char*) malloc((end - pipeOut) * sizeof(char));
        memcpy(fileName, &cmd[pipeOut], (end - pipeOut));
        
        //Put output into this file
        int newFDOut = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (newFDOut < 0) {
          fprintf(stderr, "cannot open %s for input\n", fileName);
          fflush(stdout);
          exit(1);
        }
        if(dup2(newFDOut, STDOUT_FILENO) == -1) {
          printf("ERROR, unable to open file");
          fflush(stdout);
        }
        
        free(fileName);
    }
    
    //Execute the command
    executeCommand(endOfCommand, cmd);
}

/*************************************************
 * Use dup2 to copy a new fd that points to dev/null
 * this is for putting output into dev/null in bg mode
***************************************************/
void toDevNull() {
  //redirect to dev/null
  //https://stackoverflow.com/questions/19955260/what-is-dev-null-in-bash
  int fd = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  
  //check if the fd was successfully created
  if (fd < 0) {
    fprintf(stderr, "unable to open/dev/null\n");
    fflush(stdout);
    exit(1);
  }
  //use dup2 to create a copy of the FD
  //dup2 allows us to specify the fd
  //https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
  if(dup2(fd, STDOUT_FILENO) == -1) {
    printf("Error copying file descriptor from /dev/null");
    fflush(stdout);
  }
}

/*************************************************
 * Start a child process for a given command
 * Checks for background mode
***************************************************/
int startProcess(char* cmd, int status, int childStatus, pid_t child){
    int isBackground = 0;
    
    //Check if the character before the null terminator is a '&'
    //and if so, enter background mode
    if(cmd[strlen(cmd) - 2] == '&') {
        if (!foregroundMode){
            isBackground = 1;
        }
        //chop the '&' off the end since we've dealth with turning on bg mode
        cmd[strlen(cmd) - 2] = '\0';
    }
    //start the new process
    child = fork();
    //check for an error
    if(child == -1) {
        perror("Hull Breach - open() failed");
        status = -1;
    } 
    //if the child successfully forked
    else if (child == 0) {
        //check for background mode
        if(isBackground == 1){
            //go into dev/null
            toDevNull();
        }
        
        //figure out if there is file redirection in the command
        //check for out
        int reDirOut = isRedirection(0, cmd);
        //check for in
        int reDirIn = isRedirection(1, cmd);
        
        //Now that we know if there is redirection or not,
        //we can set up FDs for the specified file(s)
        setupFileDescriptors(cmd, reDirOut, reDirIn);
        
    } 
    else {
        if(isBackground) {
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
 * I miss python
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

/*************************************************
 * Given the command entered by the user,
 * do some initial triaging
 * Return 1 if the command included a builtin
 * Return 0 otherwise
 * If the caller gets  a 0 back, it knows to 
 * execute something that isnt a builtin
***************************************************/
int parseCommand(char* lineEntered){
    int status = 0;
    //Look if the line is a comment or if it's just a letter
    if (lineEntered[0] == '#' || strlen(lineEntered) == 1){
        //do nothing
        return 1;
    }
    //Look for builtins
    if (strcmp(lineEntered, "exit\n") == 0){
      return 2;  
    }
    else if (strncmp(lineEntered, "status", 6) == 0) {
      printf("exit value %d\n", status / 256);
      fflush(stdout);
      return 1;
    }
    else if(strncmp(lineEntered, "cd", 2) == 0) {
      char token = '\n';
      strtok(&lineEntered[3], &token); //my hair is going to fall out
      //Change directory to home as specified in the environment vars if no argument given
      if (strlen(&lineEntered[3]) == 0){
          chdir(getenv("HOME"));
      } 
      else {
        //If there IS a dir given, then attempt to
        //change the cwd to that dir
        if (chdir(&lineEntered[3]) == -1) {
          printf("%s: %s\n", lineEntered, strerror(errno));
          fflush(stdout);
          status = -1;
        }
      }
      return 1;
    } 
    else{
      //Return 0 to indicate that the command didn't include a comment or builtin
      return 0;
    }

}

/*************************************************
 * Does what it says on the tin
 * Gets user input
***************************************************/
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