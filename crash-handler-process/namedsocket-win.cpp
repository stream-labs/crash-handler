#include "namedsocket-win.hpp"
#include <iostream>
#include <mutex>
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

PIPEINST Pipe[INSTANCES];
HANDLE hEvents[INSTANCES];

BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;
	fConnected = ConnectNamedPipe(hPipe, lpo);

	if (fConnected)
	{
		return 0;
	}

	switch (GetLastError())
	{
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;
	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;
	default:
	{
		return 0;
	}
	}

	return fPendingIO;
}

VOID DisconnectAndReconnect(DWORD i)
{
	if (!DisconnectNamedPipe(Pipe[i].hPipeInst))
	{
		return;
	}
	
	Pipe[i].fPendingIO = ConnectToNewClient(
		Pipe[i].hPipeInst,
		&(Pipe[i].oOverlap));

	Pipe[i].dwState = Pipe[i].fPendingIO ?
		CONNECTING_STATE :
		READING_STATE;
}

NamedSocket_win::NamedSocket_win() {
	BOOL fSuccess;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\slobs-crash-handler");

	for (int i = 0; i < INSTANCES; i++)
	{
		hEvents[i] = CreateEvent(
			NULL,
			TRUE,
			TRUE,
			NULL);

		if (hEvents[i] == NULL)
		{
			return;
		}

		Pipe[i].oOverlap.hEvent = hEvents[i];

		Pipe[i].hPipeInst = CreateNamedPipe(
			lpszPipename,
			PIPE_ACCESS_DUPLEX |
			FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT, 
			INSTANCES,
			BUFSIZE * sizeof(TCHAR),
			BUFSIZE * sizeof(TCHAR),
			PIPE_TIMEOUT, 
			NULL);

		if (Pipe[i].hPipeInst == INVALID_HANDLE_VALUE)
		{
			return;
		}
		
		Pipe[i].fPendingIO = ConnectToNewClient(
			Pipe[i].hPipeInst,
			&(Pipe[i].oOverlap));

		Pipe[i].dwState = Pipe[i].fPendingIO ?
			CONNECTING_STATE : 
			READING_STATE;
	}
}

std::unique_ptr<NamedSocket> NamedSocket::create() {
	return std::make_unique<NamedSocket_win>();
}

bool NamedSocket_win::connect() {
	return ConnectNamedPipe(m_handle, NULL);
}

HANDLE NamedSocket_win::getHandle() {
	return m_handle;
}

void NamedSocket_win::disconnect() {
	CloseHandle(m_handle);
}

NamedSocket_win::~NamedSocket_win() {
	DisconnectNamedPipe(m_handle);
}

bool NamedSocket_win::read(std::vector<Process*>* processes) {
	DWORD i, dwWait, cbRet, dwErr;
	BOOL fSuccess;
	bool exitApp = false;

	dwWait = WaitForMultipleObjects(
		INSTANCES,
		hEvents,
		FALSE,
		500);

	i = dwWait - WAIT_OBJECT_0;  // get current pipe
	if (i < 0 || i >(INSTANCES - 1))
	{
		return false;
	}
	
	if (Pipe[i].fPendingIO)
	{
		fSuccess = GetOverlappedResult(
			Pipe[i].hPipeInst,
			&(Pipe[i].oOverlap),
			&cbRet,
			FALSE); 

		switch (Pipe[i].dwState)
		{
		case CONNECTING_STATE: {
			if (!fSuccess)
			{
				return false;
			}
			Pipe[i].dwState = READING_STATE;
			break;
		}
		case READING_STATE: {
			if (!fSuccess || cbRet == 0)
			{
				DisconnectAndReconnect(i);
				return false;
			}
			Pipe[i].cbRead = cbRet;
			Pipe[i].dwState = READING_STATE;
			break;
		}
		default:
		{
			return  false;
		}
		}
	}
	
	switch (Pipe[i].dwState)
	{
	case READING_STATE: {
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
				bool isCritical = msg.readBool();
				uint32_t pid = msg.readUInt32();
				processes->push_back(new Process(pid, isCritical));
				break;
			}
			case UNREGISTER: {
				uint32_t pid = msg.readUInt32();
				auto it = std::find_if(processes->begin(), processes->end(), [&pid](Process* p) {
					return p->getPID() == pid;
				});

				if (it != processes->end()) {
					Process* p = (Process*)(*it);
					p->stopWorker();
					processes->erase(it);

					if (p->getCritical()) {
						exitApp = true;
					}
				}
				break;
			}
			case EXIT:
				exitApp = true;
			default:
				break;
			}
		}
		dwErr = GetLastError();
		if (!fSuccess && (dwErr == ERROR_IO_PENDING))
		{
			Pipe[i].fPendingIO = TRUE;
			return false;
		}
		DisconnectAndReconnect(i);
		break;
	}
	}
	return exitApp;
}

bool NamedSocket_win::flush() {
	return true;
}