/*
Sources:
https://stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
https://codeforwin.org/2018/03/c-program-to-list-all-files-in-a-directory-recursively.html
https://stackoverflow.com/questions/5141960/get-the-current-time-in-c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <pthread.h> 
#include <time.h>

#define NUM_ROOMS 7

struct Room{
    int id;
    char* name;
    /*the number of outbound connections with other rooms */
    int numOutboundConnections;
    struct Room* outboundConnections[6];
    char* location;
}Room;

/*Global array of room structs*/
struct Room* rooms[NUM_ROOMS];

/******************************************
Get the time and write it to a file
*******************************************/
void* getTime(void* mutex) {
    /*https://stackoverflow.com/questions/5141960/get-the-current-time-in-c */
    time_t rawTime = time(NULL);
    struct tm* t = localtime(&rawTime);
    
    /*56 seems big enough*/
    char buffer[56];
    memset(&buffer, '\0', 56);
    /*https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm*/
    strftime(buffer, 56, "%l:%M %p, %A, %B %d, %Y", t);
    
    /*Lock the mutex until we know this
    operation is finished*/
    pthread_mutex_lock(mutex);
    /*Write the time to the file */
    FILE* fp = fopen("currentTime.txt", "w");
    /*https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.htm*/
    fwrite(buffer, 1, 56, fp);
    fclose(fp);
    /*we're done and can release the lock now*/
    pthread_mutex_unlock(mutex);
}


/******************************************
Read the time file created in getTime and
print it out
*******************************************/
void showTime(pthread_mutex_t* t) {
  /*Set up and init a buffer*/
  char buffer[56];
  memset(&buffer, '\0', 56);

  /*Read the file that getTime made*/
  pthread_mutex_lock(t);
  FILE* fd = fopen("currentTime.txt", "r");
  fgets(buffer, 256, fd);
  buffer[strcspn(buffer, "\n")] = 0;
  printf("%s\n", buffer);
  
  /*Clean up by closing the file and
  unlocking the mutex*/
  fclose(fd);
  pthread_mutex_unlock(t);
}

/******************************************
Print all the rooms that have been visited
in this playthrough
*******************************************/
void showHistory(int* history, int steps){
    int i;
    for(i=0;i<steps; i++){
        printf("%s\n", rooms[history[i]]->name);
    }
}

/******************************************
Get user command as input
Does not validate
*******************************************/
char* getInput(){
    /*Set up a variable for the input*/
    char* input = malloc(sizeof(char) * 256);
    memset(input, '\0', 256);
    
    /*This asks for input*/
    if (fgets(input, 255, stdin)) {
        /*Clean up newlines*/
        input[strcspn(input, "\n")] = 0;
    }
    printf("\n");
    
    return input;
}
/******************************************
Find the starting node, return a pointer to it
*******************************************/
struct Room* findStart() {
  int i = 0;
  /*Use the NUM_ROOMS directive to iterate over
  the rooms array to find the starting room*/
  while (strcmp(rooms[i]->location, "START_ROOM") != 0 && i < NUM_ROOMS) {
    i++;
  }
  return rooms[i];
}

/******************************************
Print all connections that a given Room has
*******************************************/
void showConnections(struct Room* room){
    int j = 0;
    printf("POSSIBLE CONNECTIONS: ");
    while (room->outboundConnections[j] != NULL && j < room->numOutboundConnections) {
        /*If this is not the last element*/
        if (room->outboundConnections[j + 1] != NULL && j + 1 < room->numOutboundConnections){
          /*printf("%s, ",room->outboundConnections[j]->name);*/
          printf("%s, ",room->outboundConnections[j]->name);
          /*Had some debugging issues here and this helped me find the source of the segfault*/
          fflush(stdout);
        }
        /*If this is the last element
        add a newline*/
        else{
          /*printf("%s.\n",room->outboundConnections[j]->name);*/
          printf("%s.\n",room->outboundConnections[j]->name);
          fflush(stdout);
    }
        j++;
        
    }
}

/******************************************
Given a file name, read the room in that
file into memory
*******************************************/
void readOneRoom(char* file, int i){
    /*Open the file for reading*/
    FILE* fd = fopen(file, "r");
    /*Allocate memory for a room pointer,
    then actually point it at something*/
    struct Room* curRoom;
    curRoom = malloc(sizeof(struct Room));
    
    /*Read the first line of the file and get the name*/
    char buffer[256];
    char* name = (char*) malloc(sizeof(char) * 256);
    fgets(buffer, 256, fd);
    buffer[strcspn(buffer, "\n")] = 0;
    memset(name, '\0', sizeof(name));
    memcpy(name, &buffer[11], 255);
    curRoom->name = name;
    
    
    /*It took me an embarassingly long
    time to remember I needed to init the
    array of room pointers....
    Anyway, make space for each possible
    connection to another room that every
    room can have*/
    int g;
    for (g = 0; g < 6; g++) {
        curRoom->outboundConnections[g] = malloc(sizeof(struct Room));
    }
    
    /*Read the rest of the lines to get the other information*/
    int outbound = 0;
    while(fgets(buffer,256,fd)){
        buffer[strcspn(buffer, "\n")] = 0;
        /*For room connections, the : will be in the 13th spot*/
        if ((char) buffer[12] == ':') {
            /*We know this is the name of a room connection so we can
            grab it and store it*/
            char * curConn = (char*)malloc(sizeof(char)*256);
            memset(curConn, '\0', sizeof(curConn));
            memcpy(curConn, &buffer[14], 255);
            
            /*Set the outbound connection at index outbound to the current connection*/
            curRoom->outboundConnections[outbound]->name = curConn; 
            curRoom->numOutboundConnections++;
            outbound++;
        }
        /*For room location, the : will be in the 10th spot*/
        if ((char) buffer[9] == ':') {
            char* location = (char*) malloc(sizeof(char) * 256);
            memset(location, '\0', sizeof(location));
            memcpy(location, &buffer[11], 255);
            curRoom->location = location;
        }
    }
    fclose(fd);
    rooms[i] = curRoom;
    /*printf("From readOneRoom...\nRoom: %sLocation: %s\n\n", rooms[i]->name, rooms[i]->location)
    free(curRoom);;*/
}

