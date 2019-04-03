#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include<arpa/inet.h> //inet_ntop
#include <string.h>
#include <iostream>
#include <fstream>
#include<sys/un.h>
#include <dirent.h>
using namespace std;


//End the iostream strings with "\n" always.

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << "Usage : ./main <portNum> <passwdfile> <userDatabase>";
    exit(1);
  }

  else {
    int MYPORT = stoi(argv[1]);
    int sock_fd, new_fd; //one for bind and one for accept
    sock_fd = socket(PF_INET, SOCK_STREAM, 0); // Getting the value of the Socket file Descriptor
    //cout << sock_fd << "\n";

    //Initializing Buffers
    char inbuffer[1024] = {0};
    char outbuffer[1024] = {0};

    //Populating the sockaddr_in structure
    struct sockaddr_in addr, peer_addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(MYPORT);
    memset(&(addr.sin_zero), '\0', 8);

    // Binding the server to the Socket File Descriptor
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      cerr << "Bind on port " << MYPORT << " failed\n";
      exit(2);
    }

    else cout << "BindDone: " << MYPORT << "\n";
    // else cout <<"Hello";

    //Instantiating Password File
    string file = argv[2];
    ifstream pwdfile(file);
    if (!pwdfile.good())
    {
      cerr << "Password File not present or not readable" << "\n";
      exit(3);
    }

    /*
      Different from Phase 1
    */

    //Checking whether user database exists
    string db = argv[3];
    const char *dbase = db.c_str();
    // struct stat sb;
    // if (stat(dbase, &sb) != 0 || !(S_ISDIR(sb.st_mode)))
    // {
    //     cerr << "Database folder not present";
    //     exit(4);
    // }
    struct dirent *dp;
    DIR *fd;

    if ((fd = opendir(dbase)) == NULL) {
      cerr << "Database folder not present";
      exit(4);
    }

    //Listening on Socket
    if (listen(sock_fd, 3) < 0) {
      cerr << "Listen on Socket Failed" << "\n";
      exit(4);
    }

    else cout << "ListenDone: " << MYPORT << "\n";

    int addrlen = sizeof(struct sockaddr_in);
    // char* ip;

    while (1) {
      //Accepting incoming connection on Socket

      new_fd = accept(sock_fd, (struct sockaddr *)&peer_addr, (socklen_t*)&addrlen);
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(peer_addr.sin_addr), ip, INET_ADDRSTRLEN);
      cout << "Client: " << ip << ":" <<  ntohs(peer_addr.sin_port) << "\n";
      // cout << "Hello" <<"\n";
      recv(new_fd , inbuffer, 1024, 0);
      // cout << "Hello" <<"\n";
      string inbuff(inbuffer);
      //cout  << inbuff << "\n";

      //Instantiating credential strings
      string username, passwd, realpwd;

      //Parsing the String
      try {
        int passpos = inbuff.find("Pass:");
        username = inbuff.substr(6, passpos - 7);
        passwd = inbuff.substr(passpos + 6);
        //cout << "User" << username << " " << "Passwd" << passwd <<"\n";
        // cout << passpos <<"\n";
      }


      catch (...) {
        cout << "Unknown Command\n"; //Need to send fail command too
        close(new_fd);
      }

      // Checking the user in the file
      int found = 0;

      string line;
      while (getline(pwdfile, line)) {
        int spacepos = line.find(" ");
        //cout << line << "\n";
        if (username == line.substr(0, spacepos)) {
          realpwd = line.substr(spacepos + 1);
          found++;
          break;
        }
      }

      //Cursor of the password file back on top
      pwdfile.clear();
      pwdfile.seekg(0, ios::beg);

      //checking if the user is present.
      if (found == 0) {
        cout << "Invalid User\n";
        strcpy(outbuffer, "fail");   //Sending fail signal to client
        send(new_fd, outbuffer, 1024, 0);
        close(new_fd);
        // close(sock_fd);
      }

      //Checking if the password matches
      else if (realpwd != passwd) {
        cout << "Wrong Password\n";
        strcpy(outbuffer, "fail");
        send(new_fd, outbuffer, 1024, 0);
        close(new_fd);
        // close(sock_fd);
      }

      //Successful Login
      else {
        string welcome_msg = "Welcome " + username + "\0";
        const char *wel_msg = welcome_msg.c_str();
        strcpy(outbuffer, wel_msg);
        send(new_fd, outbuffer, 1024, 0);

        cout << welcome_msg << "\n";

        recv(new_fd , inbuffer, 1024, 0);
        inbuff = string(inbuffer);

        if (inbuff == "LIST\0") {
          string userdb = db + "/" + username;
          const char* userdbase = userdb.c_str();
          struct dirent *userdp;
          DIR *userfd;

          if ((userfd = opendir(userdbase)) == NULL) {
            cout << username << ": Folder Read Fail\n";
            strcpy(outbuffer, "fail");
            send(new_fd, outbuffer, 1024, 0);
            close(new_fd);
          }

          else {
            int count = 0;
            while ((userdp = readdir(userfd)) != NULL) {
              if (!strcmp(userdp->d_name, ".") || !strcmp(userdp->d_name, ".."))
                continue;    /* skip self and parent */
              // printf("%s/%s\n", dir, dp->d_name);
              count++;
            }
            closedir(userfd);
            string listMessage = username + ": No of messages " + to_string(count) + "\0";
            const char *list_msg = listMessage.c_str();
            strcpy(outbuffer, list_msg);
            send(new_fd, outbuffer, 1024, 0);
            cout << listMessage << "\n";

            recv(new_fd , inbuffer, 1024, 0);
            inbuff = string(inbuffer);
            if (inbuff == "quit\0") {
              cout << "Bye " << username <<"\n";
              close(new_fd);
              // exit(1);
            }
            else {
              cout << "Unknown command\n";
              close(new_fd);
            }

          }



        }

        else {
          cout << "Unknown Commannd \n";
          strcpy(outbuffer, "fail");
          send(new_fd, outbuffer, 1024, 0);
          close(new_fd);
        }




      }




    }




//TODO  Add buffer for the communication between client and

  }
}
