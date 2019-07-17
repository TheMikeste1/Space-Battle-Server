/*********************************************************************
 * File: game.h
 * Description: Defines the game class for Asteroids
 *
 *********************************************************************/

#ifndef CONNECTION_H
#define CONNECTION_H

//Console
#include <iostream>
#include <vector>
using namespace std;

//Networking
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "6789"
#define BACKLOG 10
#define MAX_BUF_LEN 2048

class Connection
{
private:
	WSADATA wsaData;

	//Client
	SOCKET connectionSocket = NULL;
	bool connectionLive = false;

	//Server
	SOCKET listenSocket = NULL;
	bool isListening = false;
	bool serverOpen = false;
	vector<SOCKET> sockets;

	void *get_in_addr(struct sockaddr *sa);

public:
	Connection()
	{
		int resultCode;
		if ((resultCode = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
		{
			cout << "WSAStartup failed: " << resultCode << endl;
		}
	}

	~Connection();	
	
	void cleanUp() { WSACleanup(); }

	//Client
	bool connected() const { return connectionLive; }
	void initializeClient() throw (const int);
	void disconnect();
	void sendDataToServer(string data) throw (const int);
	string receiveDataFromServer() throw (const int);

	//Server
	bool listening() const { return isListening; }
	bool isServerOpen() const { return serverOpen; }
	void initializeServer() throw (const int);
	int getClient();
	void stopListening() { closesocket(listenSocket); isListening = false; }
	void shutdownServer();
	void sendDataToClient(string data, SOCKET client) throw (const int);
	string receiveDataFromClient(SOCKET client) throw (const int);

	vector<SOCKET>& getSockets() { return sockets; }
};

#endif