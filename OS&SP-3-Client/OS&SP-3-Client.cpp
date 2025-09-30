#include <Windows.h>
#include <iostream>
#include <Bits.h>
#include <vector>
#include <thread>
#include <string>

#define ll long long
#define PIPE_NAME  R"(\\.\pipe\LogPipe)"
#define MESSAGES_COUNT  5

using namespace std;

void ClientThread(ll id)
{
	HANDLE pipeHandler;
	DWORD bytesWritten;

	while (true)
	{
		pipeHandler = CreateFileA(PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (pipeHandler != INVALID_HANDLE_VALUE)
		{
			break;
		}
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			cerr << "Client " << id << ": Unable to connect to the server" << endl;
			return;
		}
		if (!WaitNamedPipeA(PIPE_NAME, 1000))
		{
			cerr << "Client " << id << ": Server is not responding" << endl;
			return;
		}
	}

	for (ll i = 0; i < MESSAGES_COUNT; ++i)
	{
		string message = "Client " + to_string(id) + ": Message " + to_string(i + 1);

		BOOL result = WriteFile(pipeHandler, message.c_str(), static_cast<DWORD>(message.size()), &bytesWritten, NULL);
		if (!result)
		{
			cerr << "Client " << id << ": Error whilst sending message" << endl;
		}
		else
		{
			cout << message << endl;
		}

		this_thread::sleep_for(chrono::milliseconds(100));
	}

	CloseHandle(pipeHandler);
}

void UserInput(ll& input)
{
	do
	{
		cout << "Enter number of clients [1;16]: ";
		cin.clear();
		cin >> input;
	} while (cin.fail() || input < 1 || input > 16);
}

int main()
{
	ll clientCount;
	UserInput(clientCount);

	vector<thread> threads;

	for (ll i = 0; i < clientCount; ++i)
	{
		threads.emplace_back(ClientThread, i + 1);
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	return 0;
}