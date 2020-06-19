/******************************************************************************
	Copyright (C) 2016-2020 by Streamlabs (General Workings Inc)

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

#include "../util.hpp"

#include <windows.h>
#include <string>
#include <fstream>
#include <codecvt>
#include <psapi.h>

void Util::restartApp(std::wstring path) {
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	memset(&processInfo, 0, sizeof(processInfo));

	std::wstring slobs_path = path.substr(0, path.size() - strlen("app.asar.unpacked"));
	slobs_path += L"\\Streamlabs OBS.exe";

	CreateProcess(slobs_path.c_str(),
		L"",
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

void Util::runTerminateWindow() {
    int code = MessageBox(
        NULL,
        L"An error occurred which has caused Streamlabs OBS to close. Don't worry! If you were streaming or recording, that is still happening in the background."
        L"\n\nWhenever you're ready, we can relaunch the application, however this will end your stream / recording session.\n\n"
        L"Click the Yes button to keep streaming / recording. \n\n"
        L"Click the No button to stop streaming / recording.",
        L"An error occurred",
        MB_YESNO | MB_SYSTEMMODAL
        );
    switch (code) {
        case IDYES:
        {
            MessageBox(
                NULL,
                L"Your stream / recording session is still running in the background. Whenever you're ready, click the OK button below to end your stream / recording and relaunch the application.",
                L"Choose when to restart",
                MB_OK | MB_SYSTEMMODAL
            );
            break;
        }
        case IDNO:
        default:
            break;
    }
}

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

void Util::check_pid_file(std::string& pid_path) {
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
	if (name.find("crash-handler-process.exe") != std::string::npos) {
		kill(pi, -1);
	}
	close_process(pi);
}

std::string Util::get_temp_directory() {
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