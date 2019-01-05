#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_ROOMS 7

struct Room{
    int id;
    char name[8];
    /*the number of outbound connections with other rooms */
    int numOutboundConnections;
    char filename[12];
    /* array of pointers to rooms */
    struct Room* outboundConnections[5];
    char location[10];
}Room;

/*Global array of room structs*/
struct Room* rooms[10];
struct Room* chosenRooms[7];
char newDir[15];

/*Matrix that stores connections between rooms*/
int outboundConnections[7][7];


/******************************************
 * Given a room, find its index in the 
 * chosenRooms array
*******************************************/
int getIndex(struct Room* room){
    int i=0;
    int index1;
    /*Look for the element in the chosenRooms array with the
    same name as room, now we know room's index */
    for(i=0; i<7;i++){
        int result;
        result = strcmp(chosenRooms[i]->name, room->name);
        if(result == 0){
            index1 = i;
        }
    }
    
    return index1;
}

/******************************************
 * Given two rooms, formalize a connection
 * between the first and second room
 * A second inverted call to this
 * function will be needed to connect
 * room 2 to room 1
*******************************************/
void connectRooms(struct Room* room1, struct Room *room2){
    /*Add room 2 to the outboundConnections array
    of room1
    First, get the index of where the new connection
    will live by referring to the number of outbound
    connections associated with each room*/
    /*printf("In connectRooms at the top ..%s: %s\n",chosenRooms[5]->name, chosenRooms[5]->location);*/

    int newConnection1 = room1->numOutboundConnections;
    int newConnection2 = room2->numOutboundConnections;
    /*printf("Room1: %s, %s Room2: %s, %s\n", room1->name, room1->location, room2->name, room2->location);*/
    /*Increment numOutboundConnections of room2*/
    room1->outboundConnections[newConnection1] = room2; /*this is fucking bruce's location??*/
    /*printf("Room1: %s, %s Room2: %s, %s\n", room1->name, room1->location, room2->name, room2->location);
    printf("In connectRooms after setting a new outboundConnection ..%s: %s\n",chosenRooms[5]->name, chosenRooms[5]->location);*/
    
    /*Add this connection to the outboundConnections array*/
    int room1index;
    room1index= getIndex(room1);
    int room2index;
    room2index = getIndex(room2);
    /*printf("%d %d\n", room1index, room2index);*/
    outboundConnections[room1index][room2index] = 1;
    room1->numOutboundConnections++;
}

/******************************************
 * Check the matrix of outbound connections
 * at the index of A and B as specified in 
 * the chosenRooms array
*******************************************/
int connectionAlreadyExists(struct Room* A, struct Room *B){
    int i=0;
    int index1, index2;
    /*Look for the element in the chosenRooms array with the
    same name as A, now we know A's index */
    for(i=0; i<7;i++){
        int result;
        result = strcmp(chosenRooms[i]->name, A->name);
        if(result == 0){
            index1 = i;
        }
    }
    /*Look for the element in the chosenRooms array with the
    same name as B, now we know B's index */
    i=0;
    for(i=0; i<7;i++){
        int result;
        result = strcmp(chosenRooms[i]->name, B->name);
        if(result == 0){
            index2 = i;
        }
    }
    
    /*Now that we have the indeces of A and B as they are
    in the 'master' array, we can see if there is already
    a connection between them by checking the outboundConnections
    matrix at the with A as the first index and B as trhe second*/
    int element, element2;
    element = outboundConnections[index1][index2];
    element2 = outboundConnections[index2][index1];
    /*printf("Checking connection: %d, %d\n", element, element2);
    printf("Names: %s, %s\n", A->name, B->name);*/
    
    if(element == 1 || element2 == 1){
        return 1;
    }
    
    return 0;
}

/******************************************
 * Given two rooms, check to see if they
 * are the same room
 * Return 0 for false
 * Return 1 for True
*******************************************/
int isSameRoom(struct Room* A, struct Room* B){
    int result;
    result = strcmp(A->name, B->name);
    /*If the strings are the same, return 1*/
    if(result == 0){
        return 1;
    }
    
    return 0;
}

/******************************************
 * Randomly pick a room from the
 * chosenRooms array
 * Return a pointer to a struct Room
*******************************************/
struct Room* getRandomRoom(){
    int index1 = rand() % (6 + 1 - 0) + 0;
    struct Room* room = chosenRooms[index1];
    return room;
}

/******************************************
 * Given a room, check to see if the
 * room can accept more connections to it
*******************************************/
int canAddConnectionFrom(struct Room* room){
    if(room->numOutboundConnections < 6){
        /*Then this room can accept more connections*/
        return 1;
    }
    return 0;
}

