#include <bits/stdc++.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <fstream>

using namespace std;

vector<string> get_data_from_file(string name) {
    vector<string> res;
    string line;
    ifstream myfile(name);
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            res.push_back(line);
        }
        myfile.close();
    }
    return res;
}

int main() {
    string client, ipAddress;
    vector<string> tokens, com;
    bool get, text, html, image;
    int cnt = 1, port;

    cin >> client >> ipAddress >> port;

    //  Create a hint structure for the server we're connecting with

    //freopen("commands.txt", "r", stdin);
    com = get_data_from_file("commands.txt");
    for (int i = 0; i < com.size(); i++) {
        //  Create a socket
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
            return 1;

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

        text = html = image = false;
        char *p = strtok((char *) com[i].c_str(), " ");
        while (p) {
            tokens.push_back(p);
            p = strtok(NULL, " ");
        }
        if (tokens[0] == "GET")
            get = true;
        else
            get = false;
        if (tokens.size() > 3) {
            string pn = tokens[3].substr(1, tokens[3].size() - 2);
            port = std::stoi(pn);
        }
        if (tokens[1].find(".txt") != std::string::npos)
            text = true;
        else if (tokens[1].find(".html") != std::string::npos)
            html = true;
        else
            image = true;
        string msg = "";
        msg.append(tokens[0] + " " + tokens[1] + " HTTP/1.1\n" + "Host: " + tokens[2] + " \n");

        //  Connect to the server on the socket
        int connectRes = connect(sock, (sockaddr *) &hint, sizeof(hint));
        if (connectRes == -1)
            return 1;

        //  While loop:
        char buf[4096];

        //      Send to server
        int sendRes = send(sock, msg.c_str(), msg.size() + 1, 0);
        if (sendRes == -1) {
            cout << "Could not send to server! Whoops!\r\n";
            continue;
        }

        //      Wait for response
        memset(buf, 0, 4096);
        int bytesReceived = -1;
        if(!image)
        bytesReceived = recv(sock, buf, 4096, 0);
        else
            bytesReceived = 1;

        if (bytesReceived == -1)
            cout << "There was an error getting response from server\r\n";
        else {
            //      Display response
            if (get) {
                if (!image) {
                    freopen(tokens[1].c_str(), "w", stdout);
                    int i = 0;
                    while (buf[i] != '\0')
                        cout << buf[i++];
                } else
                {
                    FILE *file = fopen(tokens[1].c_str(), "wb");
                    char b[2048];
                    int recvsize;
                    recvsize = recv(sock, b, sizeof(b), 0);
                    while (recvsize > 0) {
                        fwrite(b, 1, recvsize, file);
                        recvsize = recv(sock, b, sizeof(b), 0);
                        if (recvsize < 0)
                            cout << "failed to receive image" << endl;
                    }
                    fclose(file);
                }
            } else {
                string m = buf;
                if (m.find("200 OK") != std::string::npos) {
                    string msg2 = "";
                    if (std::ifstream(tokens[1])) {
                        if (!image) {
                            vector<string> s = get_data_from_file(tokens[1]);
                            for (int j = 0; j < s.size(); j++)
                                msg2 += s[j];
                        } else {
                            FILE *file = fopen(tokens[1].c_str(), "rb");
                            char b[2048];
                            int sz;
                            while (!feof(file)) {
                                sz = fread(b, 1, 2048, file);
                                if (send(sock, b, 2048, 0) != sz)
                                    cout << "send failed" << endl;
                            }
                            fclose(file);
                        }
                    } else
                        cout << "File doesn`t exists !!!" << endl;
                    if(!image)
                    {
                        int sendRes = send(sock, msg2.c_str(), msg2.size() + 1, 0);
                        if (sendRes == -1) {
                            cout << "Could not send to server! Whoops!\r\n";
                            continue;
                        }
                    }
                } else
                    cout << "server doesn`t confirm sending !!!" << endl;
            }
        }
        tokens.clear();
        //  Close the socket
        close(connectRes);
        cnt++;
    }
    return 0;
}
