#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

using namespace std;

const int QUEUE = 10;
//Server side
int main(int argc, char *argv[])
{
    ofstream fout;
    fout.open("log.txt");
    if ( !fout )
    {
      cerr << "ERROR - output file failed to open" << endl;
      exit(0);
     }

    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    //port number
    int port = atoi(argv[1]);
    //buffer to receive messages 
    char msg[1500];
    // socket 
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    //open stream socket with internet address
    int serverVal = socket(AF_INET, SOCK_STREAM, 0);
    if(serverVal < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }
    //bind the socket to its local address
    int bindVal = bind(serverVal, (struct sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindVal < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    cout << "Server Active\nWaiting for client to connect. " << endl << endl;
    //listen 
    listen(serverVal, 5);
    // new address to connect with the client
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    sockaddr_in* arr = new sockaddr_in[QUEUE];
    for(int i = 0; i < QUEUE; i++)
    {
        arr[i].sin_addr.s_addr = 0;
    }
    time_t serverStart = time(NULL);
    time_t start [QUEUE];
    time_t end;
    
    //accept, create a new socket descriptor handle connection with client
    // put server in infinate loop
    while(true)
    {
        int newConnect = accept(serverVal, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if(newConnect < 0)
        {
            cerr << "Error accepting request from client" << endl;
            exit(1);
        }
        cout << "Connected with client from IP "  << inet_ntoa(newSockAddr.sin_addr) << ", port "   
        << ntohs(newSockAddr.sin_port) << endl;
        memset(&msg, 0, sizeof(msg));//clear the buffer
        read(newConnect, (char*)&msg, sizeof(msg));
        int choice = atoi(msg);
        if(choice == 1)
        {
            cout << "The action requested is $JOIN. "  << endl << endl;
            bool member = false;
            fout << "TIME: " << (time(NULL) - serverStart) 
                 <<" Recieved a #JOIN from agent "
                 << inet_ntoa(newSockAddr.sin_addr) << endl;

           for(int i = 0; i < QUEUE; i++)
           {
               if(arr[i].sin_addr.s_addr == newSockAddr.sin_addr.s_addr)
               {
                   char* s = "$ALREADY MEMBER";
                   write(newConnect, s, strlen(s));
                   fout << "TIME: " << (time(NULL) - serverStart) 
                         << " Responded to agent "
                        << inet_ntoa(newSockAddr.sin_addr) << " with "<< "$ALREADY MEMBER." << endl;
                        member = true;
                    break;
               }
           }
           if (!member)
            {    for(int i = 0; i < QUEUE; i++)
                {
                    if(arr[i].sin_addr.s_addr == 0)
                    {
                        arr[i] = newSockAddr;
                        start[i] = time(NULL);
                        char* s = "$OK";
                        write(newConnect, s, strlen(s));
                        fout << "TIME: " << (time(NULL) - serverStart)  
                             << " Responded to agent "
                             << inet_ntoa(newSockAddr.sin_addr) << " with "<< "$OK" << endl;
                    break;
                    }
                }
            } 
        }
        else if(choice == 2)
        {
            bool removed = false;
            cout << "The action requested is $LEAVE "  << endl << endl;
            fout << "TIME: " << (time(NULL) - serverStart) 
                 <<" Recieved a #LEAVE from agent "
                << inet_ntoa(newSockAddr.sin_addr) << endl;
            for(int i = 0; i < QUEUE; i++)
            {
                if(arr[i].sin_addr.s_addr == newSockAddr.sin_addr.s_addr)
                {
                    char* s = "$OK";
                    write(newConnect, s, strlen(s));
                    fout << "TIME: " << (time(NULL) - serverStart)
                         << " Responded to agent "
                        << inet_ntoa(newSockAddr.sin_addr) << " with "<< "$OK" << endl;
                    arr[i].sin_addr.s_addr = 0;
                    removed = true;
                    break;

                }
            }
            if(!removed)
            {   char* s = "$NOT MEMBER";
                write(newConnect, s, strlen(s));
                fout << "TIME: " << (time(NULL) - serverStart) 
                     << " Responded to agent "
                     << inet_ntoa(newSockAddr.sin_addr) << " with "<< "$NOT MEMBER" << endl;
            }           
        }
        else if(choice == 3)
        {
            bool member = false;
            cout << "The action requested is $LIST. "  << endl << endl;
            fout << "TIME: " << (time(NULL) - serverStart)
                 <<" Recieved a #LIST from agent "
                << inet_ntoa(newSockAddr.sin_addr) << endl;
            for(int i = 0; i < QUEUE; i++)
            {
                if(arr[i].sin_addr.s_addr == newSockAddr.sin_addr.s_addr)
                {
                    member = true;
                }
            }
            if(member)
            {
                char* s = "The following IP's are active and the amount of time in seconds they have been active.\n";
                write(newConnect, s, strlen(s));
                for(int i = 0; i < QUEUE; i++)
                {
                    if(arr[i].sin_addr.s_addr != 0)
                    {
                        char* ip = inet_ntoa(arr[i].sin_addr);
                        write(newConnect, ip, strlen(ip));
                        float tim = time(NULL) - start[i]; 
                        char t [sizeof(tim) + 1];
                        sprintf(t,"     %f",tim);
                        write(newConnect, t, strlen(t));
                    }   memset(&msg, 0, sizeof(msg));
                }
            }
            if(!member)
            {
               fout <<  "TIME: " << (time(NULL) - serverStart)
                    << " No response is supplied to agent "  
                    << inet_ntoa(newSockAddr.sin_addr) << endl;
            }    
        }
        else if(choice == 4)
        {
            bool member = false;
            cout << "The action requested is $LOG. "  << endl << endl;
            fout << "TIME: " << (time(NULL) - serverStart)
                 <<" Recieved a #LOG from agent "
                << inet_ntoa(newSockAddr.sin_addr) << endl;
            for(int i = 0; i < QUEUE; i++)
            {
                if(arr[i].sin_addr.s_addr == newSockAddr.sin_addr.s_addr)
                {
                    member = true;
                    FILE *fd = fopen("log.txt", "rb");
                    if(!fopen)
                    {
                        cerr << "File could not send";
                        exit(1);
                    }
                    while(true)
                    {
                        /* read file in chunks of 256 bytes */
                        unsigned char buff[256]={0};
                        int dataRead = fread(buff,1,256,fd);      
                        if(dataRead > 0)
                        {
                            write(newConnect, buff, dataRead);
                        }

                        if (dataRead < 256)
                        {
                            break;
                        }
                    }
                }
            }
            if(!member)
            {
                fout <<  "TIME: " << (time(NULL) - serverStart)
                    << " No response is supplied to agent "
                    << inet_ntoa(newSockAddr.sin_addr) << endl;
            }
        }
        else
        {
            cout << "No valid action is supplied" << endl << endl;
            fout <<  "TIME: " << (time(NULL) - serverStart)
                    << " No valid action is supplied by agent " 
                    << inet_ntoa(newSockAddr.sin_addr) << endl;
        }  
        memset(&msg, 0, sizeof(msg));
        close(newConnect);  
    }
    return 0;   
}