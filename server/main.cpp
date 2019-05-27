#include <iostream>
#include <winsock2.h>
#include <time.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include <chrono>
#include <thread>

using namespace std;
 
#define MAXLINE 100
#define SERVER_PORT 12345
#define LISTENNQ 5
#define MAXTHREAD 5

void* request_func(void *args);

void getHTTP(string requestString, addrinfo* IPaddr, string* outputString, char* ip_str){
	int sockfd, n;
	char recvline[MAXLINE] = {0};
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = IPaddr->ai_family;
    servaddr.sin_addr.s_addr = inet_addr(ip_str);
    servaddr.sin_port = htons(80);

    /* init socket */
    sockfd = socket(IPaddr->ai_family, IPaddr->ai_socktype, IPaddr->ai_protocol);       /* TCP */
    if (sockfd < 0) {
        printf("Error: init socket\n");
        return;
    }
	
    /* connect to the server */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Error: connect\n");
    }
	
    /* send the request */
    send(sockfd, requestString.c_str(), requestString.length(),0);

    /* read the response */
    while (1) {
        n = recv(sockfd, recvline, sizeof(recvline) - 1,0);

        if (n <= 0) {
             break;
        } else {
        	recvline[n] = 0;        /* 0 terminate */
			outputString->append(recvline);
        }
    }

    /* close the connection */
    closesocket(sockfd);
    return;
}

void readRequest(int connfd, string* requestString, string* remoteIP){
	int i,j,n;
	char buff = 0;
	char last4chars[5] = {0};
	char* terminator = "\r\n\r\n";
	string received = "";
	
	i=0;
	
	while(1) {
		n = recv(connfd, &buff, sizeof(buff),0);

		if(n<=0) {
			break;
		}

		received += buff;

		/* retrive last 4 chars */
		for(j = 0; j < 4; ++j) {
			last4chars[j] = received[i - 3 + j];
		}
		i++;
		
		/* HTTP header end with "\r\n\r\n" */
		if(strcmp(last4chars, terminator) == 0) {
			break;
		}
	}
	
	if(received.length()>=7){
		for(int ipStartLocation=7; ipStartLocation<received.length(); ipStartLocation++){
			if(strcmp(received.substr(ipStartLocation-7,7).c_str(),"http://") == 0){
				int ipEndLocation = ipStartLocation+1;
				
				while(1){
					if(received.at(ipEndLocation) == '/') break;
					else ipEndLocation++;
					
					if(ipEndLocation>received.length()){ //invalid request
						return;
					}
				}
							
				*remoteIP = received.substr(ipStartLocation, ipEndLocation-ipStartLocation);
				received.erase(ipStartLocation-7, ipEndLocation-ipStartLocation+7);
				*requestString = received;
				
				cout<<"HTTP request"<<endl;
				return;
			}
		}
		
		for(int ipStartLocation=8; ipStartLocation<received.length(); ipStartLocation++){
			if(strcmp(received.substr(ipStartLocation-8,8).c_str(),"https://") == 0){
				int ipEndLocation = ipStartLocation+1;
				
				while(1){
					if(received.at(ipEndLocation) == '/') break;
					else ipEndLocation++;
					
					if(ipEndLocation>received.length()){ //invalid request
						return;
					}
				}
							
				*remoteIP = received.substr(ipStartLocation, ipEndLocation-ipStartLocation);
				received.erase(ipStartLocation-8, ipEndLocation-ipStartLocation+8);
				*requestString = received;
				
				cout<<"HTTPS request"<<endl;
				return;
			}
		}
		
		return; //also invalid request if reached here
	}		
}

bool accessControl(string requestedHostName){
	const int BANNED_LIST_SIZE = 3;
	string banned_list[BANNED_LIST_SIZE];
	
	banned_list[0] = "sing.cse.ust.hk";
	banned_list[1] = "google.com";
	banned_list[2] = "www.youtube.com";
	
	//for getting the ip address from url
    addrinfo hints = {0};
	hints.ai_flags = 0;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	addrinfo *addr = NULL;
	
	
	cout<<"Requested Host: "<<requestedHostName<<endl;
	
	for(int i=0; i<BANNED_LIST_SIZE; i++){
		if(banned_list[i].compare("") != 0){
			int ret = getaddrinfo(banned_list[i].c_str(), NULL, &hints, &addr);
			if(ret!=0){
				cout<<"Error in banned list URL: "<<i<<" th item"<<endl;
				continue;
			}
			
			struct sockaddr_in *temp = (struct sockaddr_in *)addr->ai_addr;
			char ip_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(temp->sin_addr), ip_str, INET_ADDRSTRLEN);
			
			char hostname[NI_MAXHOST];
		    int error = getnameinfo(addr->ai_addr, addr->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
		    cout<<"Banned Host: "<<hostname<<endl;
			if(requestedHostName.compare(hostname) == 0) return true;
		}
	}
	
	return false;
}

