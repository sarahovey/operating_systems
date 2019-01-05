#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>

/*******************************************
 * Sara Hovey
 * Fall 2018
 * 
 * otp_dec_d
 * This program listens for a client to connect
 * If the decyprion client connects, this program
 * will decipher the ciphertext given
********************************************/
void decrypt(char* ciphertext, char* key, int textLen);
//from the lecture
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
    /**
    * This chunk sets up the necessary information
    * to start the server listening for connections
    **/
    //Check command line args
    if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args
    
    //Set up variables
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

    /**
     * This chunk starts the
     * server listening
    **/
  //Start listening
    while(1) {
        //this section listens indefinitely 
        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0) error("ERROR on accept");
        
        //Now that we've established a connection,
        //Make a child process to perform the decryption
        pid_t child = fork();
        if(child == -1) {
          perror("Process unsucessfully spawned");
        
        } 
        else if(child == 0) {
            //If the child successfully forks, do all this stuff
            //The child process is responsible for closing the socket
            memset(buffer, '\0', 256); //get ready to read
            charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
            if (charsRead < 0) error("ERROR reading from socket");
            
            //this checks to make sure we're interfacing with this daemon's decryption client
            //The program introduces itself, and this program evaluates its "nametag"
            //if the nametag doesn't match the decryption client's nametag,
            //we send a rejection message that the client handles
            if (strncmp(buffer, "decrypt", 7) == 0) {
                // Get size of incoming message
                int length = atoi(&buffer[7]);
                
                // Send a Success message back to the client
                charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
                if (charsRead < 0) error("ERROR writing to socket");
                
                //Set up buffers for the ciphertext and key
                char ciphertext[length];
                char key[length];
                
                //Truncate the key to the length of the cipher
                //this helped me with some segfaults
                //also thought about changing it to a ring buffer but this does for now
                memset(ciphertext, '\0', length);
                memset(key, '\0', length);
                
                //Get the ciphertext from the client
                charsRead = recv(establishedConnectionFD, ciphertext, length, 0); // Read the client's message from the socket
                if (charsRead < 0) error("ERROR reading from socket");
                
                // Send a Success message back to the client
                charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
                if (charsRead < 0) error("ERROR writing to socket");
                
                //Get the key from the client
                charsRead = recv(establishedConnectionFD, key, length, 0); // Read the client's message from the socket
                if (charsRead < 0) error("ERROR reading from socket");
                
                //decipher the message we got from the client
                decrypt(ciphertext, key, length);
                
                //Respond to the client with the decrypted message
                charsRead = send(establishedConnectionFD, ciphertext, length, 0); // Send success back
                
                
                if (charsRead < 0) error("ERROR writing to socket");
                close(establishedConnectionFD); // Close the existing socket which is connected to the client
                return 0;
        
          } 
          else {
            //This is the case where the enc program attempts t connect
            perror("Error: this daemon cannot interface with otp_enc");
            //Rejct the client by sending this message back
            //the client will look for this message and handle it appropriately
            charsRead = send(establishedConnectionFD, "nope", 4, 0);
            //cleanup
            if (charsRead < 0) error("ERROR writing to socket");
            close(establishedConnectionFD); // Close the existing socket which is connected to the client
            return(1);
          }
        }
    }
    //Cleanup the socket
 	close(listenSocketFD); // Close the listening socket
 	return 0;
 }
 
/*****************************************
 * Given a key, and plaintext, encrypt
 * the plaintext using a one time pad
******************************************/
void decrypt(char* ciphertext, char* key, int textLen) {
   int i=0;
   //Iterate over the length of the ciphertext
   //decipher the text character by character
   for (i = 0; i < textLen; i++) {
       
        int curCipherChar = (int) ciphertext[i];
        int curKeyChar = (int) key[i];
        
        //If curCipherChar isn't a space, convert it to its numeric position in the alphabet
        if(curCipherChar!=32){
            curCipherChar=curCipherChar-65;
        }
        else{
            curCipherChar=26;
        }
        
        //Same idea with the key
        if(curKeyChar!=32){
            curKeyChar=curKeyChar-65;
        }
        else{
            curKeyChar=26;
        }
        
        //Now we substract the key value from the value of the ciphertext
        curCipherChar -= curKeyChar;
        
        //and perform the mod
        curCipherChar = (curCipherChar +27) % 27;
        
        //This will get us the uppercase value
        //converts from the numeric position in the alphabet to an uppercase ascii
        curCipherChar += 65;
        
        //Convert the bracket back to a ' '
        if(curCipherChar == 91){
            curCipherChar == 32;
        }
        
        //Stash it in the string we're using to store ciphertext
        ciphertext[i] = (char)curCipherChar;

   }
   
   //once this loop is finished, the ciphertext will have been deciphered char by char

 }