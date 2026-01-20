#include <iostream>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
using namespace std;

int main()
{
    // build socket-> bind to a port-> listen for calls-> accept the connection

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(6379);

    // build socket
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setpocket failed");
        return -1;
    }

    // bind to a port
    int a = bind(socket_id, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (a < 0)
    {
        perror("Bind failed!");
        return -1;
    }
    cout << "Bind successful!" << endl;

    // listen
    int b = listen(socket_id, 5);
    if (b < 0)
    {
        perror("Listen failed!");
        return -1;
    }
    cout << "Server is listening at PORT 6379..." << endl;

    // accept

    vector<pollfd> v;
    pollfd p;
    p.fd = socket_id;
    p.events = POLLIN;
    v.push_back(p);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    unordered_map<string, string> kv_store;

    while (true)
    {
        int ready = poll(v.data(), v.size(), -1);
        if (ready == -1)
        {
            perror("Poll failed");
            return -1;
        }
        for (int i = 0; i < v.size(); i++)
        {
            if (v[i].revents)
            {
                if (v[i].fd == socket_id)
                {
                    int c = accept(socket_id, (struct sockaddr *)&client_addr, &client_len);
                    if (c == -1)
                    {
                        cout << "Accept failed";
                        return -1;
                    }
                    pollfd clientFd;
                    clientFd.fd = c;
                    clientFd.events = POLLIN;
                    v.push_back(clientFd);
                }
                else if (v[i].revents & POLLIN)
                {
                    int clientFd = v[i].fd;
                    char buffer[4096];
                    int d = read(clientFd, buffer, 4096);
                    if (d == 0)
                    {
                        close(v[i].fd);
                        v.erase(v.begin() + i);
                        i--;
                        continue;
                    }
                    buffer[d] = '\0';
                    // cout << buffer << endl;

                    stringstream ss(buffer);
                    string token;
                    vector<string> commands;

                    while (getline(ss, token, '\n'))
                    {
                        token.pop_back();
                        commands.push_back(token);
                    }

                    string response_buffer = "";
                    int i = 0;
                    while (i < commands.size())
                    {   
                        string s = "+PONG\r\n";
                        string cmd;

                        string cmdno = commands[i].erase(0,1);
                        int nCmds = stoi(cmdno);

                        if(commands.size() > i+2) cmd = commands[i+2];
                        if(cmd == "COMMAND") {
                            s = "-ERR unknown command 'COMMAND'\r\n";
                            if(commands.size() > i+4 && commands[i+4] == "DOCS") i += (2*nCmds +1);
                            else i += (2*nCmds +1);
                            response_buffer += s;
                            continue;
                        }
                        if(cmd == "CONFIG") {
                            s = "-ERR unknown command 'CONFIG'\r\n";
                            i += (2*nCmds +1);
                            response_buffer += s;
                            continue;
                        }

                        // if(commands.size() > i+2) cout << "User provided command is " << commands[i+2] << endl;

                        if (commands.size() > i+4 && commands[i+2] == "ECHO")
                        {
                            s = "+" + commands[i+4] + "\r\n";
                            i += (2*nCmds +1);
                        }
                        else if (commands.size() > i+6 && commands[i+2] == "SET")
                        {
                            kv_store[commands[i+4]] = commands[i+6];
                            s = "+OK\r\n";
                            i += (2*nCmds +1);
                        }
                        else if (commands.size() > i+4 && commands[i+2] == "GET")
                        {
                            if (kv_store.count(commands[i+4]))
                            {
                                string val = kv_store[commands[i+4]];
                                s = "$" + to_string(val.size()) + "\r\n" + val + "\r\n";
                            }
                            else
                            {
                                s = "$-1\r\n";
                            }
                            i += (2*nCmds +1);
                        }
                        else  i += (2*nCmds +1);
                        response_buffer += s;
                    }
                    int e = send(clientFd, response_buffer.c_str(), response_buffer.size(), 0);
                }
            }
        }
    }

    // read

    close(socket_id);
    return 0;
}