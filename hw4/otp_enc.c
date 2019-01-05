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
 * otp_enc
 * this client makes a request to a server
 * it passes a file of plaintext, and a key
 * is returned an encrypted version of the
 * plaintext
********************************************/

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
    //char buffer[256];
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s original_text key port\n", argv[0]); exit(0); } // Check usage & args
    
	//original_text file is the first arg
	//Open it for reading
	FILE* fp = fopen(argv[1], "r");

	//Read from the file
	//We're looking for bad characters
	//https://stackoverflow.com/questions/21074084/check-text-file-textLen-in-c
	fseek(fp, 0, SEEK_END);
	int textLen = ftell(fp);
	fseek(fp, 0, 0);

    //store text in a string
	char original_text[textLen];
	memset(original_text, '\0', textLen);
	//put the text fp into the string we made
	fgets(original_text, textLen, fp);

	fclose(fp);

    //read key from the file specified on cli
	fp = fopen(argv[2], "r");
	char key[textLen];
	//We only need as much key as there is plaintext
	//Truncate the key to the length of the cipher
    //this helped me with some segfaults
    //also thought about changing it to a ring buffer but this does for now
	memset(key, '\0', textLen);
	fgets(key, textLen, fp);

	fclose(fp);

	char buffer[textLen];
	memset(buffer, '\0', textLen);
	
	//this program passes a 'nametag' to the server
	//during the handshake transaction
	//this program's "nametag" is 'encrypt'
	sprintf(buffer, "encrypt%d", textLen);

    //Check if the key is shorter than the original_text
	if (strlen(key) != strlen(original_text)) {
		perror("Error: key file is shorter than original_text");
		return 1;
	}
	
    //Look for bad characters
	int i = 0;
	//Iterate over the length of the plaintext
	for (i=0; i < strlen(original_text); i++) {
	    //if the value of a character is not a space or upper case character, it's no good
		if ((original_text[i] < 65 || original_text[i] > 90) && original_text[i] != 32) {
			perror("Error: bad character found");
			return 1;
		}
	}
	
	//Now that we've read the data from the keyfile, and original_text,
	//we can prepare to ask the daemon for encryption
	
	//This is all lifted from client.c 
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
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
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, 39, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	//This catches+handles program incompabilities
	//This is when we check for rejection
	if (strncmp(buffer, "nope", 4) == 0) {
		fprintf(stderr, "%s%d", "otp_enc cannot connect to otp_dec_d! Occured on port \n", portNumber);
		exit(2);
	}

	/**
	Plaintext
	**/
	//Give the daemon our original_text
	charsWritten = send(socketFD, original_text, textLen, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < textLen) printf("CLIENT: WARNING: Not all data written to socket!\n");
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, 39, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	/**
	Key
	**/
	//Give the daemon our key
	charsWritten = send(socketFD, key, textLen, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < textLen) printf("CLIENT: WARNING: Not all data written to socket!\n");
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	printf("%s\n", buffer);


	close(socketFD); // Close the socket
	return 0;
}