// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT     8080 
#define BUFFER_SIZE 1024
#define TerminalType 2
#define InitialType 1
#define NegACKType -1

typedef struct {
    int Type;
    int seqNum;
    int length;
    char bytes[BUFFER_SIZE];
} dataPck;

typedef struct {
    int seqNum;
    int Type;
}ackPck;

int alternateNum (int prevNum, int sendSize) {
   if (prevNum == sendSize*2) {
       return 0;
        }
    else {
	prevNum++;
        return prevNum;
        }
    }
    
    ackPck SelecRepeatServer(int windowSize, int sockfd, dataPck Frame,
    struct sockaddr_in cliaddr, int len, ackPck ACK, char* storedData, int timesNAKsent) {
	
    int expectedNAK = 0;

	//Receive N-1 packets ();
	for (int i = 0; i<windowSize-1; i++) {
    recvfrom(sockfd, &Frame, sizeof(Frame),  
    MSG_WAITALL,(struct sockaddr*) &cliaddr, &len);
    
	//if (frame.SeqNo = RSeqNo) then Extract_Data();
	if(Frame.seqNum == ACK.seqNum) {
    	strcpy(storedData, Frame.bytes);
    
    	ACK.Type = Frame.Type;

	printf("Data Received\n");

		}
	else {
	// Corrupted SendNAK(current SeqNo);
	printf("NAK found\n");
	ACK.Type = NegACKType;
	timesNAKsent++;
		}

		//RSeqNo = RSeqNo + 1;
	ACK.seqNum = alternateNum(ACK.seqNum, windowSize);

	//Send N-1 Packets(RSeqNo);
   	sendto(sockfd,&ACK, sizeof(ACK),  
    0, (const struct sockaddr *) &cliaddr, len); 
	printf("ACK message sent.\n");
	
	if (ACK.Type == TerminalType) {
	printf("Final ACK message sent. \n");
	break;
			}
		}

		int expectedNAKS = 0;
	/*if(timesNAKsent > expectedNAKS) {
		timesNAKsent++;
//	ACK = SelecRepeatServer(timesNAKsent, sockfd, Frame,
//    cliaddr, len, ACK, storedData, expectedNAK);   
	 }
//	*/
    return ACK;
}
  
// Driver code 
int main() { 
    int sockfd, len, n;
    int windowSize = 4;
    int timesNAKsent = 0;
    FILE *f1;
    char* storedData;
    struct sockaddr_in servaddr, cliaddr;
    dataPck Frame;
    ackPck ACK;

	ACK.seqNum = 0;	// Initialize sequence number of expected frame
	ACK.Type = InitialType;
      
    // Creating socket file descriptor 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)
    &servaddr, sizeof(servaddr)) < 0 )  { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    len = sizeof(cliaddr);  //len is value/resuslt
    storedData = (char*)malloc(BUFFER_SIZE);

    
	while(1) {

	ACK = SelecRepeatServer(windowSize, sockfd, Frame,
    cliaddr, len, ACK, storedData, timesNAKsent);

	printf("1 ACK.Type = %d\n", ACK.Type);
	
	if (ACK.Type == TerminalType) {
	break;
		}	
		
	}
//	*/      
    return 0; 
} 
