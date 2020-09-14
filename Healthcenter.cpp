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

#include <sys/mman.h>

using namespace std;

static char * hospital_compile; // global variable used to accumulate hospital information
static int * num_hospital; // used to check how many hospitals have sent their information
static int * hospital_inputDone; // Indicates whether hospital input receive is done. This is to make sure to separate Student logic
static char * student_compile; // global variable that compiles each student input
static char * student_compile_perm; // global var used to accumulate all students
static int * num_student; // used to check how many students have sent their information
static int * TCP_done; // used to indicate that TCP input handle is done
static int * ind; // value used to control print out of port and IP address for Phase 2

/* Takes string containing all the hospital information and convert to a hash map */
map<string, vector<int>> processHospital(char * hospital_compile) {
    // The inpu8t hospital_compile is in format A1/1/2=A2/3/5= .....C2/5/8=C3/7/10=
    map<string, vector<int>> returnMap;
    string inputString = string(hospital_compile);
    string delimDep = "="; // delimiter used to separate out each department
    string delimDetail = "/"; // delimiter used to separate information within each department

    size_t pos = 0;
    string token;

    while ((pos = inputString.find(delimDep)) != string::npos) {
        token = inputString.substr(0, pos);
        // Now we've separated each department, we also need to separate by each detail
        size_t innerPos = 0;
        string innerToken;
        string curKey;
        vector<int> rangeVector;
        int counter = 0;
        while ((innerPos = token.find(delimDetail)) != string::npos) {
            innerToken = token.substr(0, innerPos);
            if (counter == 0) {
                curKey = innerToken; // Extract key ex) A1/3/5 -> A1
                counter++;
            } else {
                rangeVector.push_back(stoi(innerToken)); // Extract min severity ex) A1/3/5 -> 3
            }
            token.erase(0, innerPos + delimDetail.length());
        }
        rangeVector.push_back(stoi(token)); // Extract max severity ex) A1/3/5 -> 5

        // rangeVector.push_back(1); // This is an indicator which shows if this department is available or not (binary 1 or 0)(used for phase 2)

        inputString.erase(0, pos + delimDep.length());
        returnMap[curKey] = rangeVector; // Compile the current vect<int> into the final map using key and severity values extracted
    }

    // Output is of format ex) A1/3/5/1 where the last 1 indicates
    // whether it's taken or not, which is used for phase 2
    return returnMap;
}

map<string, vector<string>> processStudentHelper(char * student_compile) {
    map<string, vector<string>> returnMap;
    string inputString = string(student_compile);
    string delimDep = "="; // delimiter used to separate out each department
    string delimDetail = "/"; // delimiter used to separate information within each department

    size_t pos = 0;
    string token;

    while ((pos = inputString.find(delimDep)) != string::npos) {
        token = inputString.substr(0, pos);

        // Now we've separated each department, we also need to separate by each detail
        size_t innerPos = 0;
        string innerToken;
        string curKey;
        vector<string> rangeVector;
        int counter = 0;
        while ((innerPos = token.find(delimDetail)) != string::npos) {
            innerToken = token.substr(0, innerPos);
            if (counter == 0) {
                curKey = innerToken; // Extract key ex) Student1/3/A1/A2/A3 -> Student1
                counter++;
            } else {
                rangeVector.push_back(innerToken); // Extract severity and first 2 depts ex) Student1/3/A1/A2/A3 -> 3, A1, A2            }
            }
            token.erase(0, innerPos + delimDetail.length());
        }
        rangeVector.push_back(token); // Extract last dept ex) ex) Student1/3/A1/A2/A3 -> A3

        inputString.erase(0, pos + delimDep.length());
        returnMap[curKey] = rangeVector; // Compile the current vect<int> into the final map using key and severity values extracted
    }

    return returnMap;
}

