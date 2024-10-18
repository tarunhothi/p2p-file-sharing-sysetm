#include "client_header.h"

int list_groups(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char reply[3*SIZE];
    memset(reply, 0, sizeof(reply));
    read(sock, reply, 3*SIZE);
vector<std::string> grps = splitString(std::string(reply), "$$");

    size_t i = 0;
while (i < grps.size() - 1) {
    std::cout << grps[i] << endl;
    i++;
}

    return 0;
}

int list_requests(int sock){

    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);
    
    char reply[3*SIZE];
    memset(reply, 0, 3*SIZE);
    read(sock, reply, 3*SIZE);
    if(std::string(reply) == "**err**") {
        return -1;
    }
    
    if(std::string(reply) == "**er2**") {
return 1;
    }
    vector<std::string> requests = splitString(std::string(reply), "$$");

    size_t i = 0;
while (i < requests.size() - 1) {
    std::cout << requests[i] << endl;
    i++;
}
    return 0;
}

void accept_request(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char buf[96];
    read(sock, buf, 96);
    std::cout << buf << endl;
}

void leave_group(int sock){
    char buf[96];
    read(sock, buf, 96);
    std::cout << buf << endl;
}

void list_files(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char buf[1024];
    memset(buf, 0, 1024);
    read(sock, buf, 1024);
    vector<std::string> listOfFiles = splitString(std::string(buf), "$$");
    for(auto i: listOfFiles)
        std::cout << i << endl;
}

void show_downloads(){
    for(auto i: downloadedFiles){
        std::cout << "[C] " << i.second << " " << i.first << endl;
    }
}

int processCMD(vector<std::string> inpt, int sock){
    char server_reply[10240]; 
    memset(server_reply, 0, 10240);
    read(sock , server_reply, 10240); 
    std::cout << server_reply << endl;
 
    if(std::string(server_reply) == "Invalid argument count") {
return 0;
    }
    if(inpt[0] == "login"){
        if(std::string(server_reply) == "Login Successful"){
        loggedIn = true;
        std::string peerAddress = peer_ip + ":" + to_string(peer_port);
    write(sock, &peerAddress[0], peerAddress.length());
        }
    }
    else if(inpt[0] == "logout"){
loggedIn = false;
    }
    else if(inpt[0] == "upload_file"){
        if(std::string(server_reply) == "Error 101:"){
            std::cout << "Group doesn't exist" << endl;
            return 0;
        }
        else  if(std::string(server_reply) == "Error 102:"){
            std::cout << "You are not a member of this group" << endl;
            return 0;
        }
        else  if(std::string(server_reply) == "Error 103:"){
            std::cout << "File not found." << endl;
            return 0;
        }
        return uploadFile(inpt, sock);
    }
    else if(inpt[0] == "download_file"){
        if(std::string(server_reply) == "Error 101:"){
            std::cout << "Group doesn't exist" << endl;
            return 0;
        }
        else  if(std::string(server_reply) == "Error 102:"){
            std::cout << "You are not a member of this group" << endl;
            return 0;
        }
        else  if(std::string(server_reply) == "Error 103:"){
            std::cout << "Directory not found" << endl;
            return 0;
        }
        if(downloadedFiles.find(inpt[2])!= downloadedFiles.end()){
            std::cout << "File already downloaded" << endl;
            return 0;
        }
        return downloadFile(inpt, sock);
    }
    else if(inpt[0] == "list_groups"){
        return list_groups(sock);
    }
    else if(inpt[0] == "list_requests"){
        int t;
        if((t = list_requests(sock)) < 0){
            std::cout << "You are not the admin of this group\n";
        }
        else if(t>0){
            std::cout << "No pending requests\n";
        }
        else return 0;
    }
    else if(inpt[0] == "accept_request"){
        accept_request(sock);
    }
    else if(inpt[0] == "leave_group"){
        leave_group(sock);
    }
    else if(inpt[0] == "list_files"){
        list_files(sock);
    }
    else if(inpt[0] == "stop_share"){
        isUploaded[inpt[1]].erase(inpt[2]);
    }
    else if(inpt[0] == "show_downloads"){
        show_downloads();
    }
    return 0;
}