#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void encryptCipher(char* cipher, char* key, int length);
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
     //main idea: set the server up to listen, 
     //once we get the data from the client, we can perform the encryption
     //then we can send it back
     
     if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args
     int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
     socklen_t sizeOfClientInfo;
     char buffer[256];
     struct sockaddr_in serverAddress, clientAddress;
    
     // Set up the address struct for this process (the server)
     memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
     portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
     serverAddress.sin_family = AF_INET; // Create a network-capable socket
     serverAddress.sin_port = htons(portNumber); // Store the port number
     serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
    
     // Set up the socket
     listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
     if (listenSocketFD < 0) error("ERROR opening socket");
    
     // Enable the socket to begin listening
     if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
     	error("ERROR on binding");
     listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
     
    //set up this infinite loop to keep listening for connections
    while(1) {
        
        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0) error("ERROR on accept");
        
        //Now that we've established a connection,
        //Make a child process to perform the encryption
        pid_t child = fork();
        if(child == -1) {
          perror("Hull Breach!");
        
        } 
        //If the child successfully forks, do all this stuff
        //The child process is responsible for closing the socket
        else if (child == 0) {
            
            //The client is going to pass us some information about the size of the plaintext file message
            memset(buffer, '\0', 256);
            charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
            if (charsRead < 0) error("ERROR reading from socket");
            
            //this checks to make sure we're interfacing with this daemon's encryption client
            if (strncmp(buffer, "encrypt", 7) == 0) {
                //this message carries information about the length of the plaintext
                //this way we can plan how much space to allocate for it
                //this message also acts as acting as a "nametag" to make sure 
                //we're not interfacing with the dec program
                int length = atoi(&buffer[7]);
                
                // Send a Success message back to the client
                charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
                if (charsRead < 0) error("ERROR writing to socket");
                
                //everything under here except the closing statements are not from the lecture
                // Prepare buffers
                char cipher[length];
                char key[length];
                
                //Truncate the key to the length of the plaintext
                //this helped me with some segfaults
                //also thought about changing it to a ring buffer but this does for now
                memset(cipher, '\0', length);
                memset(key, '\0', length);
                
                //Get the plaintext from the client
                charsRead = recv(establishedConnectionFD, cipher, length, 0); // Read the client's message from the socket
                if (charsRead < 0) error("ERROR reading from socket");
                // Send a Success message back to the client
                charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
                if (charsRead < 0) error("ERROR writing to socket");
                
                //Get the key from the client
                charsRead = recv(establishedConnectionFD, key, length, 0); // Read the client's message from the socket
                if (charsRead < 0) error("ERROR reading from socket");
                
                //Now we have everything we need, we can do the encryption
                encryptCipher(cipher, key, length);
                
                // Send an encrypted message back to the client
                charsRead = send(establishedConnectionFD, cipher, length, 0); // Send success back
                
                if (charsRead < 0) error("ERROR writing to socket");
                close(establishedConnectionFD); // Close the existing socket which is connected to the client
                return 0;
        
          } 
          else {
            //the case when the otp_dec attempts to connect
            perror("Error: this daemon cannot interface with otp_dec");
            
            // Send a Failure message back to the client
            //Rejct the client by sending this message back
            //the client will look for this message and handle it appropriately
            charsRead = send(establishedConnectionFD, "nope", 4, 0); // Send success back
            
            //cleanup
            if (charsRead < 0) error("ERROR writing to socket");
            close(establishedConnectionFD); // Close the existing socket which is connected to the client
            return(1);
          }
        }
  }

 	close(listenSocketFD); // Close the listening socket
 	return 0;
 }
 
/*****************************************
 * Given a key, and plaintext, encrypt
 * the plaintext using a one time pad
******************************************/
void encryptCipher(char* cipher, char* key, int length) {
    int i;
    //Iterate over the length of the given text,
    //and encrypt char by char
    for (i = 0; i < length; i++) {
        //represents current plaintext character
        int curPlaintextChar = (int) cipher[i];
        //represents current key character
        int curKeyChar = (int) key[i];
        
        //Perform the encrytion
        //if this character isn't a space,
        //convert it to its position in the alphabet
        if(curPlaintextChar!=32){
            curPlaintextChar = curPlaintextChar-65;
        }
        //otherwise, if it is, convert it to the last spot in the alphabet
        else{
            curPlaintextChar=26;
        }
        
        //Same idea with the current character in the key
        if(curKeyChar==32){
            curKeyChar=26;
        }
        else{
            curKeyChar = curKeyChar-65;
        }
        
        //add the two togeher, as shown in the example
        curPlaintextChar += curKeyChar;
        
        //do the mod,
        //again using 27 since 26 chars + space
        curPlaintextChar = curPlaintextChar % 27;
        
        //Get the uppercase version
        curPlaintextChar += 65;
        
        //Like in the keygen, if we have a 91, it should be a 32
        //functionally converting a '[' to a ' '
        if(curPlaintextChar==91){
            curPlaintextChar = 32;
        }
        
        cipher[i] = (char) curPlaintextChar;

    }
}