/******************************************
 * Driver function to add a connection
 * between two random rooms
*******************************************/
void addRandomConnection(){
    
    int a_done = 0;
    
    /*Ok we have two different rooms now*/
    struct Room *A;
    struct Room *B;
    
    /*Generate room A and check its validity*/
    while(a_done == 0){
        A = getRandomRoom();
        /*printf("\nRandom room: %s\n", A->name);*/
        if(canAddConnectionFrom(A) == 1){
            /*We have confirmed that a random connection can be made and can break out of 
            the loop when we're done
            Otherwise, we'd keep generating new As until we got one that 
            can accept a new connection*/
            a_done = 1;
        }
        
    }
    int b_done = 0;
    while(b_done == 0){
        B = getRandomRoom();
        /*printf("\nB room: %s\n", B->name);*/
        /*Check if this room can accept more connections*/
        if(canAddConnectionFrom(B) == 1){
            if(isSameRoom(A,B) == 0){
                if(connectionAlreadyExists(A, B)== 0){
                    /***corruption is between here and end**/
                    /*Indicate that we can break out of the loop*/
                    b_done = 1;
                    
                    /*Make the connection between the two rooms*/
                    connectRooms(A,B);
                    connectRooms(B,A);
                    
                }
            }
        }
        
    }

    /*Finalize the changes by
    assigning A and B back to 
    their original rooms in the
    chosenRooms array*/
    int index1, index2;
    index1 = getIndex(A);
    index2 = getIndex(B);
    chosenRooms[index1] = A;
    chosenRooms[index2] = B;
    
    /*printf("Made a connection!");*/
}

/***************************************
Checks if the graph is full
*****************************************/
int IsGraphFull(){
    int i =0;
    for(i = 0; i<7; i++){
        /*If any rooms have fewer than 3 connections then the graph isn't full*/
        if(chosenRooms[i]->numOutboundConnections < 3){
            return 0;
        }
    }
    /*If none have fewer than 3, then the graph is full enough*/
    return 1;
}

/***************************************
Make connections between rooms
This function drives the process
I dont need to have chosenRooms as an arg here
I just want it as future reference
*****************************************/
void buildConnections(){
    int i=0;
    while (IsGraphFull() == 0){
        addRandomConnection();
        /*printf("After addRandomConnection..%s: %s\n",chosenRooms[5]->name, chosenRooms[5]->location);*/
    }
 }


/***************************************
Give each room a position: start, mid
or end
*****************************************/
void room_positions(){
    /*Generate two random numbers
    The first one is the index of the start room,
    the second is the index of the end room*/
    int different = 0;
    int index1, index2;
    while (different == 0){
        /*Pick two random structs*/
        index1 = rand() % (6 + 1 - 0) + 0;
        index2 = rand() % (6 + 1 - 0) + 0;
        if(index1 != index2){
            different = 1;
        }
    }
    
    /*chosenRooms[index1]->location = "START_ROOM";
    chosenRooms[index2]->location = "END_ROOM";*/
    strcpy(chosenRooms[index1]->location,"START_ROOM");
    /*printf("Index 2: %d\n", index2);*/
    strcpy(chosenRooms[index2]->location,"END_ROOM");
        
            
    /*The rest of the rooms are middle rooms*/
    int i;
    for (i=0; i<NUM_ROOMS; i++){
        if((strcmp(chosenRooms[i]->location, "EMPTY") == 0)){
            strcpy(chosenRooms[i]->location, "MID_ROOM");
        }
        
        /*printf("%s: %s\n",chosenRooms[i]->name, chosenRooms[i]->location);*/
    }
    
}


/***************************************
Write each room to its own file
*****************************************/
void printToFile(){
    int i,j, count;
    struct Room* cur_room;
    FILE *fp;
    char filename[11];
    char path[25];
    for(i=0;i<NUM_ROOMS;i++){
        count = 1;
        cur_room = chosenRooms[i];
        sprintf(filename, "%s",cur_room->filename);
        sprintf(path, "%s//%s", newDir, filename);
        fp = fopen(path, "w");
        /*Write a whole room*/
        
        int outbound = cur_room->numOutboundConnections;
        fprintf(fp, "ROOM NAME: %s\n", cur_room->name);
        
        for(j=0;j<outbound; j++){
            /*Write each room to a file*/
            fprintf(fp, "CONNECTION %d: %s\n", count, cur_room->outboundConnections[count-1]->name);
            count++;
        }
        /*printf("Making a file, here's the current location, %s\n", cur_room->location);*/
        /*printf("Making %s 's file, here's the current location, %s\n", cur_room->name, cur_room->location);*/
        if(strcmp(cur_room->location, "START_ROOM")!=0 && strcmp(cur_room->location, "MID_ROOM")!=0){
            strcpy(cur_room->location, "END_ROOM");
        }
        /*printf("Making a file, here's the current location After mods, %s\n", cur_room->location);*/
        fprintf(fp, "ROOM TYPE: %s", cur_room->location); 
    }
}

