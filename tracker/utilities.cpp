#include "tracker_header.h"

bool pathExists(const std::string &s){
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}

vector<std::string> splitString(std::string str, std::string delim){
    vector<std::string> res;

    size_t pos = 0;
    while ((pos = str.find(delim)) != std::string::npos) {
        std::string t = str.substr(0, pos);
        res.push_back(t);
        str.erase(0, pos + delim.length());
    }
    res.push_back(str);

    return res;
}

/******************************************************/
/* Thread function which detects if quit was typed in */
/******************************************************/
void* check_input(void* arg){
    while(true){
        std::string inputline;
        getline(cin, inputline);
        if(inputline == "quit"){
            exit(0);
        }
    }
}

vector<std::string> getTrackerInfo(char* path){
    fstream trackerInfoFile;
    trackerInfoFile.open(path, ios::in);

    vector<std::string> res;
    if(trackerInfoFile.is_open()){
        std::string t;
        while(getline(trackerInfoFile, t)){
            res.push_back(t);
        }
        trackerInfoFile.close();
    }
    else{
        std::cout << "Tracker Info file not found.\n";
        exit(-1);
    }
    return res;
}

void processArgs(int argc, char *argv[]){
    logFileName = "trackerlog" + std::string(argv[2]) + ".txt";
    vector<std::string> trackeraddress = getTrackerInfo(argv[1]);
    if(std::string(argv[2]) == "1"){
        tracker1_ip = trackeraddress[0];
        tracker1_port = stoi(trackeraddress[1]);
        curTrackerIP = tracker1_ip;
        curTrackerPort = tracker1_port;
    }
    else{
        tracker2_ip = trackeraddress[2];
        tracker2_port = stoi(trackeraddress[3]);
        curTrackerIP = tracker2_ip;
        curTrackerPort = tracker2_port;
    }

}