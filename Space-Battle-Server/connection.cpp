/*********************************************************************
 * File: game.cpp
 * Description: Contains the implementaiton of the game class
 *  methods.
 *
 *********************************************************************/

#include "connection.h"

/*********************************************
* Constructors
*********************************************/
Connection::~Connection()
{
	// 5. Disconnect.
	//https://docs.microsoft.com/en-us/windows/win32/winsock/disconnecting-the-client
	int resultCode;
	if ((resultCode = shutdown(connectionSocket, SD_SEND)) == SOCKET_ERROR) 
	{
	    printf("shutdown failed: %d\n", WSAGetLastError());
	    closesocket(connectionSocket);
	    WSACleanup();
	    return;
	}

	//Cleanup
	closesocket(connectionSocket);
	WSACleanup();
}

/*********************************************
* Public
*********************************************/
/*********************************************
* Misc
*********************************************/
/*****************************************************************************
* get_in_addr()
*	Returns the IP address of a connection.
******************************************************************************/
void* Connection::get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET)
   {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*********************************************************************
 * initialize()
 * Starts a connection with an entered IP.
 *********************************************************************/
void Connection::initializeClient() throw (const int)
{
	int resultCode;
	// 2. Create a socket.
	//https://docs.microsoft.com/en-us/windows/win32/winsock/creating-a-socket-for-the-client
	struct addrinfo *servinfo = NULL;
	struct addrinfo hints;

	PCSTR port = DEFAULT_PORT; //PCSTR = const char* (Pointer Constant STRing)

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	string servIP;
	cout << "Please enter an IP: ";
	cin >> servIP;


	if ((resultCode = getaddrinfo(servIP.c_str(), port, &hints, &servinfo) != 0))
	{
		printf("getaddrinfo failed: %d\n", resultCode);
	    WSACleanup();
		throw 1;
	}

	connectionSocket = socket(servinfo->ai_family, 
								servinfo->ai_socktype, 
									servinfo->ai_protocol);

	if (connectionSocket == INVALID_SOCKET) 
	{
	    printf("Error at socket(): %ld\n", WSAGetLastError());
	    freeaddrinfo(servinfo);
	    WSACleanup();
	    throw 1;
	}

	// 3. Connect to the server.
	//https://docs.microsoft.com/en-us/windows/win32/winsock/connecting-to-a-socket
	if ((resultCode = connect(connectionSocket, servinfo->ai_addr, (int)servinfo->ai_addrlen)) == SOCKET_ERROR)
	{
		closesocket(connectionSocket);
		connectionSocket = INVALID_SOCKET;
	}

	freeaddrinfo(servinfo);

	if (connectionSocket == INVALID_SOCKET)
	{
		cout << "Failed to connect.\n";
		WSACleanup();
		throw 1;
	}

	connectionLive = true;
}

void Connection::initializeServer() throw(const int)
{
	int resultCode;
	// 2. Create a socket.
	//https://docs.microsoft.com/en-us/windows/desktop/winsock/creating-a-socket-for-the-server
	struct addrinfo *servinfo = NULL;
	struct addrinfo hints;

	PCSTR port = DEFAULT_PORT; //PCSTR = const char* (Pointer Constant STRing)

	ZeroMemory(&hints, sizeof(hints)); //This macro fills hints with zeroes.
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags    = AI_PASSIVE;

	//2.1. The getaddrinfo function is used to determine the values
	//   in the sockaddr structure:
	if ((resultCode = getaddrinfo(NULL, port, &hints, &servinfo))
		!= 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(resultCode));
		WSACleanup();
		throw 1;
	}
	
	//2.2. Create a SOCKET object called listenSocket for the server
	//   to listen for client connections.
	//2.3. Call the socket function and return its value to
	//   the listenSocket variable.
	listenSocket = socket(servinfo->ai_family,
					  servinfo->ai_socktype,
						  servinfo->ai_protocol);
	//SOCKET = unsigned integer pointer

	//2.4. Check for errors.
	if (listenSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(servinfo);
		WSACleanup();
		throw 1;
	}

	//Set up listenSocket to be used multiple times
	char yes = '1';
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(yes)) == -1)
      {
         cout << "Error setting up socket options.\n";
         throw 1;
      }

	// 3. Bind the socket.
	//https://docs.microsoft.com/en-us/windows/desktop/winsock/binding-a-socket
	if ((resultCode = bind(listenSocket, servinfo->ai_addr, servinfo->ai_addrlen))
		== SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(servinfo);
        closesocket(listenSocket);
        WSACleanup();
        throw 1;
	}

	freeaddrinfo(servinfo); //"all done with this structure" -rpsServer.cpp

	// 4. Listen on the socket.
	//https://docs.microsoft.com/en-us/windows/desktop/winsock/listening-on-a-socket
	if (listen(listenSocket, BACKLOG) == SOCKET_ERROR)
	{
		printf( "Listen failed with error: %ld\n", WSAGetLastError() );
		closesocket(listenSocket);
	    WSACleanup();
	    throw 1;
	}

	isListening = true;
	serverOpen = true;
}