/***************************************
Make a new directory with the PID of
the current process
*****************************************/
void mkDir(){
    /*Make directory called hoveys.rooms.<processID of rooms program> */
    /*get process id of this process */
     int pid = getpid();
    /*make a new directory name */
    char* dirName = "hoveys.rooms.";
    char buffer[20];
    
    /*mash em together, the concatenated string lives in buffer now*/
    sprintf(buffer, "%s%d", dirName, pid);
    /*printf("%s", buffer);
    printf("%d\n", pid);*/
    /*result will be -1 if the operation failed*/
    int result = mkdir(buffer);
    /*printf("%d", result);*/
    strcpy(newDir, buffer);
}


int main(){
    int i = 0;
    int j = 0;
    
    /*Hardcode the 10 possible rooms + add them to the rooms array
    1. declare
    2. initialize
    3.set the name
    4. set filename
    5. set location
    6. add to the array*/
    struct Room mae;
    memset(&mae, 0, sizeof(struct Room));
    strcpy( mae.name, "mae");
    mae.numOutboundConnections = 0;
    strcpy(mae.filename, "mae.txt");
    strcpy(mae.location, "EMPTY");
    rooms[0] = &mae;
    
    struct Room gregg;
    memset(&gregg, 0, sizeof(struct Room));
    strcpy( gregg.name, "gregg");
    gregg.numOutboundConnections = 0;
    strcpy(gregg.location, "EMPTY");
    strcpy(gregg.filename,"gregg.txt");
    rooms[1] = &gregg;
    
    struct Room angus;
    memset(&angus, 0, sizeof(struct Room));
    strcpy( angus.name, "angus");
    angus.numOutboundConnections = 0;
    strcpy(angus.location, "EMPTY");
    strcpy(angus.filename, "angus.txt");
    rooms[2] = &angus;
    
    struct Room bea;
    memset(&bea, 0, sizeof(struct Room));
    strcpy( bea.name, "bea");
    strcpy(bea.location, "EMPTY");
    bea.numOutboundConnections = 0;
    strcpy(bea.filename, "bea.txt");
    rooms[3] = &bea;
    
    struct Room germ;
    memset(&germ, 0, sizeof(struct Room));
    strcpy( germ.name, "germ");
    strcpy(germ.location, "EMPTY");
    germ.numOutboundConnections = 0;
    strcpy(germ.filename, "germ.txt");
    rooms[4] = &germ;
    
    struct Room selmers;
    memset(&selmers, 0, sizeof(struct Room));
    strcpy( selmers.name, "selmers");
    strcpy(selmers.location, "EMPTY");
    selmers.numOutboundConnections = 0;
    strcpy(selmers.filename, "selmers.txt");
    rooms[5] = &selmers;
    
    struct Room casey;
    memset(&casey, 0, sizeof(struct Room));
    strcpy( casey.name, "casey");
    strcpy(casey.location, "EMPTY");
    casey.numOutboundConnections = 0;
    strcpy(casey.filename, "casey.txt");
    rooms[6] = &casey;
    
    struct Room lori;
    memset(&lori, 0, sizeof(struct Room));
    strcpy( lori.name, "lori");
    strcpy(lori.location, "EMPTY");
    lori.numOutboundConnections = 0;
    strcpy(lori.filename, "lori.txt");
    rooms[7] = &lori;
    
    struct Room bruce;
    memset(&bruce, 0, sizeof(struct Room));
    strcpy( bruce.name, "bruce");
    strcpy(bruce.location, "EMPTY");
    bruce.numOutboundConnections = 0;
    strcpy(bruce.filename, "bruce.txt");
    rooms[8] = &bruce;
    
    struct Room mallcop;
    memset(&mallcop, 0, sizeof(struct Room));
    strcpy( mallcop.name, "mallcop");
    strcpy(mallcop.location, "EMPTY");
    mallcop.numOutboundConnections = 0;
    strcpy(mallcop.filename, "mallcop.txt");
    rooms[9] = &mallcop;
    
    
    int nums[NUM_ROOMS]; /*chosen numbers*/
    int numset = 10;
    int choose = 8;
    
    /*Select random numbers to use for choosing
    which rooms will be included in the game
    https://stackoverflow.com/questions/1608181/unique-random-numbers-in-an-integer-array-in-the-c-programming-language
    */
    for(i=0; i< numset && j < choose; ++i){
        int rn = numset- i;
        int rm = choose-j;
        if(rand() % rn < rm){
            nums[j++] = i +1;
        }
    }
    
    /*Take the rooms at the indeces selected
    and add them to the chosenRooms array*/
    i = 0;
    size_t arr_len = sizeof(nums) / sizeof(int);
    for(i=0; i<arr_len; i++){
        int cur_num = nums[i];
        struct Room* cur_room = rooms[cur_num];
        chosenRooms[i] = cur_room;
        /*printf("%s\n", chosenRooms[i]->name);*/
    }
    
    
    room_positions();
    buildConnections();
    mkDir();
    printToFile();
    
}