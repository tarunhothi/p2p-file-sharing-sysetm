#include "tracker_header.h"

int createUser(vector<std::string> inpt){
    std::string user_id = inpt[1];
    std::string passwd = inpt[2];

    if(loginCreds.find(user_id) == loginCreds.end()){
            loginCreds.insert({user_id, passwd});
    }
    else{
        return -1;
    }
    return 0;
}

int validateLogin(vector<std::string> inpt){
    std::string user_id = inpt[1];
    std::string passwd = inpt[2];

    if(loginCreds.find(user_id) == loginCreds.end() || loginCreds[user_id] != passwd){
        return -1;
    }

    if(isLoggedIn.find(user_id) == isLoggedIn.end()){
        isLoggedIn.insert({user_id, true});
    }
    else{
            if(isLoggedIn[user_id]){
            return 1;
    }
            else{
            isLoggedIn[user_id] = true;
        }
    }
    return 0;
}

void uploadFile(vector<std::string> inpt, int client_socket, std::string client_uid){
    //inpt - upload_file​ <file_path> <group_id​>
    if(inpt.size() != 3){
        write(client_socket, "Invalid argument count", 22);
    }
    else if(groupMembers.find(inpt[2]) == groupMembers.end()){
        write(client_socket, "Error 101:", 10);
    }
    else if(groupMembers[inpt[2]].find(client_uid) == groupMembers[inpt[2]].end()){
        write(client_socket, "Error 102:", 10);
    }
    else if(!pathExists(inpt[1])){
        write(client_socket, "Error 103:", 10);
    }
    else{
        char fileDetails[524288] =  {0};
        write(client_socket, "Uploading...", 12);
        if(read(client_socket , fileDetails, 524288)){
            if(std::string(fileDetails) == "error") return;

            vector<std::string> fdet = splitString(std::string(fileDetails), "$$");
            //fdet = [filepath, peer address, file size, file hash, piecewise hash] 
            std::string filename = splitString(std::string(fdet[0]), "/").back();

            std::string hashOfPieces = "";
            size_t i = 4;
    while (i < fdet.size()) {
        hashOfPieces += fdet[i];
        if (i != fdet.size() - 1) {
            hashOfPieces += "$$";
        }
        i++;
    }

            
            piecewiseHash[filename] = hashOfPieces;
            
            if(seederList[inpt[2]].find(filename) != seederList[inpt[2]].end()){
                seederList[inpt[2]][filename].insert(client_uid);
            }
            else{
                seederList[inpt[2]].insert({filename, {client_uid}});
            }
            fileSize[filename] = fdet[2];
            
            write(client_socket, "Uploaded", 8);
        }
    }
}

void downloadFile(vector<std::string> inpt, int client_socket, std::string client_uid){
    // inpt - download_file​ <group_id> <file_name> <destination_path>
    if(inpt.size() != 4){
        write(client_socket, "Invalid argument count", 22);
    }
    else if(groupMembers.find(inpt[1]) == groupMembers.end()){
        write(client_socket, "Error 101:", 10);
    }
    else if(groupMembers[inpt[1]].find(client_uid) == groupMembers[inpt[1]].end()){
        write(client_socket, "Error 102:", 10);
    }
    else{
        if(!pathExists(inpt[3])){
            write(client_socket, "Error 103:", 10);
            return;
        }

        char fileDetails[524288] =  {0};
        // fileDetails = [filename, destination, group id]
        write(client_socket, "Downloading...", 13);

        if(read(client_socket , fileDetails, 524288)){
            vector<std::string> fdet = splitString(std::string(fileDetails), "$$");
            
            std::string reply = "";
            if(seederList[inpt[1]].find(fdet[0]) != seederList[inpt[1]].end()){
                for(auto i: seederList[inpt[1]][fdet[0]]){
                        if(isLoggedIn[i]){
                        reply += unameToPort[i] + "$$";
                        }
                }
                reply += fileSize[fdet[0]];
                write(client_socket, &reply[0], reply.length());

                char dum[5];
                read(client_socket, dum, 5);
                
                write(client_socket, &piecewiseHash[fdet[0]][0], piecewiseHash[fdet[0]].length());
            
                seederList[inpt[1]][inpt[2]].insert(client_uid);
            }
            else{
                write(client_socket, "File not found", 14);
            }
        }
        
    }
}

int create_group(vector<std::string> inpt, int client_socket, std::string client_uid){
    //inpt - [create_group gid] 
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return -1;
    }
    for(auto i: allGroups){
        if(i == inpt[1]) return -1;
    }
    grpAdmins.insert({inpt[1], client_uid});
    allGroups.push_back(inpt[1]);
    groupMembers[inpt[1]].insert(client_uid);
    return 0;
}

void list_groups(vector<std::string> inpt, int client_socket){
    //inpt - [list_groups];
    if(inpt.size() != 1){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "All groups:", 11);

    char dum[5];
    read(client_socket, dum, 5);

    if(allGroups.size() == 0){
        write(client_socket, "No groups found$$", 18);
        return;
    }

    std::string reply = "";
    size_t i = 0;
while (i < allGroups.size()) {
    reply += allGroups[i] + "$$";
    i++;
}

    write(client_socket, &reply[0], reply.length());
}

void join_group(vector<std::string> inpt, int client_socket, std::string client_uid){
    //inpt - [join_group gid]
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(groupMembers[inpt[1]].find(client_uid) == groupMembers[inpt[1]].end()){
        grpPendngRequests[inpt[1]].insert(client_uid);
        write(client_socket, "Group request sent", 18);
    }
    else{
        write(client_socket, "You are already in this group", 30);
    }
    
}

