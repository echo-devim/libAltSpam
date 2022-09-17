/*
	Tiny API Server implementation for AltSpam.
	The server supports only one client.

	Message Structure:
	|OPERATION|SIZE|DATA|

	OPERATION is 1 byte representing the function of AltSpam to call
	SIZE is an integer representing the data length
	DATA is the argument of OPERATION

	Note: Messages are protected by a very basic XOR encryption.
	It is recommended to not expose the server except than on localhost
*/
#include<cstdio>
#include<cstring>


#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#endif

enum OPERATION {
	LOAD_WORD_COUNT,
	SAVE_WORD_COUNT,
	IMPORT_WORD_COUNT,
	ADD_TO_SPAM,
	REMOVE_FROM_SPAM,
	IS_SPAM,
	SPAM_SCORE,
	RELOAD_WORD_COUNT
};


void * get_in_addr(struct sockaddr * sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

#ifdef _WIN32

bool WinsockInitialized()
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET && WSAGetLastError() == WSANOTINITIALISED) {
		return false;
	}

	closesocket(s);
	return true;
}
#endif

int start_server(AltSpam &altspam)
{
	int status;
	struct addrinfo hints, *res;
	int listner;

#ifdef _WIN32
	//----------------------
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
#endif


	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; // TCP Socket SOCK_DGRAM 
	hints.ai_flags = AI_PASSIVE;
	bool error = false;

	// Fill the res data structure and make sure that the results make sense. 
	status = getaddrinfo("127.0.0.1", "22525", &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		error = true;
	}

	// Create Socket and check if error occured afterwards
	listner = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listner < 0)
	{
		fprintf(stderr, "socket error: %s\n", gai_strerror(status));
		error = true;
	}

	// Bind the socket to the address of my local machine and port number 
	status = bind(listner, res->ai_addr, res->ai_addrlen);
	if (status < 0)
	{
		fprintf(stderr, "Cannot bind. Probably another istance is running.\n");
		error = true;
	}

	status = listen(listner, 10);
	if (status < 0)
	{
		fprintf(stderr, "listen: %s\n", gai_strerror(status));
		error = true;
	}

	// Free the res linked list after we are done with it	
	freeaddrinfo(res);


	// We should wait now for a connection to accept
	int new_conn_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	char s[INET6_ADDRSTRLEN]; // an empty string 
	int n = 0, k;
  	char header[5] = {0,};

	// Calculate the size of the data structure	
	addr_size = sizeof client_addr;

	if (error) {
		printf("Error occurred, exiting..\n");
		return 1;
	} else {
		printf("Server started\n");
	}

	while (true) {
		// Accept a new connection and return back the socket descriptor 
		new_conn_fd = accept(listner, (struct sockaddr *) & client_addr, &addr_size);
		if (new_conn_fd < 0)
		{
			fprintf(stderr, "accept: %s\n", gai_strerror(new_conn_fd));
			continue;
		}

		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *) &client_addr), s, sizeof s);
		printf("connected to %s \n", s);
		while(true) {
			ssize_t readBytes = recv(new_conn_fd, header, sizeof(header), 0);
			if (readBytes < 1)
				break;
			printf("OPERATION: %d\n", header[0]);
			int data_size =  ((header[1] & 0xff) << 24) | ((header[2] & 0xff) << 16) | ((header[3] & 0xff) << 8) | (header[4] & 0xff);
			printf("DATA SIZE: %d\n", data_size);
			if (data_size < 1) {
				printf("Invalid size\n");
				break;
			}
			char *data = new char[data_size];
			int got_bytes = 0;
			int miss_bytes = data_size;
			while ((readBytes > 0) && (miss_bytes > 0)) {
				readBytes = recv(new_conn_fd, data+got_bytes, miss_bytes, 0);
				got_bytes += readBytes;
				miss_bytes = data_size - got_bytes;
			}
			//printf("Bytes miss: %d, bytes_got: %d\n", miss_bytes, got_bytes);
			if (readBytes < 1)
				break;
			//Decrypt data
			for (int i = 0; i < data_size; i++) {
				data[i] = data[i] ^ header[4];
			}
			//Copy data
			string payload(data, 0, data_size);
			delete data;
			//Parse the received message and call AltSpam functions
			stringstream resp;
			char function = header[0];

			if (function == LOAD_WORD_COUNT) {
				resp << altspam.loadWordCount(payload);
			} else if (function == SAVE_WORD_COUNT) {
				resp << altspam.saveWordCount(payload);
			} else if (function == IMPORT_WORD_COUNT) {
				resp << altspam.importWordCount(payload);
			} else if (function == ADD_TO_SPAM) {
				EnrichedEmail mail(payload);
				altspam.addToSpam(mail);
				resp << "Done";
			} else if (function == REMOVE_FROM_SPAM) {
				EnrichedEmail mail(payload);
				altspam.removeFromSpam(mail);
				resp << "Done";
			} else if (function == IS_SPAM) {
				EnrichedEmail mail(payload);
				resp << altspam.isSpam(mail);
			} else if (function == SPAM_SCORE) {
				EnrichedEmail mail(payload);
				resp << altspam.getSpamScore(mail);
			} else if (function == RELOAD_WORD_COUNT) {
				resp << altspam.reloadWordCount();
			}
			resp << "\n";
			status = send(new_conn_fd, resp.str().c_str(), resp.str().size(), 0);
			if (status == -1)
				break;
		}
		#ifdef _WIN32
			closesocket(new_conn_fd);
		#else
			close(new_conn_fd);
		#endif
	}

	return 0;
}