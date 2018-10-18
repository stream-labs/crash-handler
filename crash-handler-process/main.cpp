#include <windows.h> 
#include <tchar.h>
#include "message.hpp"
#include "process.hpp"
#include <iostream>
#include <algorithm>

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define INSTANCES 4 
#define PIPE_TIMEOUT 5000
#define BUFSIZE 4096

typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipeInst;
	std::vector<char> chRequest;
	DWORD cbRead;
	TCHAR chReply[BUFSIZE];
	DWORD cbToWrite;
	DWORD dwState;
	BOOL fPendingIO;
} PIPEINST, *LPPIPEINST;


VOID DisconnectAndReconnect(DWORD);
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED);

PIPEINST Pipe[INSTANCES];
HANDLE hEvents[INSTANCES];

std::vector<Process*> processes;
std::mutex mutex;
bool exitApp = false;
bool doRestartApp = false;

void checkProcesses(void) {
	bool stop = false;

	while (!stop && !exitApp) {
		bool alive = true;
		size_t index = 0;

		while (alive && index < processes.size()) {
			std::unique_lock<std::mutex> ulock(processes.at(index)->mutex);
			alive = processes.at(index)->getAlive();
			index++;
		}
		if (!alive) {
			index--;
			if (!processes.at(index)->getCritical()) {
				int code = MessageBox(
					NULL,
					"An issue occured, don't worry you are still streaming/recording. \n\nChoose whenever you want to restart the application by clicking the \"OK\" button.",
					"Oops...",
					MB_OK | MB_SYSTEMMODAL
				);
				doRestartApp = true;
			}
			stop = true;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
	std::cout << "We should exit the app" << std::endl;
	exitApp = true;
}

void close(void) {
	for (size_t i = 0; i < processes.size(); i++) {
		processes.at(i)->stopWorker();
	}
	for (size_t i = 0; i < processes.size(); i++) {
		processes.at(i)->getWorker()->join();
		if (processes.at(i)->getAlive()) {
			HANDLE hdl = OpenProcess(PROCESS_TERMINATE, FALSE, processes.at(i)->getPID());
			TerminateProcess(hdl, 1);
		}
	}
}

int _tmain(VOID)
{
	DWORD i, dwWait, cbRet, dwErr;
	BOOL fSuccess;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\slobs-crash-handler");

	// The initial loop creates several instances of a named pipe 
	// along with an event object for each instance.  An 
	// overlapped ConnectNamedPipe operation is started for 
	// each instance. 

	std::thread processManager(checkProcesses);

	for (i = 0; i < INSTANCES; i++)
	{

		// Create an event object for this instance. 

		hEvents[i] = CreateEvent(
			NULL,    // default security attribute 
			TRUE,    // manual-reset event 
			TRUE,    // initial state = signaled 
			NULL);   // unnamed event object 

		if (hEvents[i] == NULL)
		{
			printf("CreateEvent failed with %d.\n", GetLastError());
			return 0;
		}

		Pipe[i].oOverlap.hEvent = hEvents[i];

		Pipe[i].hPipeInst = CreateNamedPipe(
			lpszPipename,            // pipe name 
			PIPE_ACCESS_DUPLEX |     // read/write access 
			FILE_FLAG_OVERLAPPED,    // overlapped mode 
			PIPE_TYPE_MESSAGE |      // message-type pipe 
			PIPE_READMODE_MESSAGE |  // message-read mode 
			PIPE_WAIT,               // blocking mode 
			INSTANCES,               // number of instances 
			BUFSIZE * sizeof(TCHAR),   // output buffer size 
			BUFSIZE * sizeof(TCHAR),   // input buffer size 
			PIPE_TIMEOUT,            // client time-out 
			NULL);                   // default security attributes 

		if (Pipe[i].hPipeInst == INVALID_HANDLE_VALUE)
		{
			printf("CreateNamedPipe failed with %d.\n", GetLastError());
			return 0;
		}

		// Call the subroutine to connect to the new client

		Pipe[i].fPendingIO = ConnectToNewClient(
			Pipe[i].hPipeInst,
			&(Pipe[i].oOverlap));

		Pipe[i].dwState = Pipe[i].fPendingIO ?
			CONNECTING_STATE : // still connecting 
			READING_STATE;     // ready to read 
	}

	while (!exitApp)
	{
		// Wait for the event object to be signaled, indicating 
		// completion of an overlapped read, write, or 
		// connect operation. 
		dwWait = WaitForMultipleObjects(
			INSTANCES,    // number of event objects 
			hEvents,      // array of event objects 
			FALSE,        // does not wait for all 
			500);		  // waits 500 ms 

						  // dwWait shows which pipe completed the operation. 

		i = dwWait - WAIT_OBJECT_0;  // determines which pipe 
		if (i < 0 || i >(INSTANCES - 1))
		{
			// printf("Index out of range.\n");
			continue;
		}

		// Get the result if the operation was pending. 

		if (Pipe[i].fPendingIO)
		{
			fSuccess = GetOverlappedResult(
				Pipe[i].hPipeInst, // handle to pipe 
				&(Pipe[i].oOverlap), // OVERLAPPED structure 
				&cbRet,            // bytes transferred 
				FALSE);            // do not wait 

			switch (Pipe[i].dwState)
			{
				// Pending connect operation 
			case CONNECTING_STATE:
				if (!fSuccess)
				{
					printf("Error %d.\n", GetLastError());
					return 0;
				}
				Pipe[i].dwState = READING_STATE;
				break;

				// Pending read operation 
			case READING_STATE:
				if (!fSuccess || cbRet == 0)
				{
					DisconnectAndReconnect(i);
					continue;
				}
				Pipe[i].cbRead = cbRet;
				Pipe[i].dwState = READING_STATE;
				break;

			default:
			{
				printf("Invalid pipe state.\n");
				return 0;
			}
			}
		}

		// The pipe state determines which operation to do next. 

		switch (Pipe[i].dwState)
		{
			// READING_STATE: 
			// The pipe instance is connected to the client 
			// and is ready to read a request from the client. 

		case READING_STATE:
			Pipe[i].chRequest.resize(BUFSIZE);
			fSuccess = ReadFile(
				Pipe[i].hPipeInst,
				Pipe[i].chRequest.data(),
				BUFSIZE * sizeof(TCHAR),
				&Pipe[i].cbRead,
				&Pipe[i].oOverlap);

			GetOverlappedResult(
				Pipe[i].hPipeInst,
				&Pipe[i].oOverlap,
				&Pipe[i].cbRead,
				true
			);

			// The read operation completed successfully. 
			if (Pipe[i].cbRead > 0) {
				Pipe[i].fPendingIO = FALSE;
				Message msg(Pipe[i].chRequest);

				switch (msg.readBool()) {
				case REGISTER: {
					std::unique_lock<std::mutex> ulock(mutex);

					bool isCritical = msg.readBool();
					uint32_t pid = msg.readUInt32();

					std::cout << "Registering " << pid << ", critical : " << isCritical << std::endl;

					processes.push_back(new Process(pid, isCritical));
					break;
				}
				case UNREGISTER: {
					uint32_t pid = msg.readUInt32();
					auto it = std::find_if(processes.begin(), processes.end(), [&pid](Process* p) {
						return p->getPID() == pid;
					});

					if (it != processes.end()) {
						std::unique_lock<std::mutex> ulock(mutex);

						std::cout << "Unregistering " << pid << std::endl;

						Process* p = (Process*)(*it);
						p->stopWorker();
						if (p->getWorker()->joinable())
							p->getWorker()->join();

						processes.erase(it);
					}
					break;
				}
				case EXIT: {
					std::cout << "Exit received by main process" << std::endl;
					exitApp = true;
					break;
				}
				default: {
					// Wrong value
					exitApp = true;
					break;
				}
				}
			}
			
			// The read operation is still pending. 

			dwErr = GetLastError();
			if (!fSuccess && (dwErr == ERROR_IO_PENDING))
			{
				Pipe[i].fPendingIO = TRUE;
				continue;
			}

			// An error occurred; disconnect from the client. 

			DisconnectAndReconnect(i);
			break;


		default:
		{
			printf("Invalid pipe state.\n");
			return 0;
		}
		}
		printf("Loop\n");
	}

	close();
	processManager.join();

	if (doRestartApp) {
		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;
		
		memset(&info, 0, sizeof(info));
		memset(&processInfo, 0, sizeof(processInfo));

		CreateProcess("..\\Streamlabs OBS.exe",
			"",
			NULL,
			NULL,
			FALSE,
			DETACHED_PROCESS,
			NULL,
			NULL,
			&info,
			&processInfo
		 );
	}

	return 0;
}


// DisconnectAndReconnect(DWORD) 
// This function is called when an error occurs or when the client 
// closes its handle to the pipe. Disconnect from this client, then 
// call ConnectNamedPipe to wait for another client to connect. 

VOID DisconnectAndReconnect(DWORD i)
{
	// Disconnect the pipe instance. 

	if (!DisconnectNamedPipe(Pipe[i].hPipeInst))
	{
		printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
	}

	// Call a subroutine to connect to the new client. 

	Pipe[i].fPendingIO = ConnectToNewClient(
		Pipe[i].hPipeInst,
		&(Pipe[i].oOverlap));

	Pipe[i].dwState = Pipe[i].fPendingIO ?
		CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read 
}

// ConnectToNewClient(HANDLE, LPOVERLAPPED) 
// This function is called to start an overlapped connect operation. 
// It returns TRUE if an operation is pending or FALSE if the 
// connection has been completed. 

BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, lpo);

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected)
	{
		printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;

		// Client is already connected, so signal an event. 

	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;

		// If an error occurs during the connect operation... 
	default:
	{
		printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}
	}

	return fPendingIO;
}

