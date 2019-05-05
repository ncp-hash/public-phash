// functions to implement
// client_connect_to_server
// client_
// client_recieve_public_key (paillier)

// server_send_betas
// client_recieve_betas

// client_send_enc_hash
// server_recieve_enc_hash

//******************************************************************************************
#include <assert.h>

#include <stdio.h> 
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

// #define MAX 513 //256, 2-byte betas with one /n character at end 
#define MAX 65537 // 65,536 bytes plus one terminating byte
#define PORT 8080

#define TRUE 1
#define FALSE 0

#define SUCCESS 0
#define ABORT 1

#define BETA_SIZE 256 //each beta is 256 bytes
#define NUM_BETAS 256 //there are 256 betas in total
#define PAILLIER_KEY_SIZE 256 //this is 256 bytes

int min(int first, int second){

	if(first<second) return first;
	else if(first>=second) return second;
}

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

	if ((bind(listening_socket_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) != SUCCESS) { 
		printf("socket bind failure\n"); 
		exit(0); 
	} 
	else
		printf("socket bind success\n"); 


	if ((listen(listening_socket_fd, 5)) != SUCCESS) { 
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
	int client_socket_fd;

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


int client_connect_to_server(void){

	int socketFD;
	struct sockaddr_in serverAddress;

	// open new socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (socketFD == -1) { 
		printf("error: socket not opened\n"); 
		exit(0); 
	} 
	else
		printf("socket opened\n");

	// set address information of socket
	bzero(&serverAddress, sizeof(serverAddress)); 
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); //TODO: find how to change IP so I can send messeges between computers
	//serverAddress.sin_addr.s_addr = inet_addr("172.20.10.4"); 
	serverAddress.sin_port = htons(PORT); 

	// tell client socket to connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) { 
		printf("error: could not connect to server\n"); 
		exit(0); 
	} 
	else
		printf("connected\n"); 

	return socketFD;
}

//sends message, blocks for response. 
// will loop indefinitely without no recipient response
//re implement with forks
//returns 0 on success
int send_char_string(int recipient_socket_fd, char* message, int message_len){

	char recipient_response[message_len];
	char* expected_response = (char*)malloc((message_len+1) * sizeof(char));
	strcpy(expected_response, "message received.\n"); //TODO: may need to use strncopy 
	//char expected_response[message_len] = "message received.\n";

	while(TRUE){

		write(recipient_socket_fd, message, message_len);
		printf("message sent:\n%s\n", message);

		bzero(recipient_response, message_len); 
		read(recipient_socket_fd, recipient_response, message_len);

		if ((strcmp(recipient_response, expected_response)) == 0) { 
			printf("message was confirmed as received.\n"); 
			return 0; 
		} 
	} 

}


//recieves a character string and then returns a confirmation to sender
char* receive_char_string(int sender_socket_fd, int message_len){

	char* received_message = (char*)malloc((message_len+1) * sizeof(char));

	char* confirmation = (char*)malloc((message_len+1) * sizeof(char));
	strcpy(confirmation, "message received.\n"); //TODO: may need to use strncopy 

	char endOfTransmission;

	while (TRUE) { 

		bzero(received_message, message_len); 

		// get message from client; copy it to received_message buffer 
		read(sender_socket_fd, received_message, message_len); 
		printf("message received:\n%s\n", received_message);

		// and send that confirmation message to client 
		write(sender_socket_fd, confirmation, message_len); //may want to replace with rec or send avoid problems in large filetypes

		// if client string contains "\n" then server exit and chat ended.
		//endOfTransmission = received_message[strlen(received_message)-1]; 
		endOfTransmission = received_message[message_len-1]; 
		if (strcmp("\n", &endOfTransmission) == 0) { 
			printf("server quitting\n"); 
			break; 
		}
	} 

	free(confirmation);
	return received_message;
}

// ========================================================================================
// KeyFile Read and Write Utilities
// ========================================================================================

// These are written in c because there will be a parent process initialize the keyfiles and
// to start up the hash recieving server. This parent process will be written in c and 
// will use posix fork() calls.

// When the client and server are run on the same computer, the parent will first initialize
// the key files which both child processes (the hash recieving server and the client) will
// read from, but not write to. This solves a race condition issue experienced in a previous
// implementation, while also better simulating the separate nature of the client and server
// given that they'd be separate processes that will communicate exclusively through tcp.

// when client and server are on separate devices, both will still have access to the keyfiles.
// the client will have the keyfiles as part it's installation files and the server will hve a
// copy locally (since it generated the public keys in the first place.)

//what is the input type of the key itself? Is it void?
void write_key_file(char* file_name, void* key){

	int num_bytes;
    if (!strncmp(file_name,"betas",5)) num_bytes = (BETA_SIZE * NUM_BETAS);
    if (!strncmp(file_name,"paillier",8)) num_bytes = PAILLIER_KEY_SIZE;
    if (!strncmp(file_name,"betas_test",10)) num_bytes = (BETA_SIZE * NUM_BETAS);


	//intialize filename
	char file_path[255];
	snprintf(file_path,255,"./%s.key",file_name);
	
	//open file in append mode if a single beta
	FILE *fptr;
	if(!strncmp(file_name,"betas",5)) fptr = fopen(file_path,"ab");
	else fptr = fopen(file_path,"wb");

	//write null terminator when it's a paillier key (may need null terminator for both)
	fwrite(key,1,num_bytes,fptr);
	if(!strncmp(file_name,"paillier",8)) fwrite("\0",1,1,fptr); //write the string null terminator to file

	if(fptr == NULL)
	{
	  printf("Error!");   
	  exit(1);             
	}

	fclose(fptr);
}


