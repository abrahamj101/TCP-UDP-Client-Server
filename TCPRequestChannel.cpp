#include "TCPRequestChannel.h"
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

using namespace std;


TCPRequestChannel::TCPRequestChannel (const std::string _ip_address, const std::string _port_no) {
    /*
    if server
        create a socket on the specified port
            specify domain, type, protocol
        bind the socket to address set-ups listening
        mark socket as listening

    if client
        create socket on the specified
            specify domain, type, protocl
        connect socket to IP address of the server
    */

    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        throw runtime_error("Failed to create socket");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(_port_no));

    if (_ip_address.empty()) {  // Server
        server_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            throw runtime_error("Bind failed");
        }
        if (listen(sockfd, 10) < 0) {
            throw runtime_error("Listen failed");
        }
    } else {  // Client
        if (inet_pton(AF_INET, _ip_address.c_str(), &server_addr.sin_addr) <= 0) {
            throw runtime_error("Invalid address / Address not supported");
        }
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            throw runtime_error("Connection Failed");
        }
    }
    

    /*
   // set up variables
   if (_ip_address == "") // for server
   {
        struct sockaddr_in server;
        int server_sock, bind_stat, listen_stat;

        // socket - make socket - socket(int domain, int type, int protocol)
        // AF_INET = IPv4
        // SOCK_STREAM = TCP
        // Normally only a single protocol exists to support a prticular socket type
        // within a given protocol family, in which case protocal can be specified as
        // 0

        // provide necessary machine info for sockaddr_in
        // address family, IPv4
        // IPv4 address, use current IPv4 address (INADDR_ANY)
        // connection port
        // convert short from host byte order to network byte order

        // bind - assign address to socket - bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
        //listen - listen for client - listen(int sockfd, int backlog)
        
        //accept - accept connection
        //written in a seperate method
   }
   else // for client
   {
        //setup variable
        struct sockaddr_in server_info;
        int client_sock, connect_stat;

        // generate server's info based on parameters
        // address family, IPv4
        // connection port
        // convert short from host byte order to network byte order
        // convert ip address c-string to binary representation for sin_addr

        //connect - connect to listiening socket - connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
   } 
   */
}

TCPRequestChannel::TCPRequestChannel (int _sockfd) : sockfd(_sockfd) {
    // assign an existing socket to object's socket file descriptor
}

TCPRequestChannel::~TCPRequestChannel () {
    // close sockfd - close(this->sockfd)
    close(sockfd);
}

int TCPRequestChannel::accept_conn () {
    // struct sockaddr_storage 
    // implementing sock's accept(...) --> returns the sockfd of the client
    //or
    // accept : accept connection
    // socket file descriptor for accepted connection
    // accept connection : accept(int sockfd, strict sockaddr *addr, socklen_t *addrlen)
    // return socket file descriptor

    int new_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    new_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

    if (new_socket < 0) {
        perror("accept failed");
        exit(6);
    }
    return new_socket;
}

// read/write, recv/send
int TCPRequestChannel::cread (void* msgbuf, int msgsize) {
    //ssize_t no_bytes; //number of bytes to read
    // read from socket - read(int fd, void *buf, size_t count)
    //return number of bytes read
    //cout<<"ENTERED CREAD"<<endl;
    return read(sockfd, msgbuf, msgsize);
}

int TCPRequestChannel::cwrite (void* msgbuf, int msgsize) {
    //ssize_t no_bytes; //number of bytes to write
    // write to socket - write(int fd, void *buf, size_t count)
    // return number of bytes written
    return write(sockfd, msgbuf, msgsize);
}