int Connection::getClient()
{
	SOCKET client;
	//This is to gather information about the client's IP.
	struct sockaddr_storage their_address;
	socklen_t size = sizeof their_address;
	char s[INET6_ADDRSTRLEN];

	if ((client = accept(listenSocket, (struct sockaddr *) &their_address, &size))
		== INVALID_SOCKET)
	{
		printf("accept failed: %d\n", WSAGetLastError());
	    closesocket(listenSocket);
	    WSACleanup();
	    throw "1";
	}

	//This sets s to the IP of the client
	inet_ntop(their_address.ss_family,
      get_in_addr((struct sockaddr *)&their_address),
      s, sizeof s);

	cout << "Got connection from " << s << endl;

	sockets.push_back(client);

	return sockets.size() - 1;
}

void Connection::disconnect()
{
	// 5. Disconnect.
	//https://docs.microsoft.com/en-us/windows/win32/winsock/disconnecting-the-client
	int resultCode;
	if ((resultCode = shutdown(connectionSocket, SD_SEND)) == SOCKET_ERROR) 
	{
	    printf("Disconnect failed: %d\n", WSAGetLastError());
	    closesocket(connectionSocket);
	    return;
	}

	connectionLive = false;
}

void Connection::shutdownServer()
{
	if (isListening)
	{
		closesocket(listenSocket);
		isListening = false;
	}

	for (int i = 0; i < sockets.size(); i++)
	{
		closesocket(sockets[i]);
	}

	serverOpen = false;
}

void Connection::sendDataToServer(string data) throw (const int)
{
	if (!connected())
		return;
	// 4. Send and receive data.
	//https://docs.microsoft.com/en-us/windows/win32/winsock/sending-and-receiving-data-on-the-client
	int resultCode;
	if ((resultCode = send(connectionSocket, data.c_str(), data.size(), 0))
			== SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(connectionSocket);
		WSACleanup();
		throw 1;	
	}
}

string Connection::receiveDataFromServer() throw (const int)
{
	if (!connected())
		return "";

	char incomingData[MAX_BUF_LEN] = {};
	
	int resultCode = recv(connectionSocket, incomingData, MAX_BUF_LEN - 1, 0);

	string data(incomingData);

	if (resultCode > 0)
		cout << "Received " << resultCode << " bytes: " << data << endl;

	return data;
}


void Connection::sendDataToClient(string data, SOCKET client) throw(const int)
{
	int resultCode;
	if ((resultCode = send(client, data.c_str(), data.size(), 0))
			== SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(client);
		WSACleanup();
		throw 1;	
	}
}

string Connection::receiveDataFromClient(SOCKET client) throw(const int)
{
	char incomingData[MAX_BUF_LEN] = {};
	
	int resultCode = recv(client, incomingData, 
		               MAX_BUF_LEN - 1, 0);

	string data(incomingData);

	return data;
}