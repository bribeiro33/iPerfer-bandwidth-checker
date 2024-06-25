#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <fstream>
#include <chrono>
#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"
#include "DNSResolve.h"

using namespace std;

#define PACKET_LEN 1000
#define MAX_CONNECTIONS 1024 // Adjust based on expected load

// Finds the header length
int findHeaderLength(const string& s) {
    size_t found = s.find("\r\n\r\n");
    return found != string::npos ? static_cast<int>(found + 4) : -1;
}

// Extracts the content length from the HTTP header
int findContentLength(const string& s) {
    size_t start = s.find("Content-Length: ");
    if (start == string::npos) return -1;
    start += 16; // Move to the end of "Content-Length: "
    size_t end = s.find("\r\n", start);
    if (end == string::npos) return -1;
    string sub = s.substr(start, end - start);
    return atoi(sub.c_str());
}

// Modifies the request URI for f4m files
string modifyF4mRequest(const string& s) {
    size_t start = s.find(".f4m");
    if (start == string::npos) return s;
    string modified = s;
    modified.insert(start, "_nolist");
    return modified;
}

// Modifies the bitrate in the request URI
string modifyBitrate(const string& s, int bitrate) {
    size_t segPos = s.find("Seg");
    if (segPos == string::npos) return s;
    size_t slashPos = s.rfind('/', segPos);
    if (slashPos == string::npos) return s;
    size_t nextSlashPos = s.find('/', slashPos + 1);
    if (nextSlashPos == string::npos) return s;
    string modified = s.substr(0, slashPos + 1) + to_string(bitrate) + s.substr(nextSlashPos);
    return modified;
}

// Extracts segment and fragment numbers from the request URI
pair<int, int> findSegAndFrag(const string& s) {
    size_t segPos = s.find("Seg");
    size_t fragPos = s.find("Frag");
    if (segPos == string::npos || fragPos == string::npos) return {-1, -1};
    
    string segSub = s.substr(segPos + 3);
    string fragSub = s.substr(fragPos + 4);
    
    size_t segEnd = segSub.find('-');
    size_t fragEnd = fragSub.find(' ');
    
    int seg = atoi(segSub.substr(0, segEnd).c_str());
    int frag = atoi(fragSub.substr(0, fragEnd).c_str());
    
    return {seg, frag};
}

// Parse the f4m file to get available bitrates
vector<int> parseF4mFile(const string& filePath) {
    vector<int> bitrates;
    ifstream file(filePath);
    string line;
    while (getline(file, line)) {
        if (line.find("bitrate=") != string::npos) {
            size_t start = line.find("bitrate=") + 9;
            size_t end = line.find('"', start);
            int bitrate = atoi(line.substr(start, end - start).c_str());
            bitrates.push_back(bitrate);
        }
    }
    sort(bitrates.begin(), bitrates.end());
    return bitrates;
}
