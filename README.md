# COVID Simulation Network Project
![alt text](https://github.com/frozendrpepper/Multiclient_Network_TCP_UDP/blob/master/project_demo.png)  
(Source: USC EE450 Summer 2020 Project Homepage)

This is a multi-client, single server socket programming project which utilizes forking to handle multiple client handling on the server side. Messages between clients and the server are carried out both through UDP and TCP connections.

## Technology Used
* C++
* Socket Programming (TCP/UDP)

## File Explanation
* Hospitals = Hospitals send available departments to the Healthcenter (server) through TCP connection with dedicated port number which indicates the severity of symptoms each department can handle
* Students = Students send appointment request with their COVID symptoms and preference for certain departments through TCP connection with dedicated port number
* Health Center = Acts as the main server that handles multiple client request via fork() process. Matches students request by analyzing available departments and symptoms and send results back to Hospitals and Students via UDP connection

### Project Origin
This project was carried out as part of EE450 Computer Networks class at USC
