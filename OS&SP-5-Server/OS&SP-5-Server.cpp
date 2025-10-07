#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 5555
#define BUFFER_SIZE 1024

using namespace std;

struct ChatMessage
{
	char sender[32];
	char receiver[32];
	char message[896];
};

struct Client
{
	SOCKET socket;
	sockaddr_in address;
	string username;
};

vector<Client> connectedClients;
mutex clientsMutex;

string GetClientIP(sockaddr_in clientAddr)
{
	char ipString[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(clientAddr.sin_addr), ipString, INET_ADDRSTRLEN);
	return string(ipString);
}

void HandleClient(SOCKET clientSocket, sockaddr_in clientAddress)
{
	char buffer[BUFFER_SIZE];
	ChatMessage message;

	ChatMessage systemMsg;
	strcpy_s(systemMsg.sender, "SERVER");
	strcpy_s(systemMsg.receiver, "ALL");

	int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
	if (bytesReceived > 0)
	{
		buffer[bytesReceived] = '\0';
		string username = buffer;

		Client newClient;
		newClient.socket = clientSocket;
		newClient.address = clientAddress;
		newClient.username = username;

		{
			lock_guard<mutex> lock(clientsMutex);
			connectedClients.push_back(newClient);
		}

		cout << "User " << username << " has connected to the chat" << endl;

		strcpy_s(systemMsg.message, (username + " connected to the chat").c_str());
		{
			lock_guard<mutex> lock(clientsMutex);
			for (auto& client : connectedClients)
			{
				if (client.socket != clientSocket)
				{
					send(client.socket, (char*)&systemMsg, sizeof(ChatMessage), 0);
				}
			}
		}
	}

	while (true)
	{
		bytesReceived = recv(clientSocket, (char*)&message, sizeof(ChatMessage), 0);
		if (bytesReceived <= 0)
		{
			cout << "Client has disconnected" << endl;
			break;
		}

		cout << "Message from " << message.sender << " to " << message.receiver << ": " << message.message << endl;

		if (strcmp(message.receiver, "ALL") == 0)
		{
			lock_guard<mutex> lock(clientsMutex);
			for (auto& client : connectedClients)
			{
				if (client.socket != clientSocket)
				{
					send(client.socket, (char*)&message, sizeof(ChatMessage), 0);
				}
			}
		}
		else
		{
			lock_guard<mutex> lock(clientsMutex);
			for (auto& client : connectedClients)
			{
				if (client.username == message.receiver)
				{
					send(client.socket, (char*)&message, sizeof(ChatMessage), 0);
					break;
				}
			}
		}
	}

	{
		lock_guard<mutex> lock(clientsMutex);
		for (auto client = connectedClients.begin(); client != connectedClients.end(); ++client)
		{
			if (client->socket == clientSocket)
			{
				strcpy_s(systemMsg.message, (client->username + " has left the chat").c_str());
				for (auto& c : connectedClients)
				{
					if (c.socket != clientSocket)
					{
						send(c.socket, (char*)&systemMsg, sizeof(ChatMessage), 0);
					}
				}

				connectedClients.erase(client);
				break;
			}
		}
	}

	closesocket(clientSocket);
}

int main()
{
	WSADATA wsaData;
	SOCKET serverSocket, clientSocket;
	sockaddr_in serverAddress, clientAddress;
	int clientAddressSize = sizeof(clientAddress);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cerr << "Error while initializing Winsock" << endl;
		return 1;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "Error while creating socket" << endl;
		WSACleanup();
		return 1;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddress.sin_port = htons(PORT);

	if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "Error while binding socket" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "Error while listening" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	cout << "Server started. using Port: " << PORT << endl;
	cout << "Awaiting connections..." << endl;

	while (true)
	{
		clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "Error while accepting connection" << endl;
			continue;
		}

		cout << "New connection from " << GetClientIP(clientAddress) << endl;

		thread clientThread(HandleClient, clientSocket, clientAddress);
		clientThread.detach();
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}