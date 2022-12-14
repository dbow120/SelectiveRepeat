#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/socket.h> // for socket(), connect(), sendto() and recvfrom() 
#include <arpa/inet.h> // for sockaddr_in and inet_addr

#define PORT     8080
#define BUFFER_SIZE 1024
#define InitialType 1
#define TerminalType 2
#define NegACKType -1
#define QueryType 0

int alternateNum (int prevNum, int sendSize) {
   if (prevNum == (sendSize*2)) {
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
} dataPck;

typedef struct {
    int seqNum;
    int Type;
}recvrPck;

dataPck createDataPacket(dataPck Frame[], int index, FILE* fl, int arraySize, int windowSize, int fileSize) {

	int nBytes;	

	//configure byte size depending on index
	if(index == arraySize-2) {
	 nBytes = fileSize%BUFFER_SIZE;
	}
	else {
	nBytes = BUFFER_SIZE;
	}

	fread(&(Frame[index]).bytes, nBytes, 1, fl);

	Frame[index].seqNum = alternateNum(Frame[index-1].seqNum, windowSize);
	Frame[index].length = nBytes;
	Frame[index].Type = InitialType;

    // Initialise sequence number of outbound frame	
	if(index == 0) {
   	Frame[index].seqNum = 0;
	}

	if (index == arraySize-1) {
  	Frame[index].Type = TerminalType;
	}

	return Frame[index];
		}


    recvrPck SelecRepeat(int windowSize, int sockfd, struct sockaddr_in
	 servaddr, dataPck Frame[],int posArr, int arrSize, recvrPck Ack) {
    
    int addrlen = sizeof(servaddr);
    dataPck Query; // Sent in case ACK pac is not received
    int NAKPackCount = 0; //Determine number of packets to resend
    
	//Initializing query pck to distinguish from frame	
	Query.seqNum = -1;
	Query.Type = QueryType;
	Query.length = 0;

	if ((arrSize - posArr) < (windowSize-1)) {
	windowSize = arrSize - posArr+1;
	}
	dataPck lostPck[windowSize];// store packets not sent to server
       
	//Send N-1 packets
	for (int index = 0; index < windowSize-1; index++) {
    	sendto(sockfd, &Frame[index+posArr], sizeof(Frame[index+posArr]),
	0, (struct  sockaddr*) &servaddr, addrlen);
		}
	printf("Frame packet sent.\n");

	//Receive N-1 packet
	for (int i = 0; i<windowSize-1; i++) {

	//If pck is missing(recvfrom<0) send Query
   	while(recvfrom(sockfd, &Ack, sizeof(Ack), MSG_WAITALL,
	(struct sockaddr*) &servaddr, &addrlen)<0) {
    
    	//timeout reached
    	printf("Timout reached. Sending Query Packet\n");
    
    	// send message to server
    	sendto(sockfd, &Query, sizeof(Query), 0, (struct sockaddr*)
    	&servaddr, addrlen);
    
    	printf("Query Message sent\n");
    	}

	//Check for NAK Packs
	//if (Acknowledgement_Arrival) then Receive_ACK();
	if (Ack.seqNum == alternateNum(Frame[i+posArr].seqNum, windowSize)) {
	printf("ACK Packet received\n");	
	}

	else if (Ack.Type == NegACKType) {
	printf("received NAK Packet\n");
	lostPck[NAKPackCount] = Frame[i+posArr];//storing lost packets
	NAKPackCount++;
			}

/*	   else {
	printf("Lost Packet\n");
	lostPck[NAKPackCount] = Frame[i+posArr];//storing lost packets
	NAKPackCount++;
		}*/
	}
	int expectedNAKS = 0;

	if(NAKPackCount > expectedNAKS) {
		NAKPackCount++;
	//resending lost packets using recursion;
  	Ack = SelecRepeat(NAKPackCount,sockfd, servaddr, lostPck,
	expectedNAKS, arrSize, Ack);
	}	

	//returning final packet in sequence	
    return Ack;
   }
 
int main(int argc, char *argv[])  {

    int sockfd; //Descriptor which receives data.
    int windowSize = 4;
    int posArr = 0;
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    recvrPck Ack;

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
    int fileSize = findfileSize(fl);
    
    if (argc < 2)   {
    printf("Missing Filename\n");
    return(1);
   }
    
    int nBytes = BUFFER_SIZE;
    int arraySize = fileSize/BUFFER_SIZE + 1;
	if(fileSize % BUFFER_SIZE != 0) {
	arraySize++;
	}

	if (windowSize > arraySize) {
	    windowSize = arraySize+1;
	}
    
    //setting timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 100000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Error");
    }
    
    dataPck Frame[arraySize];   

		//Make_Frame();
    	for (int i = 0; i < arraySize; i++) {
    Frame[i] = createDataPacket(Frame, i, fl, arraySize, windowSize, fileSize);
	}
	
	 while (1) {
		//Send_Frame_To_Physical_Layer();
   	Ack = SelecRepeat(windowSize, sockfd, 
	servaddr, Frame, posArr, arraySize, Ack);   
	
		posArr +=(windowSize-1); 

	if (Ack.Type == TerminalType) {
	printf("Final ACK Packet received\n");
	break;
	}
    }
    fclose(fl);
//	*/
    
    return 0;
}
