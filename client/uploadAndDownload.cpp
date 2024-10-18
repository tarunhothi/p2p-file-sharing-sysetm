#include "client_header.h"

typedef struct peerFileInfo{
    std::string serverPeerIP;
    std::string filename;
    ll filesize;
} peerFileInfo;

typedef struct readChunkDetails{
    std::string serverPeerIP;
    std::string filename;
    ll chunkNum; 
    std::string destination;
} readChunkDetails;


void sendChunk(char* filepath, int chunkNum, int client_socket) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("[-]Error in opening file.");
        exit(1);
    }

    off_t offset = chunkNum * FILE_SEGMENT_SZ;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("[-]Error in seeking file.");
        exit(1);
    }

    char buffer[FILE_SEGMENT_SZ] = {0};
    ssize_t bytesRead = read(fd, buffer, FILE_SEGMENT_SZ);
    
    if (bytesRead == -1) {
        perror("[-]Error in reading file.");
        exit(1);
    }

    if (send(client_socket, buffer, bytesRead, 0) == -1) {
        perror("[-]Error in sending file.");
        exit(1);
    }

    close(fd);
}

int writeChunk(int peersock, long long chunkNum, char* filepath) {
    int n;
    char buffer[FILE_SEGMENT_SZ];

    std::string content = "";
    long long tot = 0;

    int fd = open(filepath, O_RDWR);
    if (fd == -1) {
        perror("[-]Error in opening file.");
        return -1;
    }

    off_t offset = chunkNum * FILE_SEGMENT_SZ;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("[-]Error in seeking file.");
        close(fd);
        return -1;
    }

    while (tot < FILE_SEGMENT_SZ) {
        n = read(peersock, buffer, FILE_SEGMENT_SZ - 1);
        if (n <= 0) {
            break;
        }
        buffer[n] = 0;

        if (write(fd, buffer, n) == -1) {
            perror("[-]Error in writing file.");
            close(fd);
            return -1;
        }

        content += buffer;
        tot += n;
        memset(buffer, 0, FILE_SEGMENT_SZ);
    }

    close(fd);

    std::string hash = "";
    getStringHash(content, hash);
    hash.pop_back();
    hash.pop_back();

    if (hash != curFilePiecewiseHash[chunkNum]) {
        isCorruptedFile = true;
    }

    std::string filename = splitString(filepath, "/").back();
    setChunkVector(filename.c_str(), chunkNum, chunkNum, false);

    return 0;
}

void getChunkInfo(peerFileInfo* pf){
    
    vector<std::string> serverPeerAddress = splitString(std::string(pf->serverPeerIP), ":");
    std::string command = "get_chunk_vector$$" + std::string(pf->filename);
    std::string response = connectToPeer(&serverPeerAddress[0][0], &serverPeerAddress[1][0], command);

    size_t i = 0;
while (i < curDownFileChunks.size()) {
    if (response[i] == '1') {
        curDownFileChunks[i].push_back(pf->serverPeerIP);
    }
    i++;
}


    delete pf;
}

void getChunk(readChunkDetails* reqdChunk){

    std::string filename = reqdChunk->filename;
    vector<std::string> serverPeerIP = splitString(reqdChunk->serverPeerIP, ":");
    ll chunkNum = reqdChunk->chunkNum;
    std::string destination = reqdChunk->destination;

    std::string command = "get_chunk$$" + filename + "$$" + to_string(chunkNum) + "$$" + destination;
    connectToPeer(&serverPeerIP[0][0], &serverPeerIP[1][0], command);
    
    delete reqdChunk;
    return;
}
 
