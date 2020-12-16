#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <pthread.h>

using namespace std;

string method = "", url = "", fileType = "", fileName = "";
int errorFlag = 0;

// make struct to pass all arguments to pthread_create
struct arguments {
    char buf[4096];
    char fileBuff[4096];
    int clientSocket;
};


// write received buffer to file
void writeToFile(char buff[]) {
    string tmpstr(buff);
    istringstream is(tmpstr);
    string line;
    ofstream myfile;
    myfile.open(url);
    while (getline(is, line)) {
        myfile << line;
        myfile << endl;
    }
    myfile.close();
}

// get current time and date
string getTimeAndDate() {
    time_t now = time(0);           // current date/time based on current system
    string dt = ctime(&now);      // convert now to string form
    string currentDateAndTime = "Date: ";
    currentDateAndTime.append(dt);
    currentDateAndTime.append("\r");
    return currentDateAndTime;
}

// check if files is exist
bool is_file_exist(const std::string &name) {
    std::ifstream input(url);
    if (!input) {
        return false;
    }
    return true;
}

// get method type GET or POST
void getMethod(char buff[]) {
    string tmpstr(buff);
    istringstream is(tmpstr);
    string line;
    getline(is, line);
    if (line.find("GET") != std::string::npos) {
        method = "GET";
    } else if (line.find("quit") != std::string::npos) {
        method = "quit";
    } else if (line.find("POST") != std::string::npos) {
        method = "POST";
    }
}

// get URL from request message
void getUrl(char buff[]) {
    url = "";
    fileName = "";
    string tmpstr(buff);
    istringstream is(tmpstr);
    string line;
    int counter = 0;
    while (getline(is, line) && counter < 2) {
        if (counter == 0) {
            for (std::string::size_type i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == ' ') {
                    i++;
                    c = line[i];
                    while (c != ' ') {
                        url += c;
                        i++;
                        c = line[i];
                    }
                }
            }
        }

        if (counter == 1) {
            fileName = url;
            url = "";
            for (std::string::size_type i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == ' ') {
                    i++;
                    c = line[i];
                    while (c != ' ') {

                        url += c;
                        i++;
                        c = line[i];
                    }
                }
            }
        }

        counter++;
    }
    url.resize(url.length() - 1);
    url += fileName;
}

// get file type html. txt or image
void getFileType(char buff[]) {
    string tmpstr(buff);
    istringstream is(tmpstr);
    string line;
    getline(is, line);
    if (line.find("html") != std::string::npos) {
        fileType = "html";
    } else if (line.find("txt") != std::string::npos) {
        fileType = "txt";
    } else {
        fileType = "image";
    }
}

// make response message
string makeResponseMessage(string type) {
    string message = "", dateTime = "";
    dateTime = getTimeAndDate();
    if (type == "GET") {
        // make GET response message with data in message body
        if (is_file_exist(url)) {
            message.append("HTTP/1.1 200 OK\r\n");
            message.append(dateTime);
            message.append("Accept-Ranges: bytes\r\n");
            message.append("Connection: Keep-Alive\r\n");
            message.append("Content-Type: text/html; charset=UTF-8\r\n");\
            message.append("\r\n\n");
            // read from file
            string line;
            ifstream myfile(url);
            if (myfile.is_open()) {
                while (getline(myfile, line)) {
                    message.append(line + "\n");
                }
                myfile.close();
            } else {
                // file found but cannot be opened
                cout << "Unable to open file";
            }
        } else {
            // file not found
            message.append("HTTP/1.1 404 Not Found\r\n");
            message.append(dateTime);
            message.append("Content-Type: text/html;\r\n");
            message.append("\\r\\n");
            message.append("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\\r\\n");
            message.append("<html><head>\r\n");
            message.append("<title>404 Not Found</title>\r\n");
            message.append("</head><body>\r\n");
            message.append("<h1>Not Found</h1>\r\n");
            message.append("<p>The requested URL " + url + "was not found on this server.</p>\r\n");
            message.append("</body></html>\r\n\n");
        }
    } else {
        // make POST response message
        message.append("HTTP/1.1 200 OK\r\n");
        message.append(dateTime);
        message.append("Accept-Ranges: bytes\r\n");
        message.append("Connection: Keep-Alive\r\n");
        message.append("Content-Type: text/html; charset=UTF-8\r\n");\
        message.append("\r\n\n");
    }
    return message;
}

