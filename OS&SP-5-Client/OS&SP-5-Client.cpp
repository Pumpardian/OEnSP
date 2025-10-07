#include <iostream>
#include <string>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 5555
#define BUFFER_SIZE 1024

using namespace std;

struct ChatMessage
{
    char sender[32];
    char receiver[32];
    char message[896];
};

SOCKET clientSocket;
bool connected = false;

void ReceiveMessages()
{
    char buffer[BUFFER_SIZE];
    ChatMessage message;

    while (connected)
    {
        int bytesReceived = recv(clientSocket, (char*)&message, sizeof(ChatMessage), 0);

        if (bytesReceived > 0)
        {
            if (strlen(message.sender) > 0)
            {
                cout << "\n[" << message.sender << " -> " << message.receiver << "]: " << message.message << endl;
            }
            else
            {
                buffer[bytesReceived] = '\0';
                cout << "\n" << buffer << endl;
            }

            cout << "Enter message (format: <receiver> <message>): ";
            cout.flush();
        }
        else if (bytesReceived == 0)
        {
            cout << "Connection lost" << endl;
            connected = false;
            break;
        }
        else
        {
            break;
        }
    }
}

int main()
{
    WSADATA wsaData;
    sockaddr_in serverAddress;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "Error while initializing Winsock" << endl;
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        cerr << "Error while creating socket" << endl;
        WSACleanup();
        return 1;
    }

    if (InetPtonA(AF_INET, SERVER_IP, &serverAddress.sin_addr) != 1)
    {
        std::cerr << "Invalid server address: " << SERVER_IP << std::endl;
        WSACleanup();
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        cerr << "Error while connecting to the server" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    connected = true;

    string username;
    cout << "Enter username: ";
    getline(cin, username);

    send(clientSocket, username.c_str(), username.length(), 0);

    cout << "Connected!" << endl;
    cout << "Message format: <receiver> <message>" << endl;
    cout << "To send to everyone: ALL <message>" << endl;
    cout << "To exit type: quit" << endl << endl;

    thread receiveThread(ReceiveMessages);

    string input;
    while (connected)
    {
        cout << "Enter message (format: <receiver> <message>): ";
        getline(cin, input);

        if (input == "quit")
        {
            break;
        }

        size_t spacePos = input.find(' ');
        if (spacePos != string::npos)
        {
            string to = input.substr(0, spacePos);
            string message = input.substr(spacePos + 1);

            ChatMessage msg;
            strcpy_s(msg.sender, username.c_str());
            strcpy_s(msg.receiver, to.c_str());
            strcpy_s(msg.message, message.c_str());

            send(clientSocket, (char*)&msg, sizeof(ChatMessage), 0);
        }
        else
        {
            cout << "Invalid format. Use: <receiver> <message>" << endl;
        }
    }

    connected = false;
    closesocket(clientSocket);
    receiveThread.join();
    WSACleanup();
    return 0;
}