#include "message.hpp"
#include "process.hpp"
#include <windows.h>

std::vector<Process*> processes;
bool exitApp = false;

void checkProcesses(void) {
	bool stop = false;

	while(!stop) {
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
					MB_OK
				);
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
		HANDLE hdl = OpenProcess(PROCESS_TERMINATE, FALSE, processes.at(i)->getPID());
		TerminateProcess(hdl, 1);
	}
}

void main() {
	std::unique_ptr<NamedSocket> socket = 
		NamedSocket::create("\\\\.\\pipe\\slobs-crash-handler");

	std::thread processManager(checkProcesses);

	while (!exitApp) {
		std::vector<char> buffer; 
		buffer.resize(512);

		size_t byteRead = socket->read(buffer.data(), buffer.size());

		if (byteRead > 0) {
			Message msg(buffer);
			if (msg.getAction() == REGISTER) {
				std::vector<char> data = msg.getData();
				bool isCritical = reinterpret_cast<bool&>
					(data[0]);

				size_t pid = reinterpret_cast<size_t&>
					(data[1]);

				processes.push_back(new Process(pid, isCritical, &socket));
			}
			else if (msg.getAction() == EXIT) {
				exitApp = true;
			}
			else {
				throw std::exception("Invalid Action");
			}
		}
	}
	close();

	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	memset(&processInfo, 0, sizeof(processInfo));

	CreateProcess("C:\\Program Files\\Streamlabs OBS\\Streamlabs OBS.exe",   
		"",        
		NULL,
		NULL,
		FALSE,
		CREATE_NO_WINDOW | DETACHED_PROCESS,
		NULL,
		NULL,
		&info,
		&processInfo
	);
}