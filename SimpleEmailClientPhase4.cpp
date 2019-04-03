#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include<arpa/inet.h> //inet_ntop
#include <string.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include<sys/un.h>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <dirent.h>
using namespace std;

//End the iostream strings with "\n" always.


int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage : ./main <serverIPAddr:port> <user-name> <passwd> <list-of-messages> <local-folder>";
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

        string messagelist = argv[4];
        stringstream ss(messagelist);
        vector<int> mvector;
        string token;


        try {
            while (getline(ss, token, ',')) {
                mvector.push_back(stoi(token));
            }
        }

        catch (...) {
            cerr << "Need a list of integers";
            exit(3);
        }

        // cout << mvector.size();

        string local = argv[5];

        try{
            const char* lfolder = local.c_str();
            struct dirent *dp;
            DIR *fd;

            if (!((fd = opendir(lfolder)) == NULL)) {
                string remove = "rm -rf " + local;
                system(remove.c_str());
            }

            int test = mkdir(local.c_str(), 0777);
        }

        catch (...) {
            cerr << "Unable to create the folder";
            exit(4);
        }

        //cout << "Test " << test << "\n";


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
        if (inbuff == "fail") {
            close(client_fd);
            exit(1);
        }
        cout  << inbuff << "\n";

        //Receive okay signal regarding the user database folder
        recv(client_fd , inbuffer, 1024, 0);
        // cout << "Hello" <<"\n";
        inbuff = string(inbuffer);
        if (inbuff == "fail") {
            close(client_fd);
            exit(1);
        }

        //Remove this inbuff
        cout << inbuff << "\n";

        //cout << inbuff << "\n";   //This returns okay if user database is accessible


        sleep(6);
        cout <<"Ho gaye 2 second\n";

        //Send number of files to be sent
        string numfiles = to_string(mvector.size()) + '\0';
        strcpy(outbuffer, numfiles.c_str());
        send(client_fd, outbuffer, 1024, 0);

        //Send RETRV command
        for (int i = 0; i < mvector.size(); i++)
        {
            string retrvm = "RETRV " + to_string(mvector[i]) + '\0';
            strcpy(outbuffer, retrvm.c_str());
            send(client_fd, outbuffer, 1024, 0);

            recv(client_fd , inbuffer, 1024, 0);  //Receive the confirmation of RECV command
            if (string(inbuffer) == "fail") {
                close(client_fd);
                exit(1);
            }

            recv(client_fd , inbuffer, 1024, 0);  //Receive the file name of the file requestd
            if (string(inbuffer) == "fail") {
                close(client_fd);
                exit(1);
            }
            else{
                //cout << string(inbuffer)<<"\n";
                string filepath = local + "/" + string(inbuffer);
                FILE* wfile;
                wfile = fopen(filepath.c_str(),"wb");
                recv(client_fd, inbuffer, 1024, 0);//Rreceive the remaining bits for the last batch
                // cout << "stoi se pehle wala " << inbuffer << "\n";
                int remBytes = stoi(string(inbuffer));
                recv(client_fd , inbuffer, 1024, 0); //Receive the number of batches of file read and write
                long numBatches = stol(string(inbuffer));
                //cout << numBatches << "\n";
                for(int j = 0; j < numBatches; j++){
                    if(j==numBatches-1){
                        recv(client_fd, inbuffer, remBytes, 0);
                        fwrite(inbuffer, 1, remBytes, wfile);
                    }
                    else{
                        recv(client_fd, inbuffer, 1024, 0);
                        fwrite(inbuffer, 1, 1024, wfile);
                    }

                    // free(inbuffer);
                }

                fclose(wfile);
            }


        }

        strcpy(outbuffer, "quit");
        send(client_fd, outbuffer, 1024, 0);
        close(client_fd);
        // strcpy(outbuffer,"LIST\0");
        // send(client_fd, outbuffer, 1024, 0);
        // recv(client_fd , inbuffer, 1024, 0);
        // cout << "Hello" <<"\n";
        // inbuff  = string(inbuffer);
        // if (inbuff == "fail") {
        //     close(client_fd);
        //     exit(1);
        // }
        // cout  << inbuff << "\n";

        // //Send  quit command
        // strcpy(outbuffer, "quit\0");
        // send(client_fd, outbuffer, 1024, 0);
        // close(client_fd);
    }
}
