#include "Server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

using namespace std;

Server::Server(int port) : port(port)
{
    init_socket();
}

Server::~Server() {
    cout<<"Shutting down server..."<<endl;

    close(server_socket_fd);

    for(int i=0;i<fds.size();i++) {
        if(fds[i].fd >= 0) {
            close(fds[i].fd);
            fds[i].fd = -1;
        }
    }
}

void Server::init_socket()
{
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setpocket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket_fd, 5) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    pollfd p;
    p.fd = server_socket_fd;
    p.events = POLLIN;
    fds.push_back(p);

    cout << "Server listening on port " << port << " ..." << endl;
}

void Server::run()
{
    while (true)
    {
        int ready = poll(fds.data(), fds.size(), -1);
        if (ready == -1)
        {
            perror("Poll failed");
            break;
        }
        int i = 0;
        for (; i < fds.size(); i++)
        {
            if (fds[i].revents)
            {
                if (fds[i].fd == server_socket_fd)
                {
                    accept_new_connection();
                }
                else if (fds[i].revents & POLLIN)
                {
                    handle_client_data(i);
                }
            }
        }
    }
}

void Server::accept_new_connection()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd >= 0)
    {
        pollfd p;
        p.fd = client_fd;
        p.events = POLLIN;
        fds.push_back(p);
    }
}

void Server::handle_client_data(int &index)
{
    char buffer[4096];
    int client_fd = fds[index].fd;
    int bytes_read = read(client_fd, buffer, sizeof(buffer));

    if (bytes_read <= 0)
    {
        close(client_fd);
        fds.erase(fds.begin() + index);
        index--;
        return;
    }

    buffer[bytes_read] = '\0';
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
    response_buffer += process_commands(commands);

    int e = send(client_fd, response_buffer.c_str(), response_buffer.size(), 0);
    if (e == -1)
        close(client_fd);
}

std::string Server::process_commands(std::vector<std::string> &commands)
{
    int i = 0;
    string response = "";
    while (i < commands.size())
    {
        string s = "+PONG\r\n";
        string cmd;

        string cmdno = commands[i].substr(1);
        int nCmds = stoi(cmdno);

        if (commands.size() > i + 2)
            cmd = commands[i + 2];
        if (cmd == "COMMAND")
        {
            s = "-ERR unknown command 'COMMAND'\r\n";
            if (commands.size() > i + 4 && commands[i + 4] == "DOCS")
                i += (2 * nCmds + 1);
            else
                i += (2 * nCmds + 1);
            response += s;
            continue;
        }
        if (cmd == "CONFIG")
        {
            s = "-ERR unknown command 'CONFIG'\r\n";
            i += (2 * nCmds + 1);
            response += s;
            continue;
        }

        // if(commands.size() > i+2) cout << "User provided command is " << commands[i+2] << endl;

        if (commands.size() > i + 4 && commands[i + 2] == "ECHO")
        {
            s = "+" + commands[i + 4] + "\r\n";
            i += (2 * nCmds + 1);
        }
        else if (commands.size() > i + 6 && commands[i + 2] == "SET")
        {
            kv_store[commands[i + 4]] = commands[i + 6];
            s = "+OK\r\n";
            i += (2 * nCmds + 1);
        }
        else if (commands.size() > i + 4 && commands[i + 2] == "GET")
        {
            if (kv_store.count(commands[i + 4]))
            {
                string val = kv_store[commands[i + 4]];
                s = "$" + to_string(val.size()) + "\r\n" + val + "\r\n";
            }
            else
            {
                s = "$-1\r\n";
            }
            i += (2 * nCmds + 1);
        }
        else
            i += (2 * nCmds + 1);
        response += s;
    }
    return response;
}