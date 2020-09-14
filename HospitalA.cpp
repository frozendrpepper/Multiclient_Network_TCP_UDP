#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <netinet/in.h>
#include <random>
#include <vector>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include <fstream>
#include <sstream>

using namespace std;

// This function converts the string input into a 2D vector
vector<vector<string>> processInput(string inputFile) {
    ifstream infile(inputFile);
    string line;
    string delimDep = "#"; // Delim used to separate department from severity range
    string delimRan = ","; // delim used to separate the severity min and max
    vector<vector<string>> finalVector;

    while (getline(infile, line)) {
        istringstream iss(line);
        vector<string> returnVector;

        // First split the line via delimDep
        size_t pos = 0;
        string token;

        while ((pos = line.find(delimDep)) != string::npos) {
            token = line.substr(0, pos);
            returnVector.push_back(token);
            line.erase(0, pos + delimDep.length());
        }

        // now split the range and insert into a vector
        pos = 0;
        while ((pos = line.find(delimRan)) != string::npos) {
            token = line.substr(0, pos);
            returnVector.push_back(token);
            line.erase(0, pos + delimRan.length());
        }
        returnVector.push_back(line);

        // push into the 2d vector
        finalVector.push_back(returnVector);
    }
    // The output is of format ex) [[A1, 3, 5], [A2, 4, 10], [A3, 7, 9]]
    return finalVector;
}

int main()
{
    string hospital = "Hospital A"; // Indicates which hospital current client is
    const int USCID = 824; // represents last 3 digits of USC ID
    int tempPort = 6000 + USCID; // as per the project phase 1 requirement
    const char * SERVERPORT = to_string(tempPort).c_str(); // This is the port the server will use to listen
    const int BACKLOG = 10;
    const char * domain = "localhost"; // It seems that everything will be run locally?

    addrinfo hints, *res, *p;
    int sockfd, valread;
    char ip4[INET_ADDRSTRLEN];

    // Set up the connection
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPV4 connection
    hints.ai_socktype = SOCK_STREAM; // TCP connection

    int status = getaddrinfo(domain, SERVERPORT, &hints, &res);

    // create a client socket
    sockfd = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol);

    // Process the input file in a way when the connection is made the information can be sent
    vector<vector<string>> returnVector = processInput("HospitalA.txt");

    // connect to the server
    connect(sockfd, res -> ai_addr, res -> ai_addrlen);

    // Print the IP of the client
    inet_ntop(AF_INET, &(((sockaddr_in * ) res-> ai_addr) -> sin_addr), ip4, INET_ADDRSTRLEN);
    // get the port number
    sockaddr_in sin;
    uint32_t portNum;
    socklen_t len = sizeof sin;
    if (getsockname(sockfd, (sockaddr * ) & sin, &len) == -1) {
        cerr << "getsockname error" << endl;
    } else {
        portNum = ntohs(sin.sin_port);
    }
    cout << hospital << " has TCP port " << portNum << " and IP address " << ip4 << " for Phase 1" << endl;
    cout << hospital << " is now connected to the health center" << endl;

    char buffer[1024]; // Buffer used to receive UDP msg from the server
    int byteSent;
    int phase = 1;

    // We need the while loop so we can handle the response for phase two
    string separator = "/";
    string depSep = "=";

    while (true) {
        if (phase == 1) {
            for (int i = 0; i < returnVector.size(); i++) {
                string resultString;
                for (int j = 0; j < returnVector[i].size() - 1; j++) {
                    resultString += returnVector[i][j] + separator;
                }
                resultString += returnVector[i][returnVector[i].size() - 1] + depSep; // add the range_max information

                const char * resultC = resultString.c_str(); // convert to char array so it can be sent

                send(sockfd, resultC, strlen(resultC), 0); // send to the server

                cout << hospital << " has sent " << returnVector[i][0] << " to the health center" << endl;
                usleep(50000); // used this to give some time between each information send or the server behaved erratically
            }

            string finalString = "exit";
            const char * finalC = finalString.c_str();
            send(sockfd, finalC, strlen(finalC), 0); // send to the server

            cout << "Updating the health center is done for " << hospital << endl;
            cout << "End of Phase 1 for " << hospital << endl;
        }
        break; // This is for phase 1, for phase 2, it should be appropriately modified
    }
    close(sockfd); // close socket

    /* ################### UDP related Variables ################### */
    int tempHosA = 21100 + USCID;
    const char * HOSAPORT = to_string(tempHosA).c_str();

    sockaddr_storage their_addr_udp; // pointer to a local sockaddr_storage where the information of incoming connection is stored
    socklen_t addr_len_udp;
    addrinfo hints_udp, *res_udp;
    socklen_t addr_size_udp;
    int sockfd_udp, valread_udp;

    // Set up the connection
    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET; // IPV4 connection
    hints_udp.ai_socktype = SOCK_DGRAM; // TCP connection
    getaddrinfo("localhost", HOSAPORT, &hints_udp, &res_udp);

    // Using the information from getaddrinfo, set up the parent socket
    sockfd_udp = socket(res_udp -> ai_family, res_udp -> ai_socktype, res_udp -> ai_protocol);

    // Bind the socket to a port
    if (bind(sockfd_udp, res_udp -> ai_addr, res_udp -> ai_addrlen) < 0) {
        cerr << "Socket bind failure" << endl;
        return -2;
    }

    /* ################ Set up UDP connection to receive from Health Center ###################### */
    int printIP = 0;
    while (true) {
        memset(buffer, 0, sizeof buffer);
        addr_len_udp = sizeof their_addr_udp; // client address length
        recvfrom(sockfd_udp, buffer, 1024, 0, (sockaddr *) &their_addr_udp, &addr_len_udp); // recvfrom for udp

        if (printIP == 0) {
            // Print the UDP port and IP address
            char ip4_udp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(((sockaddr_in * ) res_udp-> ai_addr) -> sin_addr), ip4_udp, INET_ADDRSTRLEN);
            // get the port number
            sockaddr_in sin_udp;
            uint32_t portNum_udp;
            socklen_t len_udp = sizeof sin_udp;
            if (getsockname(sockfd_udp, (sockaddr * ) &sin_udp, &len_udp) == -1) {
                cerr << "getsockname error" << endl;
            } else {
                portNum_udp = ntohs(sin_udp.sin_port);
            }
            cout << "Hospital A has UDP port " << portNum_udp << " and IP address " << ip4_udp << " for Phase 2" << endl;
            printIP++;
        }

        string finalResult = string(buffer);
        if (finalResult.find("Student1") != string::npos) {
            cout << "Student1 has been admitted to " << hospital << endl;
        } else if (finalResult.find("Student2") != string::npos) {
            cout << "Student2 has been admitted to " << hospital << endl;
        } else if (finalResult.find("Student3") != string::npos) {
            cout << "Student3 has been admitted to " << hospital << endl;
        } else if (finalResult.find("Student4") != string::npos) {
            cout << "Student4 has been admitted to " << hospital << endl;
        } else if (finalResult.find("Student5") != string::npos) {
            cout << "Student5 has been admitted to " << hospital << endl;
        }

        if (finalResult == "done") {
            break;
        }
    }

    close(sockfd_udp);
    return 0;
}
