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
bool closeAll = false;

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

void checkProcesses(void) {

	while (!exitApp) {
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
					"An error occurred which has caused Streamlabs OBS to close. Don't worry! If you were streaming or recording, that is still happening in the background."
					"\n\nWhenever you're ready, we can relaunch the application, however this will end your stream / recording session.\n\n"
					"Click the Yes button to keep streaming / recording. \n\n"
					"Click the No button to stop streaming / recording and restart the application.",
					"An error occurred",
					MB_YESNO | MB_SYSTEMMODAL
				);
				switch (code) {
				case IDYES:
				{
					MessageBox(
						NULL,
						"Your stream / recording session is still running in the background. Whenever you're ready, click the OK button below to end your stream / recording and relaunch the application.",
						"Choose when to restart",
						MB_OK | MB_SYSTEMMODAL
					);
					doRestartApp = true;
					break;
				}
				case IDNO:
				{
					doRestartApp = true;
					break;
				}
				default:
					break;
				}
				terminalCriticalProcesses();
			}
			else {
				closeAll = true;
			}
			exitApp = true;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
	exitApp = true;
}

void close(bool doCloseALl) {
	for (size_t i = 0; i < processes.size(); i++) {
		processes.at(i)->stopWorker();
	}
	for (size_t i = 0; i < processes.size(); i++) {
		processes.at(i)->getWorker()->join();
		if (processes.at(i)->getAlive() &&
			!processes.at(i)->getCritical() && closeAll) {
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

void restartApp(std::string path) {
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	memset(&processInfo, 0, sizeof(processInfo));

	path.substr(0, path.size() - strlen("app.asar.unpacked"));
	path += "\Streamlabs OBS.exe";

	CreateProcess(path.c_str(),
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

int main(int argc, char** argv)
{
	std::string path;
	if (argc == 1)
		path = argv[0];
	std::thread processManager(checkProcesses);

	std::unique_ptr<NamedSocket> sock = NamedSocket::create();

	while (!exitApp && !sock->read(&processes))
	{

	}

	exitApp = true;
	if (processManager.joinable())
		processManager.join();
	close(closeAll);

	if (doRestartApp) {
		restartApp(path);
	}
	return 0;
}




