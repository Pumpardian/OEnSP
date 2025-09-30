#include <Windows.h>
#include <iostream>
#include <Bits.h>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <fstream>

#define PIPE_NAME R"(\\.\pipe\LogPipe)"
#define bufferSize 1024

using namespace std;

bool isRunning = true;
mutex loggerMutex;
ofstream logFile;

string GetCurrentTimestamp()
{
	time_t now = time(NULL);
	tm localTime;
	localtime_s(&localTime, &now);

	char timeString[32];
	strftime(timeString, sizeof(timeString), "%H:%M:%S %Y-%m-%d", &localTime);
	return timeString;
}

void HandleClient(HANDLE pipeHandler)
{
	char readBuffer[bufferSize];
	DWORD bytesRead;

	while (true)
	{
		BOOL isSuccessful = ReadFile(pipeHandler, &readBuffer, sizeof(readBuffer) - 1, &bytesRead, NULL);
		if (!isSuccessful || bytesRead == 0)
		{
			break;
		}
		readBuffer[bytesRead] = '\0';

		string currentTimestamp = GetCurrentTimestamp();

		{
			lock_guard<mutex> lock(loggerMutex);
			logFile << "(" << currentTimestamp << ") " << readBuffer << endl;
			logFile.flush();
		}
		cout << "Message received: " << readBuffer << endl;
	}

	CloseHandle(pipeHandler);
}

int main()
{
	logFile.open("server_log.txt", ios::out);
	if (!logFile.is_open())
	{
		cerr << "Error whilst opening log file";
		return 1;
	}

	HANDLE pipeHandler;
	vector<thread> clientThreads;

	cout << "Awaiting clients...";

	while (isRunning)
	{
		pipeHandler = CreateNamedPipeA(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
			bufferSize, bufferSize, 0, NULL);
		if (pipeHandler == INVALID_HANDLE_VALUE)
		{
			cerr << "Error whilst creating client thread: " << GetLastError() << endl;
			return 1;
		}

		BOOL isConnected = ConnectNamedPipe(pipeHandler, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (isConnected)
		{
			clientThreads.emplace_back(HandleClient, pipeHandler);
		}
		else
		{
			CloseHandle(pipeHandler);
		}
	}

	for (auto& t : clientThreads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	logFile.close();
	return 0;
}