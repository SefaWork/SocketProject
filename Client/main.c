#include<stdio.h>

// https://stackoverflow.com/a/28031039
#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501 // Apparently for windows XP ??
  #endif
  #include <winsock2.h>
  #include <Ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h> 
  #include <unistd.h>
#endif

// Data structure sent when the game starts.
struct StartData {
	unsigned short int minVal;
	unsigned short int maxVal;
	unsigned short int startLives;
};

// Data structure sent when the game is ongoing.
struct ResponseData {
	unsigned short int lives;
	char state; // State 'a': Guessed Low, State 'b': Guessed High, State 'd': Lost, State 'c': Won, State 'e': Error
	unsigned short int realNum;
};

int initSocket(void);
void closeSocket(SOCKET sock);
void cleanup(void);

int main(void)
{
	SOCKET s;
	struct sockaddr_in server;
	int recv_size;
	
	struct ResponseData response_data;
	struct StartData start_data;

	// Initialize.
	//printf("Initialising...");
	if (initSocket())
	{
		printf("Failed to initialize.");
		return 1;
	}
	
	//printf("Initialised.\n");
	
	//Create a socket.
	if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
	{
		printf("Could not create socket.");
		return 1;
	}

	printf("Socket created.\n");
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 25565 );

	//Connect to remote server.
	while (connect(s , (struct sockaddr *)&server , sizeof(server)) == 0)
	{
		printf("Connected!\n");
		
		while(1) {
			recv_size = recv(s, (char*) &start_data, sizeof(start_data), 0);
			
			if(recv_size == SOCKET_ERROR) {
				printf("There was an error. Game stopped.\n");
				getchar();
				return 1;
			}
			
			printf("Starting game!\n\nStarting Lives: %d\n\nCan you guess the number? Hint: It is between %d and %d!\nInput your guess: ", start_data.startLives, start_data.minVal, start_data.maxVal);
			
			int state = 1, input;
			
			while(state) {
				scanf("%d", &input);
				getchar();
				
				if(input < start_data.minVal || input > start_data.maxVal) {
					printf("Remember, your guess must be between %d and %d!\n", start_data.minVal, start_data.maxVal);
				} else {
					send(s, (unsigned char*) &input, sizeof(int), 0);
					recv(s, (unsigned char*) &response_data, sizeof(struct ResponseData), 0);
					
					if(response_data.state == 'c') {
						printf("Congratulations! The number was %d! You won!\n", response_data.realNum);
						break;
					} else if(response_data.state == 'd') {
						printf("Wrong guess! You ran out of lives...\n\nThe correct number was %d.\n", response_data.realNum);
						break;
					} else if(response_data.state == 'a') {
						printf("Wrong guess! %d lives remaining. The value is higher than what you guessed.\n", response_data.lives);
					} else if(response_data.state == 'b') {
						printf("Wrong guess! %d lives remaining. The value is smaller than what you guessed.\n", response_data.lives);
					} else {
						printf("Server received an error.\n");
						break;
					}
				}
				
				printf("\n\nGuess a new number > ");
			}
			
			printf("\n\nDo you wish to play again? [y/n]\n> ");
			char againResponse = getchar();
			
			if(againResponse == 'y' || againResponse == 'Y') {
				unsigned short int _buff = 1;
				send(s, (unsigned char*) &_buff, sizeof(unsigned short int), 0);
			} else {
				closeSocket(s);
				cleanup();
				
				printf("Press the ENTER key to exit...");
				getchar();
				getchar();
				return 0;
			}
		}
	}

	printf("Could not reach the server.\n");
	return 1;
}

int initSocket(void) {
	#ifdef _WIN32
	    WSADATA wsa_data;
	    return WSAStartup(MAKEWORD(1,1), &wsa_data);
	#else
    	return 0; // Always successful on Linux since you don't need to initialize.
  	#endif
}

void closeSocket(SOCKET sock) {
	#ifdef _WIN32
		closesocket(sock);
	#else
	    close(sock);
	#endif
}

void cleanup(void) {
	#ifdef _WIN32
		WSACleanup();
	#endif
}
