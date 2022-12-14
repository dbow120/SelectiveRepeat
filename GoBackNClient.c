#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/socket.h>// for socket(), connect(), sendto() and recvfrom() 
#include <arpa/inet.h> // for sockaddr_in and inet_addr

#define PORT     8080
#define BUFFER_SIZE 1024
#define InitialType 1
#define TerminalType 2
#define QueryType 0

int alternateNum (int prevNum, int sendSize) {
   if (prevNum == (sendSize - 1)) {
       return 0;
   }
    else {
	prevNum++;
        return prevNum;
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

    recvrPck sendPacket(int windowSize, int sockfd, struct sockaddr_in servaddr, senderpck Frame[],int posArr, senderpck Query, int arrSize) {
    int len;
    recvrPck Ack;

	if ((arrSize - posArr) < (windowSize-1)) {
	windowSize = arrSize - posArr +3;
	}
    
    // send message to server
	for (int index = 0; index < windowSize-1; index++) {
    	sendto(sockfd, &Frame[index+posArr], sizeof(Frame[index+posArr]), 0, (struct 		sockaddr*)
    &servaddr, sizeof(servaddr));
	}
    
    printf("Message sent.\n");
       
    while(recvfrom(sockfd, &Ack, sizeof(Ack), MSG_WAITALL,
    (struct sockaddr*) &servaddr, &len)<0) {
    
    //timeout reached
    printf("Timout reached. Sending Query Packet\n");
    
    // send message to server
    sendto(sockfd, &Query, sizeof(Query), 0, (struct sockaddr*)
    &servaddr, sizeof(servaddr));
    
    printf("Message sent.\n");
    	}
    return Ack;
   }
 
int main(int argc, char *argv[])  {

    int sockfd; //Descriptor which receives data.
    int senderSize = N;
    int index1 = 0;
    struct sockaddr_in servaddr;
    senderpck Query;
    
	Query.seqNum = -1;
	Query.Type = QueryType;
	Query.length = 0;
	

    // Creating socket to receive datagrams 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information, assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(PORT);
    
    char *filename = argv[1];
    FILE *fl = fopen(filename, "r"); // opening a file in read mode;
    int filesize = findfileSize(fl);
    
    if (argc < 2)   {
    printf("Missing Filename\n");
    return(1);
   }
    
    int nBytes = BUFFER_SIZE;
    int arraySize = filesize/BUFFER_SIZE;
	if(filesize % BUFFER_SIZE != 0) {
	arraySize++;
	}
    
    //setting timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Error");
    }
    
    senderpck Frame[arraySize+1];
    recvrPck Ack;    

	//Make_Frame();
for (int index = 0; index <= arraySize; index++) {
    fread(&(Frame[index]).bytes, nBytes, 1, fl);

// Initialise sequence number of outbound frame	
	if(index == 0) {
   	Frame[index].seqNum = 0;
    	Frame[index].length = nBytes;
    	Frame[index].Type = InitialType;
	}

	else if (index == arraySize-1) {
    	Frame[index].seqNum = alternateNum(Frame[index-1].seqNum, senderSize);
    	Frame[index].length = filesize%BUFFER_SIZE;;
    	Frame[index].Type = InitialType;
	}

	else if (index == arraySize) {
  	Frame[index].Type = TerminalType;
   	Frame[index].seqNum = alternateNum(Frame[index-1].seqNum, senderSize);
    	Frame[index].length = nBytes;
	}

	else {
    Frame[index].seqNum = alternateNum(Frame[index-1].seqNum, senderSize);
    Frame[index].length = nBytes;
    Frame[index].Type = InitialType;
		}
	}

	if (senderSize < arraySize+1) {
	    senderSize = arraySize+2;
	}

    
    while (1) {
		//Send_Frame_To_Physical_Layer();
    Ack = sendPacket(senderSize, sockfd, servaddr, Frame, index1, Query, arraySize);
	index1 +=(senderSize-1);
    
	//if (Acknowledgement_Arrival) then Receive_ACK();
    while (Ack.seqNum != alternateNum(Frame[index1].seqNum,senderSize)) {
	index1--;
    }

	if (Ack.Type == TerminalType) {
	printf("Final message received");
	break;
	}
      
    printf("Message Received\n");

    }
    fclose(fl);
    
    return 0;
}