void piecewiseAlgo(vector<std::string> inpt, vector<std::string> peers){
    // inpt = [command, group id, filename, destination]
    ll filesize = stoll(peers.back());
    peers.pop_back();
    ll segments = filesize/FILE_SEGMENT_SZ+1;
    curDownFileChunks.clear();
    curDownFileChunks.resize(segments);

    
    vector<thread> threads, threads2;
 
    size_t i = 0;
while (i < peers.size()) {


peerFileInfo* pfi = new peerFileInfo();
    pfi->filename = inpt[2];
pfi->serverPeerIP = peers[i];
pfi->filesize = segments;
    threads.push_back(thread(getChunkInfo, pfi));
    i++;
}

    auto it = threads.begin();
while (it != threads.end()) {
    if (it->joinable()) {
        it->join();
        it = threads.erase(it);
    } else {
        ++it;
    }
}


    size_t j = 0;
while (j < curDownFileChunks.size()) {
    if (curDownFileChunks[i].size() == 0) {
        std::cout << "All parts of the file are not available." << endl;
        return;
    }
    j++;
}


    threads.clear();
    srand((unsigned) time(0));
    ll segmentsReceived = 0;

    std::string des_path = inpt[3] + "/" + inpt[2];
    // FILE* fp = fopen(&des_path[0], "r+");
	// if(fp != 0){
	// 	printf("The file already exists.\n") ;
    //     fclose(fp);
    //     return;
	// }
    // std::string ss(filesize, '\0');
    // fstream in(&des_path[0],ios::out|ios::binary);
    // in.write(ss.c_str(),strlen(ss.c_str()));  
    // in.close();

    int fd = open(des_path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
if (fd != -1) {
    printf("The file already exists.\n");
    close(fd);
    return;
}

std::string ss(filesize, '\0');
fd = open(des_path.c_str(), O_RDWR);
write(fd, ss.c_str(), ss.length());
close(fd);

    fileChunkInfo[inpt[2]].resize(segments,0);
    isCorruptedFile = false;

    vector<int> tmp(segments, 0);
    fileChunkInfo[inpt[2]] = tmp;
    
    std::string peerToGetFilepath;

    while(segmentsReceived < segments){
        
        ll randompiece;
        while(true){
            randompiece = rand()%segments;
            if(fileChunkInfo[inpt[2]][randompiece] == 0) break;
        }
        ll peersWithThisPiece = curDownFileChunks[randompiece].size();
        std::string randompeer = curDownFileChunks[randompiece][rand()%peersWithThisPiece];

        readChunkDetails* req = new readChunkDetails();
        req->filename = inpt[2];
        req->serverPeerIP = randompeer;
        req->chunkNum = randompiece;
        req->destination = inpt[3] + "/" + inpt[2];

        fileChunkInfo[inpt[2]][randompiece] = 1;

        threads2.push_back(thread(getChunk, req));
        segmentsReceived++;
        peerToGetFilepath = randompeer;
    }    
    for(auto it=threads2.begin(); it!=threads2.end();it++){
        if(it->joinable()) it->join();
    } 

    if(isCorruptedFile){
        std::cout << "Download completed. The file may be corrupted." << endl;
    }
    else{
         std::cout << "Download completed. No corruption detected." << endl;
    }
    downloadedFiles.insert({inpt[2], inpt[1]});

    vector<std::string> serverAddress = splitString(peerToGetFilepath, ":");
    connectToPeer(&serverAddress[0][0], &serverAddress[1][0], "get_file_path$$" + inpt[2]);
    return;
}
 
int downloadFile(vector<std::string> inpt, int sock){
    // inpt -  download_file​ <group_id> <file_name> <destination_path>
    if(inpt.size() != 4){
        return 0;
    }
    std::string fileDetails = "";
    fileDetails += inpt[2] + "$$"; 
    fileDetails += inpt[3] + "$$";
    fileDetails += inpt[1];
    // fileDetails = [filename, destination, group id]
    if(send(sock , &fileDetails[0] , strlen(&fileDetails[0]) , MSG_NOSIGNAL ) == -1){
        printf("Error: %s\n",strerror(errno));
        return -1;
    }

    char server_reply[524288] = {0}; 
    read(sock , server_reply, 524288); 

    if(std::string(server_reply) == "File not found"){
        std::cout << server_reply << endl;
        return 0;
    }
    vector<std::string> peersWithFile = splitString(server_reply, "$$");
    
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    memset(server_reply, 0, 524288);
    read(sock , server_reply, 524288); 

    vector<std::string> tmp = splitString(std::string(server_reply), "$$");
    curFilePiecewiseHash = tmp;

    piecewiseAlgo(inpt, peersWithFile);
    return 0;
}

int uploadFile(vector<std::string> inpt, int sock){
    if(inpt.size() != 3){
            return 0;
    }
    std::string fileDetails = "";
    char* filepath = &inpt[1][0];

    std::string filename = splitString(std::string(filepath), "/").back();

    if(isUploaded[inpt[2]].find(filename) != isUploaded[inpt[2]].end()){
        std::cout << "File already uploaded" << endl;
        if(send(sock , "error" , 5 , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return -1;
        }
        return 0;
    }
    else{
        isUploaded[inpt[2]][filename] = true;
        fileToFilePath[filename] = std::string(filepath);
    }

    std::string piecewiseHash = getHash(filepath);

    if(piecewiseHash == "$") return 0;
    std::string filehash = getFileHash(filepath);
    std::string filesize = to_string(file_size(filepath));

    fileDetails += std::string(filepath) + "$$";
    fileDetails += std::string(peer_ip) + ":" + to_string(peer_port) + "$$";
    fileDetails += filesize + "$$";
    fileDetails += filehash + "$$";
    fileDetails += piecewiseHash;

    if(send(sock , &fileDetails[0] , strlen(&fileDetails[0]) , MSG_NOSIGNAL ) == -1){
        printf("Error: %s\n",strerror(errno));
        return -1;
    }
    
    char server_reply[10240] = {0}; 
    read(sock , server_reply, 10240); 
    std::cout << server_reply << endl;
    setChunkVector(filename, 0, stoll(filesize)/FILE_SEGMENT_SZ + 1, true);

    return 0;
}