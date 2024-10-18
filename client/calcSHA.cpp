#include "client_header.h"

long long file_size(char *path) {
    int fd = open(path, O_RDONLY);
    
    if (fd == -1) {
        perror("File not found");
        return -1;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    close(fd);

    if (size == -1) {
        perror("Error obtaining file size");
        return -1;
    }

    return (long long)size;
}

void getStringHash(string segmentString, string& hash){
    unsigned char md[20];
    if(!SHA1(reinterpret_cast<const unsigned char *>(&segmentString[0]), segmentString.length(), md)){
        std::cout << "Error in hashing\n";
    }
    else{
        for(int i=0; i<20; i++){
            char buf[3];
            sprintf(buf, "%02x", md[i]&0xff);
            hash += string(buf);
        }
    }
    hash += "$$";
}

/*************************************************************/
/*        Returns combined PIECEWISE hash of the file        */
/*************************************************************/
std::string getHash(char* path) {
    int i, accum;
    int fd;
    long long fileSize = file_size(path);

    if (fileSize == -1) {
        return "$";
    }

    int segments = fileSize / FILE_SEGMENT_SZ + 1;
    char line[SIZE + 1];
    std::string hash = "";

    fd = open(path, O_RDONLY);

    if (fd != -1) {
        for (i = 0; i < segments; i++) {
            accum = 0;
            std::string segmentString;

            int rc;
            while (accum < FILE_SEGMENT_SZ && (rc = read(fd, line, std::min(SIZE - 1, FILE_SEGMENT_SZ - accum)))) {
                line[rc] = '\0';
                accum += rc;
                segmentString += line;
                memset(line, 0, sizeof(line));
            }

            getStringHash(segmentString, hash);
        }

        close(fd);
    } else {
        perror("File not found");
    }

    hash.pop_back();
    hash.pop_back();
    return hash;
}

std::string getFileHash(char* path) {
    std::string hash;
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        perror("Error: File not found");
        return hash;
    }

    const int bufferSize = 1024;
    char buffer[bufferSize];
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256_CTX context;

    SHA256_Init(&context);

    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, bufferSize)) > 0) {
        SHA256_Update(&context, buffer, bytesRead);
    }

    SHA256_Final(md, &context);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        char buf[3];
        sprintf(buf, "%02x", md[i] & 0xff);
        hash += std::string(buf);
    }

    close(fd);
    return hash;
}