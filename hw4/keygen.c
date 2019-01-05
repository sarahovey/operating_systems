#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
Sara Hovey
Fall 2018
This program generates n random characters
Where n is an integer given on the command line
*/
char* makeKey(int length);
int main(int argc, char* argv[]) {
  if (argc < 2) {
    perror("Error: Please specify a key length");
    return 1;
  }

  //Use the arg from the cli to determine the length of the key
  int length = atoi(argv[1]);

  //Call the function to build the key
  char* key = makeKey(length);

  printf("%s\n", key);

  //be free
  free(key);

  return 0;
}

/**********************************
 * Generate a random key
 * The length is determined by the
 * number given by the user
 * at runtime
***********************************/
char* makeKey(int length) {
    // Seed random with time
    //https://stackoverflow.com/questions/3693511/rand-seeding-with-time-problem
    srand(time(NULL));
    
    //Set up a buffer to hold the key
    char* key = (char *) malloc(sizeof(char) * length);
    
    //we're going to build a string letter by letter
    //by accessing each index
    int i;
    for (i = 0; i < length; i++) {
        //this is 27 because we're also using spaces
        //so we'll get anything between 0 and 27
        int thischar = rand() % 27;
        
        //Adding 65 gives capital letters
        thischar += 65;
        
        //91 is the first sequential character after 'A'-'Z', 
        //or ascii 65-90
        //ascii 91 is '['
        //we obviously don't want brackets, so if we happen to
        //generate one, we'll just convert it to 32 to be a space ' '
        if(thischar==91){
            thischar=32;
        }
        
        //Convert to a char, so from say '65' to 'A'
        //and store in the string at the current index
        key[i] = (char) thischar;
    }
    
    return key;
}