void list_requests(vector<std::string> inpt, int client_socket, std::string client_uid){
    // inpt - [list_requests groupid]
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Fetching group requests...", 27);

    char dum[5];
    read(client_socket, dum, 5);
    if(grpAdmins.find(inpt[1])==grpAdmins.end() || grpAdmins[inpt[1]] != client_uid){
        write(client_socket, "**err**", 7);
    }
    else if(grpPendngRequests[inpt[1]].size() == 0){
        write(client_socket, "**er2**", 7);
    }
    else {
        std::string reply = "";
        for(auto i = grpPendngRequests[inpt[1]].begin(); i!= grpPendngRequests[inpt[1]].end(); i++){
            reply += std::string(*i) + "$$";
        }
        write(client_socket, &reply[0], reply.length());
    }
}

void accept_request(vector<std::string> inpt, int client_socket, std::string client_uid){
    // inpt - [accept_request groupid user_id]
    if(inpt.size() != 3){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Accepting request...", 21);

    char dum[5];
    read(client_socket, dum, 5);

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(grpAdmins.find(inpt[1])->second == client_uid){
        grpPendngRequests[inpt[1]].erase(inpt[2]);
        groupMembers[inpt[1]].insert(inpt[2]);
        write(client_socket, "Request accepted.", 18);
    }
    else{
        //cout << grpAdmins.find(inpt[1])->second << " " << client_uid <<  endl;
        write(client_socket, "You are not the admin of this group", 35);
    }
    
}

void leave_group(vector<std::string> inpt, int client_socket, std::string client_uid){
    // inpt - [leave_group groupid]
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Leaving group...", 17);

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(groupMembers[inpt[1]].find(client_uid) != groupMembers[inpt[1]].end()){
        if(grpAdmins[inpt[1]] == client_uid){
            write(client_socket, "You are the admin of this group, you cant leave!", 48);
        }
        else{
            groupMembers[inpt[1]].erase(client_uid);
            write(client_socket, "Group left succesfully", 23);
        }
    }
    else{
        write(client_socket, "You are not in this group", 25);
    }
}

void list_files(vector<std::string> inpt, int client_socket){
    // inpt - list_files​ <group_id>
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Fetching files...", 17);

    char dum[5];
    read(client_socket, dum, 5);

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(seederList[inpt[1]].size() == 0){
        write(client_socket, "No files found.", 15);
    }
    else{

        std::string reply = "";

        for(auto i: seederList[inpt[1]]){
            reply += i.first + "$$";
        }
        reply = reply.substr(0, reply.length()-2);

        write(client_socket, &reply[0], reply.length());
    }
}

void stop_share(vector<std::string> inpt, int client_socket, std::string client_uid){
    // inpt - stop_share ​<group_id> <file_name>
    if(inpt.size() != 3){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(seederList[inpt[1]].find(inpt[2]) == seederList[inpt[1]].end()){
        write(client_socket, "File not yet shared in the group", 32);
    }
    else{
        seederList[inpt[1]][inpt[2]].erase(client_uid);
        if(seederList[inpt[1]][inpt[2]].size() == 0){
            seederList[inpt[1]].erase(inpt[2]);
        }
        write(client_socket, "Stopped sharing the file", 25);
    }
}

//client connection handling thread
void handle_connection(int client_socket){
    std::string client_uid = "";
    std::string client_gid = "";

    //for continuously checking the commands sent by the client
    while(true){
        char inptline[1024] = {0}; 

        if(read(client_socket , inptline, 1024) <=0){
            isLoggedIn[client_uid] = false;
            close(client_socket);
            break;
        }

        std::string s, in = std::string(inptline);
        stringstream ss(in);
        vector<std::string> inpt;

        while(ss >> s){
            inpt.push_back(s);
        }

        if(inpt[0] == "create_user"){
            if(inpt.size() != 3){
                write(client_socket, "Invalid argument count", 22);
            }
            else{
                if(createUser(inpt) < 0){
                    write(client_socket, "User exists", 11);
                }
                else{
                    write(client_socket, "Account created", 15);
                }
            }
        }
        else if(inpt[0] == "login"){
            if(inpt.size() != 3){
                write(client_socket, "Invalid argument count", 22);
            }
            else{
                int r;
                if((r = validateLogin(inpt)) < 0){
                    write(client_socket, "Username/password incorrect", 28);
                }
                else if(r > 0){
                    write(client_socket, "You already have one active session", 35);
                }
                else{
                    write(client_socket, "Login Successful", 16);
                    client_uid = inpt[1];
                    char buf[96];
                    read(client_socket, buf, 96);
                    std::string peerAddress = std::string(buf);
                    unameToPort[client_uid] = peerAddress;
                }
            }            
        }
        else if(inpt[0] ==  "logout"){
            isLoggedIn[client_uid] = false;
            write(client_socket, "Logout Successful", 17);
        }
        else if(inpt[0] == "upload_file"){
            uploadFile(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "download_file"){
            downloadFile(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "create_group"){
            if(create_group(inpt, client_socket, client_uid) >=0){
                client_gid = inpt[1];
                write(client_socket, "Group created", 13);
            }
            else{
                write(client_socket, "Group exists", 12);
            }
        }
        else if(inpt[0] == "list_groups"){
            list_groups(inpt, client_socket);
        }
        else if(inpt[0] == "join_group"){
            join_group(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "list_requests"){
            list_requests(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "accept_request"){
            accept_request(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "leave_group"){
            leave_group(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "list_files"){
            list_files(inpt, client_socket);
        }
        else if(inpt[0] == "stop_share"){
            stop_share(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "show_downloads"){
            write(client_socket, "Loading...", 10);
        }
        else{
            write(client_socket, "Invalid command", 16);
        }
    }
    close(client_socket);
}