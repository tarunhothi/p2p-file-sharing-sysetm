#include "client_header.h"

/*************************************************************/
/*           Handles different requests from peer client     */
/*************************************************************/
void handleClientRequest(int client_socket){
    std::string client_uid = "";
    char inptline[1024] = {0}; 

    if(read(client_socket , inptline, 1024) <=0){
        close(client_socket);
        return;
    }

    vector<std::string> inpt = splitString(std::string(inptline), "$$");

    if(inpt[0] == "get_chunk_vector"){
        std::string filename = inpt[1];
        vector<int> chnkvec = fileChunkInfo[filename];
        std::string tmp = "";
        for(int i: chnkvec) tmp += to_string(i);
        char* reply = &tmp[0];
        write(client_socket, reply, strlen(reply));
    }
    else if(inpt[0] == "get_chunk"){
        //inpt = [get_chunk, filename, to_string(chunkNum), destination]
        std::string filepath = fileToFilePath[inpt[1]];
        ll chunkNum = stoll(inpt[2]);
        sendChunk(&filepath[0], chunkNum, client_socket);
        
    }
    else if(inpt[0] == "get_file_path"){
        std::string filepath = fileToFilePath[inpt[1]];
        write(client_socket, &filepath[0], strlen(filepath.c_str()));
    }
    close(client_socket);
    return;
}

/****************************************************************/
/*Connects to <serverPeerIP:serverPortIP> and sends it <command>*/
/****************************************************************/
std::string connectToPeer(char* serverPeerIP, char* serverPortIP, std::string command){
    int peersock = 0;
    struct sockaddr_in peer_serv_addr; 
    if ((peersock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
        std::cout << "\n Socket creation error \n";
        return "error"; 
    } 

    peer_serv_addr.sin_family = AF_INET; 
    uint16_t peerPort = stoi(std::string(serverPortIP));
    peer_serv_addr.sin_port = htons(peerPort); 

    if(inet_pton(AF_INET, serverPeerIP, &peer_serv_addr.sin_addr) < 0){ 
        perror("Peer Connection Error(INET)");
    } 
    if (connect(peersock, (struct sockaddr *)&peer_serv_addr, sizeof(peer_serv_addr)) < 0) { 
        perror("Peer Connection Error");
    } 
 
    std::string curcmd = splitString(command, "$$").front();
    if(curcmd == "get_chunk_vector"){
        if(send(peersock , &command[0] , strlen(&command[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        char server_reply[10240] = {0};
        if(read(peersock, server_reply, 10240) < 0){
            perror("err: ");
            return "error";
        }
        close(peersock);
        return std::string(server_reply);
    }
    else if(curcmd == "get_chunk"){
        //"get_chunk $$ filename $$ to_string(chunkNum) $$ destination
        if(send(peersock , &command[0] , strlen(&command[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        vector<std::string> cmdtokens = splitString(command, "$$");
        
        std::string despath = cmdtokens[3];
        ll chunkNum = stoll(cmdtokens[2]);
        writeChunk(peersock, chunkNum, &despath[0]);

        return "ss";
    }
    else if(curcmd == "get_file_path"){
        if(send(peersock , &command[0] , strlen(&command[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        char server_reply[10240] = {0};
        if(read(peersock, server_reply, 10240) < 0){
            perror("err: ");
            return "error";
        }
        fileToFilePath[splitString(command, "$$").back()] = std::string(server_reply);
    }

    close(peersock);
    return "aa";
}

/*****************************************************************************/
/* The peer acts as a server, continuously listening for connection requests */
/*****************************************************************************/
void* runAsServer(void* arg){
    int server_socket; 
    struct sockaddr_in address; 
    int addrlen = sizeof(address); 
    int opt = 1; 

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons(peer_port); 

    if(inet_pton(AF_INET, &peer_ip[0], &address.sin_addr)<=0)  { 
        std::cout << "Invalid address/ Address not supported \n";
        return NULL; 
    } 
       
    if (bind(server_socket, (SA *)&address,  sizeof(address))<0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(server_socket, 3) < 0) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    vector<thread> vThread;
    while(true){

        int client_socket;

        if((client_socket = accept(server_socket, (SA *)&address, (socklen_t *)&addrlen)) < 0){
            perror("Acceptance error"); 
        }
        vThread.push_back(thread(handleClientRequest, client_socket));
    }
    for(auto it=vThread.begin(); it!=vThread.end();it++){
        if(it->joinable()) it->join();
    }
    close(server_socket);
}