/* This function is responsible for sending appropriate messages to clients */
void sendUDP(string curKey, string curValue, vector<string> & portVector, int ind) {
    /* ################ Set up UDP connection to receive from Health Center ###################### */
    string finalString;
    string printString;
    string curPort;
    if (curKey.find("A") != string::npos) {
        finalString = curValue;
        printString = "The health center has sent one admitted student to HospitalA";
        curPort = portVector[0];
    } else if (curKey.find("B") != string::npos) {
        finalString = curValue;
        printString = "The health center has sent one admitted student to HospitalB";
        curPort = portVector[1];
    } else if (curKey.find("C") != string::npos) {
        finalString = curValue;
        printString = "The health center has sent one admitted student to HospitalC";
        curPort = portVector[2];
    } else if (curKey.find("Student1") != string::npos) {
        finalString = "Accept#" + curValue + "#hospital" + curValue[0];
        printString = "The health center has sent the application result to Student1";
        curPort = portVector[3];
    } else if (curKey.find("Student2") != string::npos) {
        finalString = "Accept#" + curValue + "#hospital" + curValue[0];
        printString = "The health center has sent the application result to Student2";
        curPort = portVector[4];
    } else if (curKey.find("Student3") != string::npos) {
        finalString = "Accept#" + curValue + "#hospital" + curValue[0];
        printString = "The health center has sent the application result to Student3";
        curPort = portVector[5];
    } else if (curKey.find("Student4") != string::npos) {
        finalString = "Accept#" + curValue + "#hospital" + curValue[0];
        printString = "The health center has sent the application result to Student4";
        curPort = portVector[6];
    } else if (curKey.find("Student5") != string::npos) {
        finalString = "Accept#" + curValue + "#hospital" + curValue[0];
        printString = "The health center has sent the application result to Student5";
        curPort = portVector[7];
    }

    if (curValue == "0") {
        finalString = "0";
    }

    sockaddr_storage their_addr_udp; // pointer to a local sockaddr_storage where the information of incoming connection is stored
    socklen_t addr_len_udp;
    addrinfo hints_udp, *res_udp;
    socklen_t addr_size_udp;
    int sockfd_udp, valread_udp;

    // Set up the connection
    const char * curPortconv = curPort.c_str();
    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET; // IPV4 connection
    hints_udp.ai_socktype = SOCK_DGRAM; // TCP connection
    getaddrinfo("localhost", curPortconv, &hints_udp, &res_udp);

    // Using the information from getaddrinfo, set up the parent socket
    sockfd_udp = socket(res_udp -> ai_family, res_udp -> ai_socktype, res_udp -> ai_protocol);
    // Convert the finalString to char array
    const char * finalC = finalString.c_str();
    // send UDP message
    sendto(sockfd_udp, finalC, strlen(finalC), 0, res_udp -> ai_addr, res_udp -> ai_addrlen);

    if (ind == 0) {
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
        cout << "The health center has UDP port " << portNum_udp << " and IP address " << ip4_udp << " for Phase 2" << endl;
    }

    // print appropriate message from finalString vector
    if (curValue != "0") {
        cout << printString << endl;
    }
    //close sockets
    close(sockfd_udp);
}

