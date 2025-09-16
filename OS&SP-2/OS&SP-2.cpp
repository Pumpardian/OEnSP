#include <bits.h>
#include <windows.h>
#include <chrono>
#include <string>
#include <iostream>

#define ll long long
#define ld long double

using namespace std;
using namespace chrono;

const ll RECORD_SIZE = 256;

struct Record
{
    ll id;
    char name[248];
};

class Database
{
    string filename_;
    ll recordCount_;
    HANDLE hFile_ = INVALID_HANDLE_VALUE;
    HANDLE hMap_ = NULL;
    char* data_ = NULL;

public:
    Database(const string& filename, ll recordCount) : filename_(filename), recordCount_(recordCount)
    {
        hFile_ = CreateFileA(filename_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        LARGE_INTEGER fileSize;
        fileSize.QuadPart = recordCount_ * RECORD_SIZE;
        SetFilePointerEx(hFile_, fileSize, NULL, FILE_BEGIN);
        SetEndOfFile(hFile_);

        hMap_ = CreateFileMappingA(hFile_, NULL, PAGE_READWRITE, 0, 0, NULL);
        data_ = static_cast<char*>(MapViewOfFile(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    }

    ~Database()
    {
        if (data_)
        {
            UnmapViewOfFile(data_);
        }
        if (hMap_)
        {
            CloseHandle(hMap_);
        }
        if (hFile_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile_);
        }
    }

    void writeRecord(ll index, const Record& record)
    {
        char* ptr = data_ + index * RECORD_SIZE;
        memcpy(ptr, &record, sizeof(Record));
    }

    Record readRecord(ll index) const
    {
        Record record;
        const char* ptr = data_ + index * RECORD_SIZE;
        memcpy(&record, ptr, sizeof(Record));
        return record;
    }
};

class SynchronousDatabase
{
    string filename_;
    ll recordCount_;
    HANDLE hFile_ = INVALID_HANDLE_VALUE;

public:
    SynchronousDatabase(const string& filename, ll recordCount) : filename_(filename), recordCount_(recordCount)
    {
        hFile_ = CreateFileA(filename_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        LARGE_INTEGER fileSize;
        fileSize.QuadPart = recordCount_ * RECORD_SIZE;
        SetFilePointerEx(hFile_, fileSize, NULL, FILE_BEGIN);
        SetEndOfFile(hFile_);
    }

    ~SynchronousDatabase()
    {
        if (hFile_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile_);
        }
    }

    void writeRecord(ll index, const Record& record)
    {
        LARGE_INTEGER offset;
        offset.QuadPart = index * RECORD_SIZE;
        SetFilePointerEx(hFile_, offset, NULL, FILE_BEGIN);
        WriteFile(hFile_, &record, sizeof(Record), NULL, NULL);
    }

    Record readRecord(ll index) const
    {
        Record record;
        LARGE_INTEGER offset;
        offset.QuadPart = index * RECORD_SIZE;
        SetFilePointerEx(hFile_, offset, NULL, FILE_BEGIN);
        ReadFile(hFile_, &record, sizeof(Record), NULL, NULL);
        return record;
    }
};

Record createRecord(ll id, const string& name)
{
    Record record;
    record.id = id;
    memset(record.name, 0, sizeof(record.name));
    strncpy_s(record.name, name.c_str(), sizeof(record.name) - 1);
    return record;
}

int main()
{
    system("cls");

    const string filename = "database.dat";
    const ll recordCount = 10000000;

    Database db(filename, recordCount);

    auto start = high_resolution_clock::now();
    for (ll i = 0; i < recordCount; ++i)
    {
        db.writeRecord(i, createRecord(static_cast<ll>(i), "Name_" + to_string(i)));
    }
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    cout << "Write time using memory (MMT): " << duration.count() << " seconds." << endl;

    start = high_resolution_clock::now();
    for (ll i = 0; i < recordCount; ++i)
    {
        Record r = db.readRecord(i);
    }
    end = high_resolution_clock::now();
    duration = end - start;
    cout << "Read time using memory (MMT): " << duration.count() << " seconds." << endl;

    SynchronousDatabase dbsync(filename, recordCount);

    start = high_resolution_clock::now();
    for (ll i = 0; i < recordCount; ++i)
    {
        dbsync.writeRecord(i, createRecord(static_cast<ll>(i), "Name_" + to_string(i)));
    }
    end = high_resolution_clock::now();
    duration = end - start;
    cout << "Synchromous write time: " << duration.count() << " seconds." << endl;

    start = high_resolution_clock::now();
    for (ll i = 0; i < recordCount; ++i)
    {
        Record r = dbsync.readRecord(i);
    }
    end = high_resolution_clock::now();
    duration = end - start;
    cout << "Synchronous read time: " << duration.count() << " seconds." << endl;

    return 0;
}