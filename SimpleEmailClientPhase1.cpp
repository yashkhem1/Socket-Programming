#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include<arpa/inet.h> //inet_ntop
#include <string.h>
#include <iostream>
#include <fstream>
#include<sys/un.h>
using namespace std;

//End the iostream strings with "\n" always.

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage : ./main <serverIPAddr:port> <user-name> <passwd>";
        exit(1);
    }

    else {
        string ip_port = argv[1];
        int colon_pos = ip_port.find(":");
        string server_ip = ip_port.substr(0, colon_pos);
        const char *serv_ip = server_ip.c_str();
        int server_port = stoi(ip_port.substr(colon_pos + 1));
        //cout << server_ip << "\n";
        //cout << server_port << "\n";
        string username = argv[2];
        string passwd = argv[3];

        //Initializing Buffers
        char inbuffer[1024] = {0};
        char outbuffer[1024] = {0};

        struct sockaddr_in my_addr, serv_addr;
        int client_fd;

        //Initializing Client File Descriptor
        client_fd = socket(PF_INET, SOCK_STREAM, 0);

        //Initializing the Value of serv_addr
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);
        memset(&(serv_addr.sin_zero), '\0', 8);
        // inet_pton(AF_INET, server_ip , &serv_addr.sin_addr)
        if (inet_pton(AF_INET, serv_ip , &serv_addr.sin_addr) <= 0) {
            cerr << "\nInvalid address/ Address not supported \n";
            exit(2);
        }

        //connecting to server
        if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            cerr << "\nConnection Failed \n";
            exit(2);
        }
        else {
            cout << "ConnectDone: " << serv_ip << ":" << server_port << "\n";
        }

        //Send the whole buffer with strcpy command

        string credentials = "User: " + username + " Pass: " + passwd + "\0";
        const char *cred = credentials.c_str();
        strcpy(outbuffer, cred);
        // cout << outbuffer;
        send(client_fd, outbuffer, 1024, 0);

        recv(client_fd , inbuffer, 1024, 0);
        // cout << "Hello" <<"\n";
        string inbuff(inbuffer);
        cout  << inbuff << "\n";

        strcpy(outbuffer, "quit\0");
        send(client_fd, outbuffer, 1024, 0);
    }
}