/* Function takes returnMap which represents the hospital input in hashmap format and
compare to the student result in student_compile and return result indicating which student
got the appointment scheduled */
void processStudent(char * student_compile, char * hospital_compile, vector<string> & portVector, int ind, int mode) {
    /*
       ind is used to indicate whether to print UDP connection port and IP address information or not
       mode is used to indicate which mode this func runs in. 0 means it is used as a filter call to filter out Students
       whose input departments are all invalid. 1 means regular mode where the func handles all regular inputs from Students
    */

    map<string, vector<string>> hospitalResult; // contains result for hospitals
    vector<map<string, vector<string>>> finalResult; // compiled result
    map<string, vector<string>> studentResult; // contains result for students
    map<string, vector<int>> hospitalMap = processHospital(hospital_compile);
    map<string, vector<string>> studentMap = processStudentHelper(student_compile); // Turn student input into a hashmap style

    for (auto const & x : studentMap) {
        int matched = 0; // indicate whether a student was matched or not
        int nomatch = 0;
        int valuesize = x.second.size();
        string curStudent = x.first; // Student1,2,3,4,5
        // cout << "curStudent: " << curStudent << endl;
        int severity = stoi(x.second[0]); // severity in numerical value
        for (int i = 1; i < x.second.size(); i++) {
            string curDep = x.second[i]; // current department
            // cout << "curDep: " << curDep << endl;

            if (hospitalMap.find(curDep) == hospitalMap.end()) {
                // This condition is required to handle the case when there's department
                // from a student's request that isn't present in the hospital information.
                // Otherwise, the program would get stuck. In the project description, this was
                // supposed to be handled as inputs come in, but I found this to be more intuitive
                // implementation and went along with it.
                nomatch++;
                continue;
            }

            // Access min and max value corresponding to this current department
            int curMin = hospitalMap[curDep][0];
            int curMax = hospitalMap[curDep][1];

            if (severity <= curMax && severity >= curMin) {
                // The patient qualifies for this department
                studentResult[curStudent].push_back(curDep);
                string hospitalString = curStudent + "#" + to_string(severity) + "#" + curDep;
                hospitalResult[curDep].push_back(hospitalString);
                matched = 1;
                break;
            }
            valuesize++;
        }

        // If studentResult is still empty, assign Reject as result
        if (nomatch == (valuesize - 1)) {
            // None of the departments existed
            studentResult[curStudent].push_back("0");
        } else if (matched == 0) {
            // Simply no match, reject
            studentResult[curStudent].push_back("Reject");
        }
    }

    // compile studentMap and hospitalMap
    finalResult.push_back(studentResult);
    finalResult.push_back(hospitalResult);

    for (int i = 0; i < finalResult.size(); i++) {
        for (auto const & x : finalResult[i]) {
            // cout << "curKey: " << x.first << " curVal: ";
            string curKey = x.first;
            for (int j = 0; j < x.second.size(); j++) {
                // cout << x.second[j] << "---";
                string curVal = x.second[j];
                if (mode == 0) {
                    if (curVal == "0") {
                        sendUDP(curKey, curVal, portVector, ind); // This is when this funct is called filter out none matching inputs
                    }
                } else {
                    sendUDP(curKey, curVal, portVector, ind); // regular call
                }
                ind++;
            }
        }
    }
}

/* This function informs all clients that the process is done and they should quit */
void sendFinal(vector<string> & portVector) {
    string finishMsg = "done";
    const char * finishMsgConv = finishMsg.c_str();

    for (int i = 0; i < portVector.size(); i++) {
        const char * curPortconv = portVector[i].c_str();
        sockaddr_storage their_addr_udp; // pointer to a local sockaddr_storage where the information of incoming connection is stored
        socklen_t addr_len_udp;
        addrinfo hints_udp, *res_udp;
        socklen_t addr_size_udp;
        int sockfd_udp, valread_udp;

        // Set up the connection
        memset(&hints_udp, 0, sizeof hints_udp);
        hints_udp.ai_family = AF_INET; // IPV4 connection
        hints_udp.ai_socktype = SOCK_DGRAM; // TCP connection
        getaddrinfo("localhost", curPortconv, &hints_udp, &res_udp);

        // Using the information from getaddrinfo, set up the parent socket
        sockfd_udp = socket(res_udp -> ai_family, res_udp -> ai_socktype, res_udp -> ai_protocol);
        // Convert the finalString to char array
        // send UDP message
        sendto(sockfd_udp, finishMsgConv, strlen(finishMsgConv), 0, res_udp -> ai_addr, res_udp -> ai_addrlen);

        //close sockets
        close(sockfd_udp);
    }
}

