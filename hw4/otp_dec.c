#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/*******************************************
 * Sara Hovey
 * Fall 2018
 * 
 * otp_dec
 * This program takes a key and file containing
 * ciphertext, and connects to a daemon to
 * ask the daemon to decrypt it
 * This program then prints the decrypted message
 * to stdout
********************************************/
void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

// void connect(char* buffer, int textLen, int portNumber, char* cipher, char* key){
    
// }
int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
    //char buffer[256];
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s mycipher mykey port\n", argv[0]); exit(0); } // Check usage & args


	/**
	File setup
	In this stage, we extract the data from the
	ciphertext file, and key file, and store
	it in a string buffer
	This way it can be given to the daemon
	**/
	
	//ciphertext file is the first arg
	//Open it for reading
	FILE* fp = fopen(argv[1], "r");

	//Read from the file
	//https://stackoverflow.com/questions/21074084/check-text-file-textLen-in-c
	fseek(fp, 0, SEEK_END);
	int textLen = ftell(fp);
	fseek(fp, 0, 0);

    //Store ciphertext in a string
	char cipher[textLen];
	memset(cipher, '\0', textLen);
	fgets(cipher, textLen, fp);
	fclose(fp);
	
	//read key from the file specified on cli
	fp = fopen(argv[2], "r");
	
	//store the key in a string
	char key[textLen];
	memset(key, '\0', textLen);
	fgets(key, textLen, fp);

	fclose(fp);

	char buffer[textLen];
	memset(buffer, '\0', textLen);
	
	//This acts as this program's "nametag"
	//When it makes an attempt to connect to a daemon,
	//the daemon will see this and figure out if 
	//it should accept the connection or not
	sprintf(buffer, "decrypt%d", textLen);

	if (strlen(key) != strlen(cipher)) {
		perror("Error: key file is shorter than plaintext");
		return 1;
	}
	
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	
	/**
	Connection
	Now that we've read the data from the keyfile, and ciphertext,
	we can prepare to ask the daemon for decryption
	connect(buffer, textLen, portNumber, cipher, key );
	**/
	
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	//portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
	

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");
		
	/**
	Handshake
	This performs an "introduction" by giving the daemon its nametag
	The server will reject if it's the wrong program by sending back
	a "nope"
	We check for that to see if we've been rejected
	**/
	// Send message to server
	//this is where we tell the server the size of the message we're sending next
	//I also did this for 372 projects and it was really helpful
	//this also acts as a "nametag" for the program,
	//the contents of buffer are the "nametag", and length of the program
	//this way the daemon can decide whether or not to reject the program
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, 39, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	//This catches+handles program incompatibility
	//If it gets "nope" back, then it's attempting to connect to the wrong program
	if (strncmp(buffer, "nope", 4) == 0) {
		fprintf(stderr, "%s%d", "otp_dec cannot connect to otp_enc_d! Occured on port \n", portNumber);
		exit(2);
	}
	
	/**
	Ciphertext
	**/
	//Give the daemon our ciphertext that we extracted from the file
	charsWritten = send(socketFD, cipher, textLen, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < textLen) printf("CLIENT: WARNING: Not all data written to socket!\n");
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, 39, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	/**
	Key
	**/
	//Give the daemon our key that we extracted from the file
	charsWritten = send(socketFD, key, textLen, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < textLen) printf("CLIENT: WARNING: Not all data written to socket!\n");
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	//Send the message we get back to stdout
	printf("%s\n", buffer);
	
	//cleanup
	close(socketFD); // Close the socket
	return 0;
}