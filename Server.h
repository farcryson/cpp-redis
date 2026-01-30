#include<vector>
#include<poll.h>
#include<unordered_map>
#include<string>

class Server{
private:
    int server_socket_fd;
    int port;

    std::vector<pollfd> fds;

    std::unordered_map<std::string, std::string> kv_store;

    void init_socket();
    void accept_new_connection();
    void handle_client_data(int& client_fd_index);
    std::string process_commands(std::vector<std::string>& tokens);

public:
    Server(int port);
    
    void run();
};