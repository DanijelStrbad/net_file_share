#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "net_functions.h"

#define MAXLEN 1400



void printHelp() {
	printf("\nHelp:\n");
	printf("tcpserver [-p port] [-v] [-h]\n\n");
	return;
}



int main(int argc, char *argv[]) {
	char debug = (char)(0);
	
	int flagP = 0;
	char inChar;
	char portIn[10] = "1234";
	int i, onFlag = 1;
	
	int sockTCP, newSock;
	struct sockaddr_in addrTCPserv;
	struct addrinfo hints, *res;
	
	struct sockaddr *addrCli;
	socklen_t *cliLen;
	
	ssize_t recvLen;
	int recvLenTmp;
	char msgBuff[MAXLEN], offsetChar[4], fileName[100];
	int offsetInt = 0;
	int err123;
	char messageFile[25] = "File error\n";
	
	FILE *locFile;
	char fileBuff[MAXLEN];
	int fileBuffSize;
	
	
	while( (inChar = getopt(argc, argv, "p:v")) != -1) {
		switch(inChar) {
			case 'p':	flagP = 1;
						break;
			case 'v':	debug = 1;
						break;
			default: printHelp();
				err(1, "\n\nWrong input option(s) (exit code 1)\n");
		}
	}
	if( argc-optind != 0) {
		printHelp();
		err(1, "\n\nWrong input option(s) (exit code 1)\n");
	}
	
	
	/* set network */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags |= AI_PASSIVE;
	//hints.ai_flags |= AI_CANONNAME;
	if(flagP == 0) {
		Getaddrinfo(NULL, portIn, &hints, &res);
		if(debug) {
			printf("\nPort: %s\n", portIn);
		}
	} else {
		Getaddrinfo(NULL, optarg, &hints, &res);
		if(debug) {
			printf("\nPort: %s\n", optarg);
		}
	}
	
	sockTCP = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if( setsockopt(sockTCP, SOL_SOCKET, SO_REUSEADDR, &onFlag, sizeof(int)) == -1 ) {
		err(22, "\n\nNet problem - setsockopt (exit code 22)\n");
	}
	
	Bind(sockTCP, res->ai_addr, res->ai_addrlen);
	
	Listen(sockTCP, 1);
	if(debug) {
		printf("\nListen done\n");
	}
	
	while(1) {
		newSock = Accept(sockTCP, NULL, NULL);
		if(debug) {
			printf("\nAccept done\n");
		}
		
		/*if( setsockopt(newSock, IPPROTO_TCP, SO_REUSEADDR, &onFlag, sizeof(int)) == -1 ) {
			err(23, "\n\nNet problem - setsockopt, newSock - from Accept (exit code 23)\n");
		}*/
		
		memset(msgBuff, (char) 0, sizeof(msgBuff));
		// recvLen = Recv(newSock, msgBuff, MAXLEN-1, 0);
		recvLen = Recv(newSock, &msgBuff[0], 1, 0);
		recvLen = Recv(newSock, &msgBuff[1], 1, 0);
		recvLen = Recv(newSock, &msgBuff[2], 1, 0);
		recvLen = Recv(newSock, &msgBuff[3], 1, 0);


		recvLenTmp = 4;
		for(i = 0; 1; i++) {
			recvLen = Recv(newSock, &msgBuff[4+i], 1, 0);
			recvLenTmp++;
			if( msgBuff[4+i] == (char)0 ) {
				break;
			}
		}
		
		offsetChar[0] = msgBuff[0];
		offsetChar[1] = msgBuff[1];
		offsetChar[2] = msgBuff[2];
		offsetChar[3] = msgBuff[3];
		for(i = 0; 1; i++) {
			fileName[i] = msgBuff[i+4];
			if(msgBuff[i+4] == (char) 0) {
				break;
			}
		}
		
		
		offsetInt = (offsetChar[0] << 24) | ((offsetChar[1] & 0xFF) << 16) | 
							((offsetChar[2] & 0xFF) << 8) | (offsetChar[3] & 0xFF);
		
		if(debug) {
			printf("\nRecieved:\n");
			for(i = 0; i<recvLenTmp; i++) {
				printf("%c", msgBuff[i]);
			}
			printf("\noffsetInt = %d\nfileName = %s\n\n", offsetInt, fileName);
		}
		
		
		locFile = fopen(fileName, "rb");
		if( locFile == NULL ) {
			if(debug) {
				printf("\nFile %s error\n", fileName);
			}
			
			memset(fileBuff, (char) 1, sizeof(fileBuff));
			Send(newSock, fileBuff, 1, 0);
			Send(newSock, messageFile, (sizeof (char)) * strlen(messageFile), 0);
			
		} else {
			if(debug) {
				printf("\nFile %s success\n", fileName);
			}
			
			memset(fileBuff, (char) 0, sizeof(fileBuff));
			Send(newSock, fileBuff, 1, 0);
			
			while( !feof(locFile) ) {
				memset(fileBuff, '\0', sizeof(fileBuff));
				fileBuffSize = fread (fileBuff, sizeof (char), MAXLEN, locFile);
				if(fileBuffSize < 0) {
					err(200, "\n\nFile read problem (exit code 200)\n");
				}
				
				Send(newSock, fileBuff, (sizeof (char))*fileBuffSize, 0);
			}
			if(debug) {
				printf("\nFile %s sent\n", fileName);
			}
		}
		close(newSock);
	}
	
	return 0;
}

