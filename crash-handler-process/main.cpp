#include <windows.h> 
#include <tchar.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include "namedsocket-win.hpp"

VOID DisconnectAndReconnect(DWORD);
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED);

std::vector<Process*> processes;
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
	exitApp = true;
}

void close(void) {
	for (size_t i = 0; i < processes.size(); i++) {
		processes.at(i)->stopWorker();
	}
	for (size_t i = 0; i < processes.size(); i++) {
		processes.at(i)->getWorker()->join();
		if (processes.at(i)->getAlive() &&
			!processes.at(i)->getCritical()) {
			HANDLE hdl = OpenProcess(PROCESS_TERMINATE, FALSE, processes.at(i)->getPID());
			TerminateProcess(hdl, 1);
		}
	}
}

void restartApp(void) {
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	memset(&processInfo, 0, sizeof(processInfo));

	CreateProcess("..\\..\\..\\..\\Streamlabs OBS.exe",
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

void terminalCriticalProcesses(void) {
	HANDLE hPipe = CreateFile(
		TEXT("\\\\.\\pipe\\exit-slobs-crash-handler"),
		GENERIC_READ |
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hPipe != INVALID_HANDLE_VALUE &&
		GetLastError() != ERROR_PIPE_BUSY) {
		std::vector<char> buffer;
		buffer.resize(sizeof(exitApp));
		memcpy(buffer.data(), &exitApp, sizeof(exitApp));

		WriteFile(
			hPipe,
			buffer.data(),
			buffer.size(),
			NULL,
			NULL);

		CloseHandle(hPipe);
	}
}

int main(void)
{
	std::thread processManager(checkProcesses);

	std::unique_ptr<NamedSocket> sock = NamedSocket::create();

	while (!exitApp)
	{
		exitApp = sock->read(&processes);
	}

	terminalCriticalProcesses();
	close();
	processManager.join();

	if (doRestartApp) {
		restartApp();
	}
	return 0;
}




