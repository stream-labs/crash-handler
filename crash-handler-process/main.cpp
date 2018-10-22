#include <windows.h> 
#include <tchar.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include "namedsocket-win.hpp"
#include <sstream>
#include <fstream>
#include <codecvt>
#include <windows.h>
#include <psapi.h>

VOID DisconnectAndReconnect(DWORD);
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED);

std::vector<Process*> processes;
bool exitApp = false;
bool doRestartApp = false;
bool monitoring = false;

static thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

std::string from_utf16_wide_to_utf8(const wchar_t* from, size_t length = -1)
{
	const wchar_t* from_end;

	if (length == 0)
		return {};
	else if (length != -1)
		from_end = from + length;
	else
		return converter.to_bytes(from);

	return converter.to_bytes(from, from_end);
}

std::wstring from_utf8_to_utf16_wide(const char* from, size_t length = -1)
{
	const char* from_end;

	if (length == 0)
		return {};
	else if (length != -1)
		from_end = from + length;
	else
		return converter.from_bytes(from);

	return converter.from_bytes(from, from_end);
}

struct ProcessInfo
{
	uint64_t handle;
	uint64_t id;

	ProcessInfo()
	{
		this->handle = 0;
		this->id = 0;
	};
	ProcessInfo(uint64_t h, uint64_t i)
	{
		this->handle = h;
		this->id = i;
	}
};

ProcessInfo open_process(uint64_t handle)
{
	ProcessInfo pi;
	DWORD       flags = PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | PROCESS_VM_READ;

	pi.handle = (uint64_t)OpenProcess(flags, false, (DWORD)handle);
	return pi;
}

bool close_process(ProcessInfo pi)
{
	return !!CloseHandle((HANDLE)pi.handle);
}

std::string get_process_name(ProcessInfo pi)
{
	LPWSTR  lpBuffer = NULL;
	DWORD   dwBufferLength = 256;
	HANDLE  hProcess = (HANDLE)pi.handle;
	HMODULE hModule;
	DWORD   unused1;
	BOOL    bSuccess;

	/* We rely on undocumented behavior here where
	* enumerating a process' modules will provide
	* the process HMODULE first every time. */
	bSuccess = EnumProcessModules(hProcess, &hModule, sizeof(hModule), &unused1);

	if (!bSuccess)
		return {};

	while (32768 >= dwBufferLength) {
		std::wstring lpBuffer(dwBufferLength, wchar_t());

		DWORD dwReturnLength = GetModuleFileNameExW(hProcess, hModule, &lpBuffer[0], dwBufferLength);

		if (!dwReturnLength)
			return {};

		if (dwBufferLength <= dwReturnLength) {
			/* Increased buffer exponentially.
			* Notice this will eventually match
			* a perfect 32768 which is the max
			* length of an NTFS file path. */
			dwBufferLength <<= 1;
			continue;
		}

		/* Notice that these are expensive
		* but they do shrink the buffer to
		* match the string */
		return from_utf16_wide_to_utf8(lpBuffer.data());
	}

	/* Path too long */
	return {};
}

std::fstream open_file(std::string& file_path, std::fstream::openmode mode)
{
	return std::fstream(from_utf8_to_utf16_wide(file_path.c_str()), mode);
}

bool kill(ProcessInfo pinfo, uint32_t code)
{
	return TerminateProcess(reinterpret_cast<HANDLE>(pinfo.handle), code);
}

static void check_pid_file(std::string& pid_path)
{
	std::fstream::openmode mode = std::fstream::in | std::fstream::binary;

	std::fstream pid_file(open_file(pid_path, mode));

	union
	{
		uint64_t pid;
		char     pid_char[sizeof(uint64_t)];
	};

	if (!pid_file)
		return;

	pid_file.read(&pid_char[0], 8);

	ProcessInfo pi = open_process(pid);

	if (pi.handle == 0)
		return;

	std::string name = get_process_name(pi);

	// if (name == serverBinaryPath) {
		kill(pi, -1);
	//}

	close_process(pi);
}

std::string get_temp_directory()
{
	constexpr DWORD tmp_size = MAX_PATH + 1;
	std::wstring    tmp(tmp_size, wchar_t());
	GetTempPathW(tmp_size, &tmp[0]);

	/* Here we resize an in-use string and then re-use it.
	* Note this is only okay because the long path name
	* will always be equal to or larger than the short
	* path name */
	DWORD tmp_len = GetLongPathNameW(&tmp[0], NULL, 0);
	tmp.resize(tmp_len);

	/* Note that this isn't a hack to use the same buffer,
	* it's explicitly meant to be used this way per MSDN. */
	GetLongPathNameW(&tmp[0], &tmp[0], tmp_len);

	return from_utf16_wide_to_utf8(tmp.data());
}

static void write_pid_file(std::string& pid_path, uint64_t pid)
{
	std::fstream::openmode mode = std::fstream::out | std::fstream::binary | std::fstream::trunc;

	std::fstream pid_file(open_file(pid_path, mode));

	if (!pid_file)
		return;

	pid_file.write(reinterpret_cast<char*>(&pid), sizeof(pid));
}

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
	std::string pid_path(get_temp_directory());
	pid_path.append("server.pid");

	check_pid_file(pid_path);

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




