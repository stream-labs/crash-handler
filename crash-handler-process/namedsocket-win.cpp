/******************************************************************************
    Copyright (C) 2016-2019 by Streamlabs (General Workings Inc)
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "namedsocket-win.hpp"
#include <iostream>
#include <mutex>
#include <algorithm>
#include <vector>
#include "logger.hpp"

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define INSTANCES 4 
#define PIPE_TIMEOUT 5000
#define BUFSIZE 128

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
const bool auto_unregister_before_exit = false;
int obs_server_crash_id = 0;
std::vector<std::thread*> requests;

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
	log_info << "DisconnectAndReconnect start for " << i << std::endl;
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

NamedSocket_win::NamedSocket_win() : m_handle(0)
{
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
		memset( &Pipe[i].oOverlap, 0x00, sizeof(OVERLAPPED) );
		Pipe[i].oOverlap.hEvent = hEvents[i];

		Pipe[i].hPipeInst = CreateNamedPipe(
			lpszPipename,
			PIPE_ACCESS_DUPLEX |
			FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT, 
			INSTANCES,
			BUFSIZE,
			BUFSIZE,
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

		Pipe[i].chRequest.resize(BUFSIZE);

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

void acknowledgeUnregister(void) {
	std::string buffer = "exit";
	HANDLE hPipe = CreateFile(
		TEXT("\\\\.\\pipe\\exit-slobs-crash-handler"),
		GENERIC_READ |
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hPipe == INVALID_HANDLE_VALUE)
		return;

	if (GetLastError() == ERROR_PIPE_BUSY)
		return;

	DWORD bytesWritten;

	WriteFile(
		hPipe,
		buffer.data(),
		buffer.size(), &bytesWritten,
		NULL);

	CloseHandle(hPipe);
}

void processRequest(std::vector<char> p_buffer, std::vector<Process*>*  processes, std::mutex* mu, bool* exitApp) {
	Message msg(p_buffer);
	log_info << "processRequest start" << std::endl;
	mu->lock();

	switch (static_cast<Action>(msg.readUInt8())) {
	case Action::REGISTER: {
		bool isCritical = msg.readBool();
		uint32_t pid = msg.readUInt32();
		log_info << "processRequest register " << pid << ", " << isCritical << std::endl;
		auto it = std::find_if(processes->begin(), processes->end(), [&pid](Process* p) {
			return p->getPID() == pid;
		});
		if (it == processes->end()) {
			processes->push_back(new Process(pid, isCritical));
		}
		break;
	}
	case Action::UNREGISTER: {
		uint32_t pid = msg.readUInt32();
		auto it = std::find_if(processes->begin(), processes->end(), [&pid](Process* p) {
			return p->getPID() == pid;
		});
		log_info << "processRequest unregister " << pid <<std::endl ;

		if (it != processes->end()) {
			Process* p = (Process*)(*it);
			p->stopWorker();

			processes->erase(it);

			if (p->getCritical()) {
				log_error << "processRequest failed for critical , set exitApp" << std::endl;
				*exitApp = true;
			}
		}
		acknowledgeUnregister();
		break;
	}
	case Action::EXIT: {
		log_info << "processRequest get EXIT command" << std::endl;
		if(auto_unregister_before_exit) {
			auto it = std::find_if(processes->begin(), processes->end(), [](Process* p) {
				return p->getCritical();
			});
			if (it != processes->end()) {
				Process* p = (Process*)(*it);
				p->stopWorker();

				processes->erase(it);
			}
		}
		*exitApp = true;
		break;
	}
	case Action::CRASH_ID: {
		uint32_t crash_id = msg.readUInt32();
		uint32_t pid = msg.readUInt32();
		obs_server_crash_id = crash_id;
		log_info << "processRequest get CRASH_ID command with id " << crash_id << std::endl;
		break;
	}
	default:
		break;
	}
	mu->unlock();
}

bool NamedSocket_win::read(std::vector<Process*>* processes, std::mutex* mu, bool* exit) {
	DWORD i, dwWait, cbRet = 0, dwErr;
	BOOL fSuccess;

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

	log_info << "NamedSocket_win::read from instance " << i << " pendingIO state "<<  (Pipe[i].fPendingIO? 1:0)<< std::endl;
	if (Pipe[i].fPendingIO)
	{
		fSuccess = GetOverlappedResult(
			Pipe[i].hPipeInst,
			&Pipe[i].oOverlap,
			&cbRet,
			FALSE); 
		dwErr = GetLastError();
		log_debug << "NamedSocket_win::read GetOverlappedResult read " << cbRet << ", dwErr " << dwErr << "\n";
		
		switch (Pipe[i].dwState)
		{
		case CONNECTING_STATE: {
			if (!fSuccess)
			{
				return false;
			}
			Pipe[i].dwState = READING_STATE;
			Pipe[i].cbRead = 0;
			break;
		}
		case READING_STATE: {
			if( !fSuccess && (dwErr == ERROR_IO_PENDING) )
			{
				log_debug << "NamedSocket_win::read pending " << dwErr << "\n" ;
				return false;
			}
			if (!fSuccess || cbRet == 0)
			{
				log_error << "NamedSocket_win::read pending check failed for  " << dwErr << "\n" ;
				DisconnectAndReconnect(i);
				return false;
			}
			Pipe[i].cbRead = cbRet;
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
		if(cbRet == 0)
		{
			fSuccess = ReadFile(
				Pipe[i].hPipeInst,
				Pipe[i].chRequest.data(),
				BUFSIZE,
				&Pipe[i].cbRead,
				&Pipe[i].oOverlap);
			dwErr = GetLastError();
			log_debug << "NamedSocket_win::read read size " << Pipe[i].cbRead << ", fSuccess " << fSuccess << "\n" ;
		}
		
		// The read operation completed successfully. 
		if (Pipe[i].cbRead > 0 && fSuccess) {
			Pipe[i].fPendingIO = FALSE;

			// Start thread here
			requests.push_back(new std::thread(processRequest, Pipe[i].chRequest, processes, mu, exit));
		} else {
			log_error << "NamedSocket_win::read failed with error code " << dwErr << "\n" ;
			if (!fSuccess && (dwErr == ERROR_IO_PENDING))
			{
				Pipe[i].fPendingIO = TRUE;
				return false;
			}
		}
		DisconnectAndReconnect(i);
		break;
	}
	}
	return false;
}

bool NamedSocket_win::flush() {
	return true;
}