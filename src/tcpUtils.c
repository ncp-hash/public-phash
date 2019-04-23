#include <stdio.h> 
#include <unistd.h>

#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 

// #define MAX 513 //256, 2-byte betas with one /n character at end 
#define MAX 65537 // 65,536 bytes plus one terminating byte
#define PORT 8080

#define TRUE 1
#define FALSE 0

int server_init(void){

	int listening_socket_fd; 
	struct sockaddr_in serverAddress;

	listening_socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (listening_socket_fd == -1) { 
		printf("error: socket not opened.\n"); 
		exit(0); 
	} 
	else
		printf("socket opened\n"); 


	bzero(&serverAddress, sizeof(serverAddress)); 

	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); 
	serverAddress.sin_port = htons(PORT); 

	if ((bind(listening_socket_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) != 0) { 
		printf("socket bind failure\n"); 
		exit(0); 
	} 
	else
		printf("socket bind success\n"); 


	if ((listen(listening_socket_fd, 5)) != 0) { 
		printf("listen failure\n"); 
		exit(0); 
	} 
	else
		printf("listening\n");


	return listening_socket_fd; //don't forget to close this in main
}


int server_connect_to_client(int listening_socket_fd){

	struct sockaddr_in clientAddress; 
	unsigned int len; 

	len = sizeof(clientAddress); 
	client_socket_fd = accept(listening_socket_fd, (struct sockaddr*)&clientAddress, &len);

	if (client_socket_fd < 0) { 
		printf("client acccept failed\n"); 
		exit(0); 
	} 
	else
		printf("client accepted\n"); 

	return client_socket_fd; //don't forget to close this in main
}


//sends message, blocks for response. 
// will loop indefinitely without no recipient response
//re implement with forks
//returns 0 on success
int send_message(int recipient_socket_fd, char* message, int message_len){

	char recipient_response[message_len];
	char expected_response[message_len] = "message recieved.\n";

	while(TRUE){

		write(recipient_socket_fd, message, message_len);
		printf("message sent:\n%s\n", message);

		bzero(recipient_response, message_len); 
		read(socketFD, recipient_response, message_len);

		if ((strcmp(recipient_response, expected_response)) == 0) { 
			printf("message recieved.\n"); 
			return 0; 
		} 
	} 

}


//returns 0 on success (positive confirmation)
int server_send_paillier_pubkey(int client_socket_fd, char* paillier_pubkey, int paillier_pubkey_len){

	ret_val = send_message(client_socket_fd, paillier_pubkey, paillier_pubkey_lenl);
	return ret_val;
} 


void get_client_input(int client_socket_fd){

	char clientEncHash[MAX]; 
	char confirmation[MAX] = "hash recieved.\n";
	char endOfTransmission;

	while (TRUE) { 

		bzero(clientEncHash, MAX); 

		// get message from client; copy it to clientEncHash buffer 
		read(client_socket_fd, clientEncHash, MAX); 

		// print buffer which contains the client contents 
		printf("client's encrypted hash: %s\t", clientEncHash); 
		
		// and send that confirmation message to client 
		write(client_socket_fd, confirmation, MAX); //may want to replace with rec or send avoid problems in large filetypes

		// if client hash contains "\n" then server exit and chat ended.
		//endOfTransmission = clientEncHash[strlen(clientEncHash)-1]; 
		endOfTransmission = clientEncHash[MAX-1]; 
		if (strcmp("\n", &endOfTransmission) == 0) { 
			printf("server quitting\n"); 
			break; 
		}
	} 
	/************************* 
		need to return client
		input here
	**************************/

}

// functions to implement
// client_recieve_public_key

// server_send_betas
// client_recieve_betas

// client_send_enc_hash
// server_recieve_enc_hash