// handle request message
void *handleRequest(void *a) {
    // get thread
    pthread_detach(pthread_self());
    // get data from struct
    struct arguments *b;
    b = (struct arguments *) a;
    int clientSocket = b->clientSocket;
    char buf[4096];
    char fileBuff[4096];
    strcpy(buf, b->buf);
    strcpy(fileBuff, b->fileBuff);

    // clear buffers
    memset(buf, 0, 4096);
    memset(fileBuff, 0, 4096);

    int bytesReceived = recv(clientSocket, buf, 4096, 0);           // Wait for client to send data

    if (bytesReceived == -1) {
        cerr << "Error in recv(). Quitting" << endl;
        errorFlag = true;
    }

    if (bytesReceived == 0) {
        cout << "Client disconnected " << endl;
        errorFlag = true;
    }
    cout << "Received: " << string(buf, 0, bytesReceived) << endl;          // display what client has set

    // get information from request message
    getMethod(buf);
    getUrl(buf);
    getFileType(buf);


    if (method == "GET") {
        // handle GET request
        cout << "GET request" << endl;
        if (fileType == "html" || fileType == "txt") {
            string message = makeResponseMessage("GET");
            char charMessage[message.size() + 1];
            strcpy(charMessage, message.c_str());
            send(clientSocket, charMessage, sizeof(charMessage), 0);
        } else {
            // sent image to client without response message
            // TODO send image
            FILE *file = fopen(url.c_str(), "rb");
            char b[2048];
            int sz;
            while (!feof(file)) {
                sz = fread(b, 1, 2048, file);
                if (send(clientSocket, b, 2048, 0) != sz)
                    cout << "send failed" << endl;
            }
            fclose(file);
        }

    } else if (method == "quit") {
        cout << "Quitting" << endl;
        errorFlag = 1;
    } else {
        // handle POST request
        cout << "POST request" << endl;
        string message = makeResponseMessage("POST");
        char charMessage[message.size() + 1];
        strcpy(charMessage, message.c_str());
        send(clientSocket, charMessage, sizeof(charMessage), 0);

        if (fileType == "html" || fileType == "txt") {
            // receive file from client
            int receivedFile = recv(clientSocket, fileBuff, 4096, 0);           // Wait for client to send data
            if (receivedFile == -1) {
                cerr << "Error receiving file from client" << endl;
                errorFlag = 1;
            } else if (receivedFile == 0) {
                cout << "Client disconnected before sending file" << endl;
                errorFlag = 1;
            } else {
                string a = fileBuff;
                if (!a.empty())
                    writeToFile(fileBuff);
            }
        } else {
            // receive file chunks from client and save it into a file
            FILE *file = fopen(url.c_str(), "wb");
            char b[2048];
            int recvsize;
            recvsize = recv(clientSocket, b, sizeof(b), 0);
            while (recvsize > 0) {
                fwrite(b, 1, recvsize, file);
                recvsize = recv(clientSocket, b, sizeof(b), 0);
                if (recvsize < 0)
                    cout << "failed to receive image" << endl;
            }
            fclose(file);
        }

    }
    close(clientSocket);
}

int main() {
    int port;
    string server;
    cin >> server >> port;
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);          // AF_INET =>ipv4, SOCK_STREAM for tcp connection
    if (listening == -1) {
        cerr << "Can't create a socket! Quitting" << endl;
        return -1;
    }

    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);         // set port number, htons: flip bits to match the machine
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);          // ip address
    std::stringstream ss;
    ss << &hint.sin_addr;
    cout << "server ip address: " << ss.str() << endl;
    if (bind(listening, (sockaddr *) &hint, sizeof(hint)) == -1) {
        cerr << "Can't bind to ip/port" << endl;
        return -2;
    }

    // Mark the socket for listening in
    if (listen(listening, SOMAXCONN) == -1) {
        cerr << "Can't listen" << endl;
        return -3;
    }

    // Wait for a connection
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    char host[NI_MAXHOST];      // Client's remote name
    char service[NI_MAXSERV];   // Service (i.e. port) the client is connect on


    // While loop: accept and echo message back to client
    char buf[4096];
    char fileBuff[4096];

    while (true) {
        int clientSocket = accept(listening, (sockaddr *) &client, &clientSize);
        if (clientSocket == -1) {            // check for connection
            cerr << "Problem with client connecting!!" << endl;
            return -4;
        }

        memset(host, 0, NI_MAXHOST);            // get rid of garbage in hot array
        memset(service, 0, NI_MAXSERV);       // get rid of garbage in service array

        if (getnameinfo((sockaddr *) &client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
            cout << host << " connected on port " << service << endl;
        } else {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << host << " connected on port " << ntohs(client.sin_port) << endl;
        }

        // Close listening socket
        //close(listening);

        pthread_t t;
        // buf, fileBuff, clientSocket
        struct arguments *a;
        a = static_cast<arguments *>(malloc(sizeof(struct arguments)));
        a->clientSocket = clientSocket;
        strcpy(a->buf, buf);
        strcpy(a->fileBuff, fileBuff);

        pthread_create(&t, 0, handleRequest, (void *) a);

        if (errorFlag == 1) {
            break;
        }
        //send(clientSocket, buf, bytesReceived + 1, 0);          // Echo message back to client
    }
    // Close the socket
    // cout << "Closing Connection....\nClosed" << endl;
//        close(clientSocket);

    return 0;
}
