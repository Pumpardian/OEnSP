#include <iostream>
#include <chrono>
#include <vector>
#include <bits.h>
#include <Windows.h>

using namespace std;
using namespace chrono;

#define ll long long
#define ld long double
#define vec_ll vector<ll>
#define vec_ld vector<ld>
#define matrix_ll vector<vec_ll>
#define matrix_ld vector<vec_ld>

#define FILETIME_TO_ULL(ft) (((ULONGLONG)(ft).dwHighDateTime << 32) | (ft).dwLowDateTime)

CRITICAL_SECTION criticalSection;
HANDLE consoleHandler;

ll numberOfThreads;
ll matrixSize;

struct ThreadData
{
	ll threadIndex;
	const matrix_ld* A;
	const matrix_ld* B;
	matrix_ld* C;
	ll startRow;
	ll endRow;
	bool finished;
	duration<ld> elapsed;
};

double GetCPUUsage()
{
	static FILETIME prevIdleTime = { 0 }, prevKernelTime = { 0 }, prevUserTime = { 0 };
	FILETIME idleTime, kernelTime, userTime;

	if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
	{
		if (prevIdleTime.dwLowDateTime != 0 || prevIdleTime.dwHighDateTime != 0)
		{
			ULONGLONG idleDelta = FILETIME_TO_ULL(idleTime) - FILETIME_TO_ULL(prevIdleTime);
			ULONGLONG kernelDelta = FILETIME_TO_ULL(kernelTime) - FILETIME_TO_ULL(prevKernelTime);
			ULONGLONG userDelta = FILETIME_TO_ULL(userTime) - FILETIME_TO_ULL(prevUserTime);
			ULONGLONG totalDelta = kernelDelta + userDelta + 1;

			if (totalDelta > 0)
			{
				double cpuUsage = 100.0 * (1.0 - (double)idleDelta / totalDelta);
				prevIdleTime = idleTime;
				prevKernelTime = kernelTime;
				prevUserTime = userTime;
				return cpuUsage;
			}
		}
		else
		{
			prevIdleTime = idleTime;
			prevKernelTime = kernelTime;
			prevUserTime = userTime;
		}
	}
	return 0.0;
}

void UpdateProgress(ll threadIndex, ll progressAmount)
{
	static auto lastCPUCheck = high_resolution_clock::now();
	static double currentCPUUsage;

	auto currentTime = high_resolution_clock::now();
	duration<ld> timeSinceLastCheck = currentTime - lastCPUCheck;

	if (timeSinceLastCheck.count() > 0.2)
	{
		currentCPUUsage = GetCPUUsage();
		lastCPUCheck = currentTime;
	}

	EnterCriticalSection(&criticalSection);

	COORD cursorPosition;
	cursorPosition.X = 0;
	cursorPosition.Y = threadIndex + 1;
	SetConsoleCursorPosition(consoleHandler, cursorPosition);

	cout << "Thread " << threadIndex << ": [";
	ll bars = progressAmount / 2;
	for (ll i = 0; i < 50; ++i)
	{
		if (i < bars)
		{
			cout << "=";
		}
		else
		{
			cout << " ";
		}
	}
	cout << "] " << progressAmount << "%";

	cursorPosition.Y = 0;
	SetConsoleCursorPosition(consoleHandler, cursorPosition);

	cout << " CPU: " << fixed << currentCPUUsage << "%";
	cout << "          " << endl;

	LeaveCriticalSection(&criticalSection);
}

void MultiplyMatricesPart(const matrix_ld& A, const matrix_ld& B, matrix_ld& C, ll startRow, ll endRow, ll threadIndex)
{
	ll matrixSize = A.size();

	for (ll i = startRow; i < endRow; ++i)
	{
		for (ll j = 0; j < matrixSize; ++j)
		{
			C[i][j] = 0;
			for (ll k = 0; k < matrixSize; ++k)
			{
				C[i][j] += A[i][k] * B[k][j];
			}
		}

		ll progress = 100 * (i - startRow + 1) / (endRow - startRow);
		if (progress > 100) progress = 100;

		UpdateProgress(threadIndex, progress);
	}
}

DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
	ThreadData* data = (ThreadData*)lpParam;

	auto startTime = high_resolution_clock::now();
	MultiplyMatricesPart(*data->A, *data->B, *data->C, data->startRow, data->endRow, data->threadIndex);
	auto finishTime = high_resolution_clock::now();

	data->elapsed = finishTime - startTime;
	EnterCriticalSection(&criticalSection);

	COORD cursorPosition;
	cursorPosition.X = 0;
	cursorPosition.Y = data->threadIndex + numberOfThreads + 1;
	SetConsoleCursorPosition(consoleHandler, cursorPosition);

	data->finished = true;
	LeaveCriticalSection(&criticalSection);
	return 0;
}

void UserInput()
{
	do
	{
		cout << "Enter number of threads [1;16]: ";
		cin.clear();
		cin >> numberOfThreads;
	} while (cin.fail() || numberOfThreads < 1 || numberOfThreads > 16);

	do
	{
		cout << "Enter matrix size (n x n) [500;5000]: ";
		cin.clear();
		cin >> matrixSize;
	} while (cin.fail() || matrixSize < 500 || matrixSize > 5000);
}

void InitializeThreads(vector<HANDLE>& threadHandlers, vector<DWORD>& threadIds, vector<ThreadData>& threadData, const matrix_ld& A, const matrix_ld& B, matrix_ld& C)
{
	ll rowsPerThread = matrixSize / numberOfThreads;
	ll remainingRows = matrixSize % numberOfThreads;

	ll currentRow = 0;
	for (ll i = 0; i < numberOfThreads; ++i)
	{
		threadData[i].threadIndex = i;
		threadData[i].A = &A;
		threadData[i].B = &B;
		threadData[i].C = &C;
		threadData[i].startRow = currentRow;
		threadData[i].finished = false;

		ll threadRows = rowsPerThread;
		if (remainingRows > 0)
		{
			++threadRows;
			--remainingRows;
		}

		threadData[i].endRow = currentRow + threadRows;
		currentRow += threadRows;

		threadHandlers[i] = CreateThread(NULL, 0, ThreadFunction, &threadData[i], 0, &threadIds[i]);
	}
}

int main()
{
	UserInput();
	InitializeCriticalSection(&criticalSection);
	consoleHandler = GetStdHandle(STD_OUTPUT_HANDLE);
	system("cls");

	matrix_ld A(matrixSize, vec_ld(matrixSize, 1.0));
	matrix_ld B(matrixSize, vec_ld(matrixSize, 1.0));
	matrix_ld C(matrixSize, vec_ld(matrixSize, 0.0));

	vector<HANDLE> threadHandlers(numberOfThreads);
	vector<DWORD> threadIds(numberOfThreads);
	vector<ThreadData> threadData(numberOfThreads);
	
	InitializeThreads(threadHandlers, threadIds, threadData, A, B, C);

	WaitForMultipleObjects(numberOfThreads, threadHandlers.begin()._Unwrapped(), true, INFINITE);
	do
	{
		Sleep(10);
		for (ll i = 0; i < numberOfThreads; ++i)
		{
			if (!threadData[i].finished)
			{
				continue;
			}
		}
		break;
	} while (true);

	for (ll i = 0; i < numberOfThreads; ++i)
	{
		cout << "Thread " << threadData[i].threadIndex << " finished in: " << threadData[i].elapsed.count() << " seconds" << endl;
		CloseHandle(threadHandlers[i]);
	}
	DeleteCriticalSection(&criticalSection);

	COORD cursorPosition;
	cursorPosition.X = 0;
	cursorPosition.Y = numberOfThreads * 2 + 1;
	SetConsoleCursorPosition(consoleHandler, cursorPosition);
	cout << "All threads has finished their tasks" << endl;
	
	char c;
	cin >> c;
    return 0;
}