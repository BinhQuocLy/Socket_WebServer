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
        FileReader::readContent(".\\resources\\index.html", _pages[0]);
        FileReader::readContent(".\\resources\\info.html", _pages[1]);
        FileReader::readContent(".\\resources\\error.html", _pages[2]);

        initializeWinSock();
    }

    void run() {
        std::cout << "Server is on!\n";
        createSocket(_result, _listenSocket, _port);
        bindSocket(_result, _listenSocket);
        listenOnSocket(_listenSocket);

        //Accept client connections
        while (true) {
            SOCKET clientSocket = acceptConnection();
            std::thread th(&Server::handleRequests, this, std::ref(clientSocket), _pages); //Handle requests from many clients concurrently
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

        //Create a socket for connecting to server
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
    
    SOCKET acceptConnection() {
        //Accept a client socket/connection
        std::cout << "\nWaiting to accept new connection\n";
        SOCKET clientSocket = accept(_listenSocket, NULL, NULL);
        std::cout << "Accepted " << clientSocket << std::endl;

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            cleanUp();
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
        bool authorized = false;

        //Receive requests
        do {
            iRecvResult = recv(clientSocket, recvbuf, recvbuflen, 0);
            if (iRecvResult > 0) {
                request.assign(recvbuf, iRecvResult);

                //Handle GET or POST requests
                if (request.find("GET") != -1) {
                    handleGET(request, pages, response, authorized);
                }
                else {
                    handlePost(request, pages, response, authorized);
                }

                //Send responses
                iSendResult = send(clientSocket, response.c_str(), response.size(), 0);
                if (iSendResult == SOCKET_ERROR) {
                    std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                    return;
                }

                //Nothing more to send, proceed to shutdown the connection
                if (response.find(pages[1]) != -1 ||
                    response.find(pages[2]) != -1) {
                    iRecvResult = 0;
                }
            }
            else if (iRecvResult < 0) {
                std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                return;
            }
        } while (iRecvResult > 0);

        //Shutdown the connection
        std::cout << "Connection with " << clientSocket << " closing...\n\n";
        iRecvResult = shutdown(clientSocket, SD_SEND);
        if (iRecvResult == SOCKET_ERROR) {
            std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            return;
        }
    }


    /*---------- Utility methods ----------*/

    void handleGET(const std::string& request, const std::string* pages, std::string& response, bool& authorized) {
        /**Redirect to correct URLs**/
        if (request.find("/ ") != -1 ||
            request.find("/index ") != -1) {
            response = "<script>window.location.href = \"index.html\";</script>";
            response = getResponse(response, 301, "Moved permanently");
            return;
        }

        if (request.find("info ") != -1) {
            response = "<script>window.location.href = \"info.html\";</script>";
            response = getResponse(response, 301, "Moved permanently");
            return;
        }

        /**Process correct URLs**/
        if (request.find("/index.html ") != -1) { //Proceed to index.html
            response = pages[0];
            response = getResponse(response, 200, "OK");
        }
        else if (request.find("/info.html ") != -1) {
            if (authorized == false) { //Unauthorized, proceed to index.html
                response = "<script>window.location.href = \"index.html\";</script>";
                response = getResponse(response, 301, "Moved permanently");
            }
            else { //Authorized, proceed to info.html
                response = pages[1];
                response = getResponse(response, 200, "OK");
            }
        }
        else { //Requested URLs not found
            response = pages[2];
            response = getResponse(response, 404, "Not found");
        }
    }

    void handlePost(const std::string& request, const std::string* pages, std::string& response, bool& authorized) {
        //Get username and password
        std::string token = request.substr(request.find("username"));
        int pos = token.find('=') + 1;
        std::string username = token.substr(pos, token.find('&') - pos);
        token = token.substr(token.find('&'));
        pos = token.find('=') + 1;
        std::string password = token.substr(pos);

        //Authentify the user
        authorized = authentify(username, password);
        if (authorized) {
            response = getResponse(response, 302, "Found");
        }
        else {
            response = getResponse(response, 401, "Unauthorized");
        }
    }

    std::string getResponse(std::string& content, int statusCode, const std::string& message) {
        //Create a response string
        content =
            "HTTP/1.1 " +
            std::to_string(statusCode) +
            " " + message +
            "\nContent-Type: text/html"
            "\nContent-Length: " + std::to_string(content.size())
            + "\n\n" + content;
        return content;
    }

    bool authentify(const std::string& username, const std::string& password) {
        //Proccess string to check for valid username and password
        std::mutex securedResouce; //Used to protect shared resource
        std::string token;
        securedResouce.lock(); 
        FileReader::readContent(".\\resources\\userinfo.dat", token);
        securedResouce.unlock();
        std::string u = token.substr(0, token.find('&'));
        std::string p = token.substr(token.find('&') + 1);

        if (username.find(u) != -1 && password.find(p) != -1) {
            return true;
        }
        return false;
    }

    void cleanUp() {
        //Free up resources
        WSACleanup();
        freeaddrinfo(_result);
        closesocket(_listenSocket);
    }
private:
    char _port[5];
    std::string _pages[3];
    addrinfo* _result;
    SOCKET _listenSocket;
};



