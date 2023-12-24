#include <time.h>
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

int initSocket(void);
void closeSocket(SOCKET sock);
void cleanup(void);

int generateRandNum(int,int);

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

int main() {
	
	// Network control.
	SOCKET s , new_socket;
	struct sockaddr_in server , client;
	int c, recv_size, client_reply;
	
	struct ResponseData toSend;
	struct StartData startData;
	
	// Game control.
	unsigned short int lives, num;
	
	printf("Initializing...\n");
	if (initSocket())
	{
		printf("Failed to initialize.");
		return 1;
	}
	
	printf("Initialised.\n");
	
	if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
	{
		printf("Could not create socket.\n");
		return 1;
	}
	
	printf("Socket created.\n");
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(25565);
	
	if(bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
	{
		printf("Couldn't bind socket.");
		return 1;
	}
	
	printf("Successfully binded socket.\n");
	
	listen(s , 3);
	printf("Listening...\n");
	
	c = sizeof(struct sockaddr_in);
	
	while( (new_socket = accept(s , (struct sockaddr *)&client, &c)) != INVALID_SOCKET )
	{
		// Receive new connections after each client.
		while(1) {
			// Loop until client is satisfied.
			char newState;
			int going = 1;
			
			lives = 3;
			
			unsigned short int min = generateRandNum(1,10);
			unsigned short int max = min + generateRandNum(10,30);
			
			int rangeIndex = (max - min) / 10;
			
			lives = 3 + rangeIndex * 1;
			
			startData.maxVal = max;
			startData.minVal = min;
			startData.startLives = lives;
			
			num = generateRandNum(min, max);
			
			printf("Starting game for client. They will start with %d lives and the corrent number is %d.\n", lives, num);
			
			//Inform the client about their lives and the guess range. Then, start the game.
			send(new_socket, (unsigned char*) &startData, sizeof(struct StartData) , 0);
			
			// Keep receiving data from client until game is over or lives are over.
			while(going) {
				int nErr = recv(new_socket, (unsigned char*) &client_reply , sizeof(int) , 0);
				
				if(nErr == SOCKET_ERROR) {
					printf("An error occured, aborting game.");
					newState = 'e';
					going = 0;
					break;
				}
			
				if(client_reply == num) {
					// Client won!
					printf("Client victory.\n");
					newState = 'c';
					going = 0;
				} else if(client_reply < num) {
					printf("Client guessed too low.\n");
					newState = 'a';
					lives--;
				} else {
					printf("Client guessed too high.\n");
					newState = 'b';
					lives--;
				}
				
				if(lives == 0) {
					printf("Client ran out of lives.\n");
					newState = 'd';
					going = 0;
				}
				
				toSend.lives = lives;
				toSend.state = newState;
				toSend.realNum = 0;
				
				if(!going) {
					toSend.realNum = num;
				}
				
				send(new_socket, (unsigned char*) &toSend, sizeof(struct ResponseData), 0);
			}
			
			// Has the socket connection failed? Abort, if so.
			if(newState == 'e') {
				closeSocket(new_socket);
				break;
			};
			
			// Send the real value.
			printf("Game over!\n");
			
			// Does the client want to go about it again?
			unsigned short int goAgain = 0;
			int nErr = recv(new_socket, (unsigned char*) &goAgain, sizeof(unsigned short int), 0);
			
			if(goAgain > 0) {
				printf("\n\nAnother round!\n\n");
			} else {
				printf("\n\nClosing socket...\n\n");
				closeSocket(new_socket);
				break;
			}
		}
	}
	
	printf("\nConnection closed.\n");
	
	getchar();

	closeSocket(s);
	cleanup();

	return 0;
}

// Generates a random number between min and max.
int generateRandNum(int min, int max) {
	srand(time(NULL));
	return (rand() % (max - min + 1)) + min;
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
