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
bool monitoring = false;

void checkProcesses(void) {
	bool stop = false;

	while (!stop && !exitApp) {
		bool alive = true;
		size_t index = 0;

		if (monitoring && processes.size() == 0) {
			exitApp = true;
			break;
		}

		while (alive && index < processes.size()) {
			monitoring = true;
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
		else if (processes.at(i)->getCritical()) {
			auto start = std::chrono::high_resolution_clock::now();
			while (processes.at(i)->getAlive()) {
				auto t = std::chrono::high_resolution_clock::now();
				auto delta = t - start;
				if (std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() > 2000) {
					HANDLE hdl = OpenProcess(PROCESS_TERMINATE, FALSE, processes.at(i)->getPID());
					TerminateProcess(hdl, 1);
					processes.at(i)->setAlive(false);
				}
			}
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

	while (!exitApp && !sock->read(&processes))
	{

	}

	exitApp = true;
	terminalCriticalProcesses();
	close();
	processManager.join();

	if (doRestartApp) {
		restartApp();
	}
	return 0;
}