int main()
{
	cout<<"----- Proxy Server -----"<<endl;
	
 	int listenfd, connfd; //fd for local port, requesting client port
    struct sockaddr_in servaddr, cliaddr;
    int len = sizeof(struct sockaddr_in);
    
    //multi_thread
    int threads_count = 0;
	pthread_t threads[MAXTHREAD];
    
    //wsa
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,0), &wsaData) == SOCKET_ERROR) {
        printf ("Error initialising WSA.\n");
        return -1;
    }

    /* initialize client-to-server entering socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
    if (listenfd < 0) {
            printf("Error: init socket\n");
            return 0;
    }

    /* initialize server address (IP:port) */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; /* IP address: 0.0.0.0 */
    servaddr.sin_port = htons(SERVER_PORT); /* port number */

    /* bind the socket to the server address */
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
        printf("Error: bind\n");
        return 0;
    }

    if (listen(listenfd, LISTENNQ) < 0) {
        printf("Error: listen\n");
        return 0;
    }

    /* keep processing incoming requests */
    while (1) {
        /* accept an incoming connection from the remote side */
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        if (connfd < 0) {
            printf("Error: accept\n");
            return 0;
        }
        
        cout<<"A request arrived"<<endl;
        
        /* create dedicate thread to process the request */
		if (pthread_create(&threads[threads_count], NULL, request_func, (void *)connfd) != 0) {
			printf("Error when creating thread %d\n", threads_count);
			return 0;
		}

		if (++threads_count >= MAXTHREAD) {
			break;
		}
        
    }
    
	printf("Max thread number reached, wait for all threads to finish and exit...\n");
	for (int i = 0; i < MAXTHREAD; ++i) {
		pthread_join(threads[i], NULL);
	}

    return 0;
}

void* request_func(void* args){
	int connfd = (int)args;
    string requestString, remoteIP;
    
    this_thread::sleep_for(chrono::milliseconds(10));
    
    readRequest(connfd, &requestString, &remoteIP);
    
    cout<<"original IP/URL: "<<remoteIP<<endl<<"Req: "<<requestString<<endl;
    
    
    //for getting the ip address from url
    addrinfo hints = {0};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	addrinfo *addr = NULL;
	
	int ret = getaddrinfo(remoteIP.c_str(), NULL, &hints, &addr);
	
	if(ret == EAI_NONAME){ //s.t. it is host name not ip address
		hints.ai_flags = 0;
		ret = getaddrinfo(remoteIP.c_str(), NULL, &hints, &addr);
	}
	
	struct sockaddr_in  *temp = (struct sockaddr_in *)addr->ai_addr;
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(temp->sin_addr), ip_str, INET_ADDRSTRLEN);
	
	cout<<"interpreted IP: "<<ip_str<<endl;
	
	char hostname[NI_MAXHOST];
    int error = getnameinfo(addr->ai_addr, addr->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
	
	if(accessControl(hostname) != true){
		string outputString;
        getHTTP(requestString, addr, &outputString, &ip_str[0]);
		
        /* send response */
        send(connfd, outputString.c_str(), outputString.length(),0);
	}else{
		string error404 = "";
		error404+= "<!DOCTYPE HTML PUBLIC ";
		error404+= '"';
		error404+= "-//IETF//DTD HTML 2.0//EN";
		error404+= '"';
		error404+= ">";
		error404+= '\n';
		
		error404+= "<HTML><HEAD>";
		error404+= '\n';
		
		error404+= "<TITLE>404 Not Found</TITLE>";
		error404+= '\n';
		
		error404+= "</HEAD><BODY>";
		error404+= '\n';
		
		error404+= "<H1>Not Found</H1>";
		error404+= '\n';
		
		error404+= "This website is not available or the file is missing on this server.<P>";
		error404+= '\n';

		error404+= "<HR>";
		error404+= '\n';
		
		error404+= "<ADDRESS>Local Proxy Server</ADDRESS>";
		error404+= '\n';
		
		error404+= "</BODY></HTML>";
		error404+= '\n';
		
		send(connfd, error404.c_str(), error404.length(), 0);
	}
	
    /* close the connection */
    closesocket(connfd);
}
