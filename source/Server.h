#pragma once

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "FileReader.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <thread>
#include <mutex>

//Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"
#define DEFAULT_BUFLEN 2048

class Server
{
public:
    Server(const std::string& port = DEFAULT_PORT) {
        port.copy(_port, port.size());
        _result = NULL;
        _listenSocket = INVALID_SOCKET;

        //Pre-load pages
        FileReader::readContent("index.html", _pages[0]);
        FileReader::readContent("info.html", _pages[1]);
        FileReader::readContent("error.html", _pages[2]);
        FileReader::readContent("index_error.html", _pages[3]);

        initializeWinSock();
    }

    void run() {
        std::cout << "Server is on!\n";
        createSocket(_result, _listenSocket, _port);
        bindSocket(_result, _listenSocket);
        listenOnSocket(_listenSocket);

        while (true) {
            SOCKET clientSocket = acceptRequests();
            std::thread th(&Server::handleRequests, this, std::ref(clientSocket), _pages);
            th.detach();
        }
    }
   
    ~Server() {
        cleanUp();
    }
private:
    /*---------- Socket-based methods ----------*/

    void initializeWinSock() {
        WSADATA wsaData;

        //Initialize Winsock
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
            cleanUp();
            throw(std::runtime_error("failed to initialize"));
        }
    }

    void createSocket(addrinfo*& result, SOCKET& listenSocket, const char* port) {
        addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        //Resolve the server address and port
        int iResult = getaddrinfo(NULL, port, &hints, &result);
        if (iResult != 0) {
            std::cerr << "getaddrinfo failed with error: " << iResult << std::endl;
            cleanUp();
            throw(std::runtime_error("failed to get address info"));
        }

        //Create a SOCKET for connecting to server
        listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
            cleanUp();
            throw(std::runtime_error("failed to create socket"));
        }
    }

    void bindSocket(addrinfo* result, const SOCKET& listenSocket) {
        //Setup the TCP listening socket
        int iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
            cleanUp();
            throw(std::runtime_error("failed to bind socket"));
        }
    }

    void listenOnSocket(const SOCKET& listenSocket) {
        //Open a socket to listen 
        int iResult = listen(listenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
            cleanUp();
            throw(std::runtime_error("failed to listen on socket"));
        }
        std::cout << "Listening on port " << _port << std::endl;
    }
    
    SOCKET acceptRequests() {
        //Accept a client socket
        std::cout << "\nWaiting to accept new connection\n";
        SOCKET clientSocket = accept(_listenSocket, NULL, NULL);
        std::cout << "Accepted " << clientSocket << std::endl;

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            throw(std::runtime_error("failed to accept clients"));
        }
        return clientSocket;
    }

    void handleRequests(SOCKET& clientSocket, const std::string* pages) {
        int iRecvResult;
        int iSendResult;
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;
        std::string request;
        std::string response;

        //Receive requests
        std::cout << "Waiting for new request from " << clientSocket << std::endl;
        iRecvResult = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (iRecvResult > 0) {
            std::cout << "Received request\n";
            request.assign(recvbuf, iRecvResult);

            if (request.find("GET") == 0) {
                handleGET(request, pages, response);
            }
            else {
                handlePost(request, pages, response);
            }

            iSendResult = send(clientSocket, response.c_str(), response.size(), 0);
            if (iSendResult == SOCKET_ERROR) {
                std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                throw(std::runtime_error("failed to send responses"));
            }
            std::cout << "Sent response\n";
        }
        else if (iRecvResult == 0) {
            std::cout << "No request received\n";
        }
        else {
            std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            throw(std::runtime_error("failed to receive requests"));
        }

        //Shutdown the connection
        std::cout << "Connection closing...\n\n";
        iRecvResult = shutdown(clientSocket, SD_SEND);
        if (iRecvResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            throw(std::runtime_error("failed to shutdown"));
        }
    }


    /*---------- Utility methods ----------*/

    void handleGET(const std::string& request, const std::string* pages, std::string& response) {
        if (request.find("/ ") != -1 ||
            request.find("/index.html ") != -1) {
            response = pages[0];
            response = getResponse(response, 200, "OK");
            std::cout << "Sent index.html\n";
        }
        else {
            response = pages[2];
            response = getResponse(response, 404, "ERROR");
            std::cout << "Sent error.html\n";
        }
    }

    void handlePost(const std::string& request, const std::string* pages, std::string& response) {
        if (request.find("/info.html ") != -1) {
            //Get username and password
            std::string token = request.substr(request.find("username"));
            int pos = token.find('=') + 1;
            std::string username = token.substr(pos, token.find('&') - pos);
            token = token.substr(token.find('&'));
            pos = token.find('=') + 1;
            std::string password = token.substr(pos);

            //Authentify users
            if (authentified(username, password)) {
                response = pages[1];
                response = getResponse(response, 200, "OK");
                std::cout << "Sent info.html\n";
            }
            else {
                response = pages[3];
                response = getResponse(response, 200, "OK");
                std::cout << "Wrong username or password - sent index_error.html\n";
            }
        }
        else {
            response = pages[2];
            response = getResponse(response, 404, "ERROR");
            std::cout << "Sent error.html\n";
        }
    }

    std::string getResponse(std::string& content, int statusCode, const std::string& message) {
        content =
            "HTTP/1.1 " +
            std::to_string(statusCode) +
            " " + message +
            "\nContent-Type: text/html"
            "\nContent-Length: " + std::to_string(content.size())
            + "\n\n" + content;
        return content;
    }

    bool authentified(const std::string& username, const std::string& password) {
        //Proccess string to check for valid username and password
        std::mutex securedResouce; //Prevent deadlock
        std::string token;
        securedResouce.lock();
        FileReader::readContent("userinfo.dat", token);
        securedResouce.unlock();
        std::string u = token.substr(0, token.find('&'));
        std::string p = token.substr(token.find('&') + 1);

        if (username.find(u) != -1 && password.find(p) != -1) {
            return true;
        }
        return false;
    }

    void cleanUp() {
        WSACleanup();
        freeaddrinfo(_result);
        closesocket(_listenSocket);
    }
private:
    char _port[5];
    std::string _pages[4];
    addrinfo* _result;
    SOCKET _listenSocket;
};



