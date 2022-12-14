#include <winsock2.h>
#include <stdio.h>
#include <sys/time.h>

#define PORT     8080
#define BUFFER_SIZE 1024
#define InitialType 1
#define TerminalType 2

int alternateNum (int prevNum) {
   if (prevNum == 0) {
       return 1;
   }
    else {
        return 0;
    }
}

int findfileSize(FILE *fp) {

    if (fp == NULL) { // checking whether the file exists or not
      printf("File Not Found!\n");
      return -1;
    }

fseek(fp, 0L, SEEK_END); // setting pointer to end of file
   int res = ftell(fp); //counting the size of the file
   fseek(fp, 0, SEEK_SET); // setting pointer to start of file for traversing
   return res;
}

typedef struct {
    int Type;
    int seqNum;
    int length;
    char bytes[BUFFER_SIZE];
} senderpck;

typedef struct {
    int seqNum;
    int Type;
}recvrPck;

    recvrPck sendPacket(int sockfd, struct sockaddr_in servaddr, senderpck Frame, int i, int nBytes) {
    int len;
    recvrPck Ack;

    // send message to server
    sendto(sockfd, &Frame, sizeof(Frame), 0, (struct sockaddr*)
    &servaddr, sizeof(servaddr));

    Frame.seqNum = (i%2);
    Frame.length = nBytes;

    printf("Message sent.\n");

    while(recvfrom(sockfd, &Ack, sizeof(Ack), 0,
    (struct sockaddr*) &servaddr, &len)<0) {

    //timeout reached
    printf("Timout reached. Resending segment %d\n", Frame.seqNum);

    recvfrom(sockfd, &Ack, sizeof(Ack), 0,
    (struct sockaddr*) &servaddr, &len);
    	}
    return Ack;
   }

int main(int argc, char **argv)
{
     WSADATA              wsaData;
     SOCKET               SendingSocket;
     SOCKADDR_IN          ReceiverAddr, SrcInfo, FromAddr;
     unsigned int fromSize;
     int                  Port = 5150;

     senderpck Frame;
    recvrPck Ack;

     // Initialize Winsock version 2.2
     if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
     {
          printf("Client: WSAStartup failed with error %ld\n", WSAGetLastError());
          // Clean up
          WSACleanup();
          // Exit with error
          return -1;
     }
     else
          printf("Client: The Winsock DLL status is %s.\n", wsaData.szSystemStatus);

     // Create a new socket to receive datagrams on.
     SendingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
     if (SendingSocket == INVALID_SOCKET)
     {
          printf("Client: Error at socket(): %ld\n", WSAGetLastError());
          // Clean up
          WSACleanup();
          // Exit with error
          return -1;
     }
     else
          printf("Client: socket() is OK!\n");

     // Set up a SOCKADDR_IN structure that will identify who we
     // will send datagrams to. For demonstration purposes, let's
     // assume our receiver's IP address is 127.0.0.1 and waiting
     // for datagrams on port 5150.
     ReceiverAddr.sin_family = AF_INET;
     ReceiverAddr.sin_port = htons(Port);
     ReceiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
     // Send a datagram to the receiver.
     printf("Client: Sending datagrams...\n");

char *filename = argv[1];
    FILE *fl = fopen(filename, "r"); // opening a file in read mode;
    int filesize = findfileSize(fl);

    if (argc < 2)   {
    printf("Missing Filename\n");
    return(1);
   }

    int nBytes = BUFFER_SIZE;
    int arraySize = filesize/BUFFER_SIZE;
    int i = 0;

    int timeout = 100000;
     unsigned int time_sz = sizeof(timeout);

    //setting timeout
    /*struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;*/
    if (setsockopt(SendingSocket, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,time_sz) < 0) {
    perror("Error");
    }

	Frame.Type = InitialType;

    while (1) {
    fread(&(Frame).bytes, nBytes, 1, fl);
    Ack = sendPacket(SendingSocket, ReceiverAddr, Frame, i, nBytes);


    while (Ack.seqNum != alternateNum(Frame.seqNum)
    || Ack.Type != Frame.Type) {
       	Ack = sendPacket(SendingSocket, ReceiverAddr, Frame, i, nBytes);
      }

	if (Ack.Type == TerminalType) {
	printf("Final message received");
	break;
	}

    printf("Message Received\n");

    memset(&Frame,'\0' ,BUFFER_SIZE);

        i++;
        if (i == arraySize) {
        nBytes = filesize%BUFFER_SIZE;
        }

	if (i > arraySize) {
	Frame.Type = TerminalType;
	nBytes = 0;
	}
    }
    fclose(fl);


   // When your application is finished receiving datagrams close the socket.
   printf("Client: Finished sending. Closing the sending socket...\n");
   if (closesocket(SendingSocket) != 0)
        printf("Client: closesocket() failed! Error code: %ld\n", WSAGetLastError());
   else
        printf("Server: closesocket() is OK\n");

   // When your application is finished call WSACleanup.
   printf("Client: Cleaning up...\n");
   if(WSACleanup() != 0)
        printf("Client: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
   else
        printf("Client: WSACleanup() is OK\n");
   // Back to the system
   return 0;
}

