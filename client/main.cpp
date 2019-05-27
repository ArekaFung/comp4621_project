#include <iostream>
#include <winsock2.h>

 
using namespace std;
 
#define MAXLINE 100

int main()
{
	char choice;
    string requestLine;
    
    cout<<"Please choose from the below links"<<endl;
    cout<<"<a> www.google.com"<<endl;
    cout<<"<b> sing.cse.ust.hk"<<endl;
    cout<<"<c> google.com"<<endl;
    cout<<"<d> www.youtube.com"<<endl;
    cout<<"<other> Quit"<<endl;
    
    choice = getchar();
    
    if(choice == 'a')
    	requestLine = "GET https://www.google.com/ HTTP/1.0\r\n\r\n";
    else if(choice == 'b')
    	requestLine = "GET https://sing.cse.ust.hk/ HTTP/1.0\r\n\r\n";
    else if(choice == 'c')
    	requestLine = "GET https://google.com/ HTTP/1.0\r\n\r\n";
    else if(choice == 'd')
    	requestLine = "GET https://www.youtube.com/?gl=HK/ HTTP/1.0\r\n\r\n";
    else
    	return 0;
    
	//string requestLine = "GET http://172.217.163.238/ HTTP/1.0\r\n\r\n";
	
	int sockfd, n;
    struct sockaddr_in servaddr;
    char recvline[MAXLINE] = {0};
    
    
    //wsa
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,0), &wsaData) == SOCKET_ERROR) {
        printf ("Error initialising WSA.\n");
		fflush(stdin);getchar();
        return -1;
    }

    /* init socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);       /* TCP */
    if (sockfd < 0) {
        printf("Error: init socket\n");
        fflush(stdin);getchar();
        return 0;
    }

    /* init server address (IP : port) */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(12345);

    /* connect to the server */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Error: connect\n");
        fflush(stdin);getchar();
        return 0;
    }
    
    
    send(sockfd, requestLine.c_str(), requestLine.length(), 0);

    /* read the response */
    while (1) {
        n = recv(sockfd, recvline, sizeof(recvline) - 1,0);

        if (n <= 0) {
                break;
        } else {
                recvline[n] = 0;        /* 0 terminate */
                printf("%s", recvline);
        }
    }

    /* close the connection */
    closesocket(sockfd);
    fflush(stdin);getchar();
    return 0;
}
