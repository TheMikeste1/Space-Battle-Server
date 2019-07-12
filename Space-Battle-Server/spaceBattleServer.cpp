/*****************************************************************************
* Author:
*    Michael Hegerhorst
* Summary:
*
*
******************************************************************************/
//Input/Output
#include <iostream>

using namespace std;

//Datatypes
#include <vector>
#include <string>
#include <list>

//Threading
#include <thread>

//Networking
#include "connection.h"

//Game
#define MAX_DECAY 40

/*****************************************************************************
* PROTOTYPES
******************************************************************************/
void setupConnections();
vector<thread*> setupThreads();
void sendStart();
void sendToClients();
void shutdownThreads(vector<thread*>);

/*****************************************************************************
* SINGLETONS
******************************************************************************/
Connection server;
vector<string> shipInfo;
list<string> bulletInfo;

/*****************************************************************************
* main()
*	Pilots the program
******************************************************************************/
int main()
{
	cout << "Starting server...\n";
	server.initializeServer();

	cout << "Waiting for connections...\n";
	setupConnections();

	cout << "Got everyone, starting threads...\n";
	vector<thread*> threads = setupThreads();

	//Send GO
	cout << "Sending start...\n";
	sendStart();

	cout << "Game start!\n\n";
	sendToClients();

	//Cleanup
	cout << "Server shutting down...\n";

	if (server.isServerOpen())
		server.shutdownServer();
	server.cleanUp();
	shutdownThreads(threads);

	cout << "Server shut down successfully.\n";

	cin.ignore();

	return 0;
}

void sendStart()
{
	for (int i = 0; i < server.getSockets().size(); i++)
	{
		server.sendDataToClient("G", server.getSockets()[i]);
	}
}

void setupConnections()
{
	SOCKET client = server.getSockets()[server.getClient()];

	server.sendDataToClient("0", client);
	shipInfo.push_back("");

	int numPlayers = 0;

	numPlayers = atoi(server.receiveDataFromClient(client).c_str());

	while (server.getSockets().size() < numPlayers)
	{
		cout << "Waiting on " << numPlayers - server.getSockets().size() << " player(s)...\n";
		int clientNumber = server.getClient();
		client = server.getSockets()[clientNumber];
		server.sendDataToClient(to_string(clientNumber), client);
		server.sendDataToClient(to_string(numPlayers), client);
		shipInfo.push_back("");
	}
}

void sendToClients()
{
	while (server.isServerOpen())
	{
		string data;

		for (int i = 0; i < shipInfo.size(); i++)
			data.append(shipInfo[i]);

		for (list<string>::iterator it = bulletInfo.begin(); it != bulletInfo.end(); ++it)
			data.append(*it);
		int client = 0;
		try
		{
			if (!data.empty())
				for (int i = 0; i < server.getSockets().size(); i++)
				{
					client = i;
					server.sendDataToClient(data, server.getSockets()[i]);
				}

		} catch (const int error)
		{
			cout <<  "Error sending to client " << client << ": " << endl;
			break;
		}

		this_thread::sleep_for(chrono::milliseconds(20));
	}

	cout << "Done sending to clients\n";
}

int extractDecay(string info)
{
	if (info == "")
		return -1;

	int pos1 = info.find(',') + 1;
	int pos2 = info.find(',', pos1);

	return atoi(info.substr(pos1, pos2 - pos1).c_str());
}

string extractID(string info)
{
	if (info == "")
		return "";

	int pos = info.find(',');

	return info.substr(2, pos - 2);
}

void parseData(string data)
{
	if (data == "")
		return;
	//Pull ship data
	int ship = atoi(&data[1]);

	string* shipString = &shipInfo[ship];

	int pos1 = 3;
	int pos2 = data.find('|', pos1);

	pos1 = pos2 + 1;

	*shipString = data.substr(0, pos2 + 1);

	//Pull bullet data
	while (pos2 < data.length() - 3)
	{
		pos2 += 3;
		pos2 = data.find('|', pos2);

		string bullet = data.substr(pos1, pos2 - pos1 + 1);

		int decay = extractDecay(bullet);
		string id = extractID(bullet);		

		bool found = false;
		for (list<string>::iterator it = bulletInfo.begin(); it != bulletInfo.end(); ++it)
		{
			if (extractID(*it) == id)
			{
				if (extractDecay(*it) < decay)
				{
					if (decay > MAX_DECAY)
						bulletInfo.remove(*it);
					else
						*it = bullet; 
				}

				found = true;
				break;
			}
		}

		if (!found)
		{
			bulletInfo.push_back(bullet);
		}

		pos1 = pos2 + 1;
	}
}

void receiveFromClient(SOCKET client)
{
	while (server.isServerOpen())
	{
		string data = "";

		try
		{
			data = server.receiveDataFromClient(client);
		} catch (const int error)
		{
			cout << "Error receiving from client " << client
			<< ": " << error << endl;
		}

		if (data == "G")
		{
			for (int i = 0; i < server.getSockets().size(); i++)
			{
				server.sendDataToClient("G", server.getSockets()[i]);
			}

			server.shutdownServer();
		}

		if (data.empty())
			break;

		parseData(data);
	}

	cout << "Done receiving from client " << client << endl;
}

vector<thread*> setupThreads()
{
	vector<thread*> threads;
	vector<SOCKET> sockets = server.getSockets();

	for (int i = 0; i < sockets.size(); i++)
	{
		thread* newThread = new thread(receiveFromClient, sockets[i]);
		threads.push_back(newThread);
	}

	return threads;
}

void shutdownThreads(vector<thread*> threads)
{
	for (int i = 0; i < threads.size(); i++)
	{
		thread* thrd = threads[i];

		if (thrd != NULL)
		{
			thrd->join();
			delete thrd;
			thrd = NULL;
		}
	}
}