/******************************************
Read the rooms in the newest directory
from files into memory
*******************************************/
void readRooms(){
    /*https://codeforwin.org/2018/03/c-program-to-list-all-files-in-a-directory-recursively.html*/
    /*General idea: Start in the current working directory
    Look for the dir that starts with 'hoveys.rooms.'
    and go in that one to look for the files*/
    
    char theGoodDir[20];
    char path[20];
    strcpy(path, "hoveys.rooms.");
    /*strcpy(path, "hoveys.rooms.1997");*/
    struct dirent *dp;
    DIR *dir = opendir(".");
    if(!dir){
        return;
    }
    
    /*Get each thing in the current directory*/
    while((dp = readdir(dir)) !=NULL){
        /*Search for the right directory
        Get the name of the current item and examine it*/
        char buffer[15];
        
        strcpy(buffer, dp->d_name);
        /*printf("%s\n", buffer);
        printf("%s\n", buffer);*/
        /*See if hoveys.rooms. is a substring of what's currently in the buffer*/
        if(strstr(buffer, path) != NULL){
            strcpy(theGoodDir, dp->d_name);
        }
        
        /*See if there's a newer version of this folder to look at*/
        struct stat stat1;
        struct stat stat2;
        if(!stat(theGoodDir,&stat1) || stat(dp->d_name, &stat2)){
            if(stat2.st_mtime > stat1.st_mtime){
                /*printf("This one is newer!");*/
                strcpy(theGoodDir, dp->d_name);
            }
        }
    }
    
    /*Now we know the name of the directory we want
    We can close this one and open the other*/
    closedir(dir);
    
    dir=opendir(theGoodDir);
    
    /*Now we can read each file in here
    and turn each file into an instance of Room*/
    int i;
    while((dp = readdir(dir)) !=NULL){
        /*Ignore the dots
        ok stop dots*/
        if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0)){
            char curFile[256]; 
            sprintf(curFile, "%s/%s", theGoodDir, dp->d_name);
            /*printf("The current file is: %s\n", dp->d_name);*/
            readOneRoom(curFile, i);
            i++;
        }
    }
    
    closedir(dir);
}

int main(){
    /*Load all the room files into memory*/
    readRooms();
    
    /*Set up history and steps*/
    int* history;
    history = malloc(sizeof(int) * 1024);
    int hist = 0;
    int steps = 0;
    pthread_mutex_t time = PTHREAD_MUTEX_INITIALIZER;
    
    struct Room* curRoom;
    curRoom = rooms[0];
    curRoom = findStart();

    
    /*Main loop for playing the game*/
    while(1){
        if(strcmp(curRoom->location, "END_ROOM")!=0){
            printf("CURRENT LOCATION: %s\n", curRoom->name);
            
            /*Show all the connections this room has*/
            showConnections(curRoom);
            
            printf("WHERE TO? >");
            char* input = getInput();
        
            
            /*Now that we have the user's command
            we can start figuring out what to do with it
            First, check for 'time'. This needs to be checked
            for first since we need to make a new thread for it*/
            if(strcmp(input, "time") == 0){
                /*printf("You asked for time\n");*/
                pthread_t timeThread;
                int result = pthread_create(&timeThread, NULL, getTime, &time);
                assert(result == 0);
                
                /*Mutex lock: lock this until the second
                thread is finished doing its thing*/
                result = pthread_join(timeThread, NULL);
                
                /*showTime made a file that we can now read from*/
                showTime(&time);
            }
            else{
                /*If they didnt ask for time, we can assume 
                they asked to navigate to a room*/
                int validRoom = 0;
                int j=0;
                int i=0;
                for (j=0; j<curRoom->numOutboundConnections;j++){
                    char curConnection[8];
                    strcpy(curConnection, curRoom->outboundConnections[j]->name);
                    if (strcmp(input, curConnection) == 0) {
                      validRoom = 1;
                      break;
                    }
                }
                if(validRoom == 0){
                    /*If the input doesn't match any known room names,
                    show an error, and ask them to try again*/
                    printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
                }
                /*The way this block is structured prevents the history
                and steps from being updated if an invalid room is given
                OR if the user asks for time*/
                else{
                     /*Now we can actually grab that room in memory and navigate to it*/
                    i=0;
                    /*Loop until we find the object in memory that matches input*/
                    while(strcmp(rooms[i]->name, input)!=0){
                        i++;
                    }
                    curRoom=rooms[i];
                    /*Add the selected room to the history tracker*/
                    int j=0;
                    /*Find the old room and */
                    while(strcmp(rooms[j]->name, curRoom->name)!=0){
                        j++;
                    }
                    /*Store the rooms index in history so it can
                    be accessed later without dealing with strings*/
                    history[hist] = j;
                    hist++;
                    steps++;
                }
                
            }
            
        }
        /*If this IS the end room:*/
        else{
            printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
            printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS: \n", steps);
            showHistory(history, steps);
            pthread_mutex_destroy(&time);
            break;
        }
        
    }

    free(history);
    return 0;
}