int main()
{
    /* ################### TCP related Variables ################### */
    const int USCID = 824; // represents last 3 digits of USC ID
    int tempPort = 6000 + USCID; // as per the project phase 1 requirement
    const char * MYPORT = to_string(tempPort).c_str(); // This is the port the server will use to listen
    const int BACKLOG = 20;

    /* ################### TCP connection set up ################### */
    sockaddr_storage their_addr; // pointer to a local sockaddr_storage where the information of incoming connection is stored
    addrinfo hints, *res, *p;
    socklen_t addr_size;
    int sockfd, valread;
    int bindfd;
    char ip4[INET_ADDRSTRLEN];

    // Set up the connection
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPV4 connection
    hints.ai_socktype = SOCK_STREAM; // TCP connection
    // decided to abandon this b/c it prints out 0.0.0.0 with this. Use localhost as domain instead
    // hints.ai_flags = AI_PASSIVE; // fill in my IP address for me
    // Call getaddrinfo to grab necessary information to create a socket
    getaddrinfo("localhost", MYPORT, &hints, &res);

    // Using the information from getaddrinfo, set up the parent socket
    sockfd = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
    if (sockfd == 0) {
        cerr << "Socket file descriptor failure" << endl;
        return -1;
    }

    // Bind the socket to a port for TCP
    if (bind(sockfd, res -> ai_addr, res -> ai_addrlen) < 0) {
        cerr << "Socket bind failure" << endl;
        return -2;
    }

    // Set the parent socket to begin to listen for any connection TCP
    if (listen(sockfd, BACKLOG) < 0) {
        cerr << "Parent socket listen failure" << endl;
        return -3;
    }

    int clientfd;
    char buffer[1024];
    pid_t childpid; // need this for fork to handle multiple clients
    string checkString;

    // mmap feature needs to be used to have variables that can be accessed by different
    // processes and update: https://stackoverflow.com/questions/23258521/invalid-conversion-from-void-to-char-when-using-mmap
    // This is used to compile all the input results from hospitals
    hospital_compile = static_cast<char*> (mmap(NULL, sizeof * hospital_compile, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    num_hospital = static_cast<int*> (mmap(NULL, sizeof * num_hospital, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    hospital_inputDone = static_cast<int*> (mmap(NULL, sizeof * num_hospital, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    student_compile = static_cast<char*> (mmap(NULL, sizeof * hospital_compile, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    student_compile_perm = static_cast<char*> (mmap(NULL, sizeof * hospital_compile, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    num_student = static_cast<int*> (mmap(NULL, sizeof * num_hospital, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    TCP_done = static_cast<int*> (mmap(NULL, sizeof * num_hospital, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    ind = static_cast<int*> (mmap(NULL, sizeof * num_hospital, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    *num_hospital = 0;
    *hospital_inputDone = 0;
    *num_student = 0;
    *TCP_done = 0;
    *ind = 0;

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
    cout << "The health center has TCP port " << portNum << " and IP address " << ip4 << endl;

    /* ################### UDP related Variables ################### */
    int tempHosA = 21100 + USCID;
    int tempHosB = 21200 + USCID;
    int tempHosC = 21300 + USCID;

    int tempStu1 = 21400 + USCID;
    int tempStu2 = 21500 + USCID;
    int tempStu3 = 21600 + USCID;
    int tempStu4 = 21700 + USCID;
    int tempStu5 = 21800 + USCID;

    string HOSAPORT = to_string(tempHosA);
    string HOSBPORT = to_string(tempHosB);
    string HOSCPORT = to_string(tempHosC);

    string STU1PORT = to_string(tempStu1);
    string STU2PORT = to_string(tempStu2);
    string STU3PORT = to_string(tempStu3);
    string STU4PORT = to_string(tempStu4);
    string STU5PORT = to_string(tempStu5);

    vector<string> portVector = {HOSAPORT, HOSBPORT, HOSCPORT, STU1PORT, STU2PORT, STU3PORT, STU4PORT, STU5PORT};

    /* ################ Main TCP loop for handling inputs from Hospitals and Students ###################### */
    map<string, vector<int>> hospitalMap; // Hashmap that stores the hospital input information
    vector<map<string, vector<string>>> replyVector;
    while (true) {
        if (*TCP_done == 1) {
            // Used to break out of while loop after compiling input results
            break;
        }

        memset(buffer, 0, sizeof buffer);
        // Accept incoming connection
        addr_size = sizeof their_addr;
        clientfd = accept(sockfd, (sockaddr *) &their_addr, &addr_size);
        if (clientfd < 0) {
            cerr << "Socket acceptance failure" << endl;
            return -4;
        }

        if ((childpid = fork()) == 0) {
            // Each forked process works on the code that is only within this if statement
            // I should expand this section to also handle incoming student request. In other words,
            // should set up a logic to separate the Hospital and Student TCP connections


            string hospitalType; // used to check which hospital sent the information for current fork()
            string studentType;
            while (true) {
                memset(buffer, 0, sizeof buffer); // clear buffer or previous result can occupy
                recv(clientfd, buffer, 1024, 0); // receive the message from client
                checkString = string(buffer);

                // cout << "checkString: " << checkString << endl;

                if (checkString == "exit") {
                    // This code runs to exit out of incoming hospital TCP
                    cout << "Received the department list from Hospital " << hospitalType << endl;
                    // usleep(10000000); // sleep used to test if concurrency is working properly
                    (*num_hospital)++; // This var is used to execute the hospital info compile step
                    close(clientfd); // incoming TCP connection sockets
                    break;
                } else if (checkString.find("Student") != string::npos) {
                    // Handle incoming student TCP
                    if (checkString.find("Student1") != string::npos) {
                        studentType = "Student1";
                    } else if (checkString.find("Student2") != string::npos) {
                        studentType = "Student2";
                    } else if (checkString.find("Student3") != string::npos) {
                        studentType = "Student3";
                    } else if (checkString.find("Student4") != string::npos) {
                        studentType = "Student4";
                    } else if (checkString.find("Student5") != string::npos) {
                        studentType = "Student5";
                    }
                    strcat(student_compile, buffer);
                    strcat(student_compile_perm, buffer);
                } else if (checkString == "exitStd") {
                    // Exit condition for student TCP
                    // cout << "exiting from student TCP" << endl;
                    (*num_student)++;
                    cout << "Health center received the application from " << studentType << endl;

                    // This particular call ONLY handles cases of Student whose input only contains invalid departments
                    // and tells that Student to stop listening.
                    // This func is later called again to handle all the requests.
                    processStudent(student_compile, hospital_compile, portVector, 1, 0);
                    (*ind)++; // increment so that port and IP addresses do not get printed anymore
                    close(clientfd); // incoming TCP connection sockets
                    memset(student_compile, 0, sizeof student_compile);
                    break;
                } else {
                    /* How to check for presence of substring
                    https://stackoverflow.com/questions/2340281/check-if-a-string-contains-a-string-in-c*/

                    // This code runs to compile the results coming in from hospitals

                    // Check which hospital it is
                    if (checkString.find("A") != string::npos) {
                        hospitalType = "A";
                    } else if (checkString.find("B") != string::npos) {
                        hospitalType = "B";
                    } else {
                        hospitalType = "C";
                    }

                    // Need to handle hospital department inputs
                    strcat(hospital_compile, buffer); // keep concatenating the content of buffer which is from the client to global hospital_compile
                }
            }
            // Need to add logic for handling students request
        }

        if (*num_hospital == 3 && *hospital_inputDone == 0) {
            // This logic handles finishing up the requests from the hospitals in Phase 1

            // Now that 3 hospital info has been received, compile them into a hashmap
            // cout << "hospital_compile: " << hospital_compile << endl;
            // hospitalMap = processHospital(hospital_compile);
            cout << "End of Phase 1 for the health center" << endl;
            *hospital_inputDone = 1; // To make sure we only enter this logic once
            cout << "The health center has TCP port " << portNum << " and IP address " << ip4 << endl; // Message for phase 2
        }

        // Here should be logic to send response to students
        if (*num_student == 5) {
            // One design choice was that we could've handled students request as they come in.
            // But dynamic vector/map can't be mmapped. So the steps taken is basically we use
            // extra space to store all hospital inputs and all student inputs and handle them here
            // replyVector = processStudent(student_compile, hospital_compile);
            (*TCP_done)++; // To break out of the entire while loop
        }
    }
    close(sockfd); // close main parent socket

    /* ################ Use UDP connection to send back appropriate messages to students and hospitals ###################### */
    // As mentioned above, this function is actually called to handle all appropriate responses
    processStudent(student_compile_perm, hospital_compile, portVector, 0, 1); // Send appropriate UDP response

    // Now send a message to all clients to finish
    sendFinal(portVector);

    return 0;
}