//to cast the betas returned from this function in the c++ code, do the following:
// std::vector<paillier_ciphertext_t> vector_name(ptr_to_return_key, ptr_to_return_key + number_of_elements);
// notice that the offset in the last argument is for the number of elements NOT the number of bytes.

//to cast the paillier public key returned from this function in c++ code, do this:
// paillier_ciphertext_t paillier_key = *ptr_to_return_key;

void* read_key_file(char* file_name){

	int num_bytes;
    if (!strncmp(file_name,"betas",5)) num_bytes = (BETA_SIZE * NUM_BETAS);
    if (!strncmp(file_name,"paillier",8)) num_bytes = PAILLIER_KEY_SIZE + 1;// +1 byte for the null terminator

	//initialize filename
	char file_path[255];
	snprintf(file_path,255,"./%s.key",file_name);

	//alocate memory for key to be returned
    FILE *key_file = fopen(file_path, "rb");  
    unsigned char *return_key = malloc(num_bytes); //allocates that many bytes
    
    //read key from file and close
    fread(return_key, 1, num_bytes, key_file);
    fclose(key_file); 

    //return the paillier public key OR the pailler ciphertext (betas) depending on file_name
	if (!strncmp(file_name,"betas",5)) return paillier_ciphertext_from_bytes(return_key, num_bytes);
	if (!strncmp(file_name,"paillier",8)) return paillier_pubkey_from_hex(return_key);
	return 1;
}

// int send_bytes_chunk(int recipient_socket_fd, void *chunk_buffer, int chunk_len){

//     unsigned char *pbuf = (unsigned char *) chunk_buffer; //todo: check on this later

//     while (chunk_len > 0){

//         int num = send(recipient_socket_fd, pbuf, chunk_len, 0);
//         if (num == -1) return ABORT;

//         pbuf += num;
//         chunk_len -= num;
//     }

//     return SUCCESS;
// }


// int send_bytes_len(int recipient_socket_fd, long bytes_len){

//     bytes_len = htonl(bytes_len);
//     return send_bytes_chunk(recipient_socket_fd, &bytes_len, sizeof(bytes_len));
// }


// int send_bytes_all(int recipient_socket_fd, void* all_bytes, int all_bytes_len){

//     unsigned char *bytes_buffer = (unsigned char *) all_bytes;


//     if (send_bytes_len(recipient_socket_fd, all_bytes_len) == ABORT)
//         return ABORT;

//     if (all_bytes_len > 0)
//     {
//         char chunk_buffer[1024];
//         do
//         {

//             unsigned int num_bytes_read = min(all_bytes_len, sizeof(chunk_buffer));

//             if (num_bytes_read < 1){
//                 return ABORT;
//                 printf("ABORT SEND\n");
//             }

            
//             strncpy(chunk_buffer, bytes_buffer, num_bytes_read);

//             if (send_bytes_chunk(recipient_socket_fd, chunk_buffer, num_bytes_read) == ABORT)
//                 return ABORT;

//             all_bytes_len -= num_bytes_read;
//             all_bytes += num_bytes_read;
//         }
//         while (all_bytes_len > 0);

//     }

//     for(int i = 0; i < 256*256; i++){
//         printf("%02x", ((char *)all_bytes)[i]);
//     }
//     putchar( '\n' );

//     return SUCCESS;
// }



// int receive_bytes_chunk(int sender_socket_fd, void *chunk_buffer, int chunk_buffer_len){

//     unsigned char *pchunk_buffer = (unsigned char *) chunk_buffer;

//     while (chunk_buffer_len > 0){

//         int num_bytes_received = recv(sender_socket_fd, pchunk_buffer, chunk_buffer_len, 0);

//         if (num_bytes_received == -1 || num_bytes_received == 0) return ABORT; 

//         pchunk_buffer += num_bytes_received;
//         chunk_buffer_len -= num_bytes_received;
//     }

//     return SUCCESS;
// }


// int receive_bytes_len(int sender_socket_fd, long *bytes_len){

//     if (receive_bytes_chunk(sender_socket_fd, bytes_len, sizeof(bytes_len)) == ABORT)
//         return ABORT;

//     *bytes_len = ntohl(*bytes_len);
//     return SUCCESS;
// }

// int receive_bytes_all(int sender_socket_fd, void* all_bytes){

// 	printf("receive was called\n");

//     unsigned char *bytes_buffer = (unsigned char *) all_bytes;

//     long all_bytes_len;
//     long offset = 0;

//     if (receive_bytes_len(sender_socket_fd, &all_bytes_len) == ABORT)
//         return ABORT;

//     if (all_bytes_len > 0){

//         char chunk_buffer[1024];
//         do{
//             int num_bytes_received = min(all_bytes_len, sizeof(chunk_buffer));

//             if (receive_bytes_chunk(sender_socket_fd, chunk_buffer, num_bytes_received) == ABORT)
//                 return ABORT;

//             memcpy(all_bytes + offset, chunk_buffer, num_bytes_received);
//             offset += num_bytes_received;
//             all_bytes_len -= num_bytes_received;

//         } while (all_bytes_len > 0);
//     }


//     //print out the 
//     for(int i = 0; i < 256*256; i++){
//         printf("%02x", ((char *) all_bytes)[i]);
//     }
//     putchar( '\n' );

//     return SUCCESS;
// }