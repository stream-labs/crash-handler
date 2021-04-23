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

#include "process-win.hpp"
#include "../util.hpp"
#include "upload-window-win.hpp"
#include <iomanip>
#include <ctime>
#include <filesystem>
#include "Shlobj.h"
#pragma comment(lib, "Shell32.lib")

struct handle_data {
    unsigned long process_id;
    HWND window_handle;
};

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !(GetWindow(handle, GW_OWNER) == (HWND)0 ))
        return TRUE;

    data.window_handle = handle;
    return FALSE;   
}

std::unique_ptr<Process> Process::create(int32_t pid, bool isCritical) {
	return std::make_unique<Process_WIN>(pid, isCritical);
}

Process_WIN::Process_WIN(int32_t pid, bool isCritical) {
	this->PID = pid;
	this->critical = isCritical;
	this->alive = true;
	this->hdl = OpenProcess(PROCESS_ALL_ACCESS, FALSE, getPIDDWORD());

	this->checker = new std::thread(&Process_WIN::worker, this);

	this->memorydump = nullptr;
	this->mds = INVALID_HANDLE_VALUE;
	this->mdf = INVALID_HANDLE_VALUE;
}

Process_WIN::~Process_WIN() {
	if(this->hdl)
		CloseHandle(this->hdl);

	if (this->checker->joinable())
		this->checker->join();

	if (memorydump != nullptr && memorydump->joinable())
		memorydump->join();
	if (mds && mds != INVALID_HANDLE_VALUE) {
		CloseHandle(mds);
		mds = NULL;
	}
	if (mdf && mdf != INVALID_HANDLE_VALUE) {
		CloseHandle(mdf);
		mdf = NULL;
	}
}

int32_t Process_WIN::getPID(void) {
	return PID;
}

bool Process_WIN::isCritical(void) {
	return critical;
}

void Process_WIN::startMemoryDumpMonitoring(const std::wstring& eventName, const std::wstring& eventFinishedName, const std::wstring& dumpPath) {
	if (memorydump && memorydump->joinable()) {
		return;
	}

	mds = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName.c_str());
	if (!mds || mds == INVALID_HANDLE_VALUE) {
		log_info << "Failed to open event for memory dump " << GetLastError()<< std::endl;
		return;
	}

	PSECURITY_DESCRIPTOR securityDescriptor = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (InitializeSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
		if (SetSecurityDescriptorDacl( securityDescriptor, TRUE, NULL, FALSE)) {
			SECURITY_ATTRIBUTES eventSecurityAttr = {0};
			eventSecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			eventSecurityAttr.lpSecurityDescriptor = securityDescriptor;
			eventSecurityAttr.bInheritHandle = FALSE;

			mdf = CreateEvent( &eventSecurityAttr, TRUE, FALSE, eventFinishedName.c_str());
			if (!mdf || mdf == INVALID_HANDLE_VALUE) {
				log_info << "Failed to create event for memory dump finish " << GetLastError()<< std::endl;
			}
		}
	}
	LocalFree(securityDescriptor);
	memorydumpPath = dumpPath;
	memorydump = new std::thread(&Process_WIN::memorydump_worker, this);
}

const std::wstring generateCrashDumpFileName()
{
	std::time_t t = std::time(nullptr);
	wchar_t wstr[100];
	std::wstring file_name = L"crash_memory_dump.dmp";
	if (std::wcsftime(wstr, 100, L"%Y%M%d_%H%M%S.dmp", std::localtime(&t))) {
		file_name = wstr;
	}
	return file_name;
}

void Process_WIN::memorydump_worker() {
	log_info << "Memory dump worker started" << std::endl;
	HANDLE handles[] = { hdl, mds };
	DWORD ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
	if (ret - WAIT_OBJECT_0 == 1) {
		log_info << "Memory dump worker event recieved" << std::endl;
		if (std::filesystem::exists(memorydumpPath) && UploadWindow::getInstance()->createWindow()) {
			alive = false;
			UploadWindow::getInstance()->crashCatched();
			log_info << "Window created. Waiting for user decision" << std::endl;
			if (UploadWindow::getInstance()->waitForUserChoise() == IDYES) {
				log_info << "User selected OK for saving a dump" << std::endl;
				UploadWindow::getInstance()->savingStarted();

				const std::wstring file_name = generateCrashDumpFileName();
				UploadWindow::getInstance()->setDumpFileName(file_name);

				bool dump_saved = Util::saveMemoryDump(PID, memorydumpPath, file_name);
				if (dump_saved) {
					UploadWindow::getInstance()->savingFinished();
					if (UploadWindow::getInstance()->waitForUserChoise() == IDYES) {
						Util::uploadToAWS(memorydumpPath, file_name);
						UploadWindow::getInstance()->waitForUserChoise();
						Util::removeMemoryDump(memorydumpPath, file_name);
					} else {
						UploadWindow::getInstance()->uploadCanceled();
						UploadWindow::getInstance()->waitForUserChoise();
						log_info << "User selected Cancel for uploading a dump" << std::endl;
					}
				} else {
					UploadWindow::getInstance()->savingFailed();
					UploadWindow::getInstance()->waitForUserChoise();
				}
			} else {
				log_info << "User selected Cancel for saving a dump" << std::endl;
			}
		}
		SetEvent(mdf);
		UploadWindow::shutdownInstance();
	} else {
		DWORD last_error = GetLastError();
		log_info << "Memory dump worker exited wait ret = " << ret << ", last error = " << last_error << std::endl;
	}
}

void Process_WIN::worker() {
    if (!this->hdl)
        return;

    if (!WaitForSingleObject(this->hdl, INFINITE)) {
        std::unique_lock<std::mutex> ul(this->mtx);
        this->alive = false;
        this->hdl = NULL;
    }
}

bool Process_WIN::isAlive(void) {
    std::unique_lock<std::mutex> ul(this->mtx);
    return this->alive;
}

bool Process_WIN::isResponsive(void) {
    std::unique_lock<std::mutex> ul(this->mtx);
	HWND window = getTopWindow();
	if (window)
		return IsHungAppWindow(window);
	else 
		return false;
}

void Process_WIN::terminate(void) {
    if (mds && mds != INVALID_HANDLE_VALUE) {
        CloseHandle(mds);
        mds = NULL;
    }

    if (memorydump != nullptr && memorydump->joinable())
        memorydump->join();

    if (mdf && mdf != INVALID_HANDLE_VALUE) {
        CloseHandle(mdf);
        mdf = NULL;
    }

    if (this->hdl && this->hdl != INVALID_HANDLE_VALUE) {
        TerminateProcess(hdl, 1);
        CloseHandle(hdl);
    }

    if (this->checker->joinable())
        this->checker->join();
}

DWORD Process_WIN::getPIDDWORD(void) {
	return static_cast<DWORD>(PID);
}

HWND Process_WIN::getTopWindow()
{
    handle_data data;
    data.process_id = PID;
    data.window_handle = 0;
    EnumWindows(enum_windows_callback, (LPARAM)&data);
    return data.window_handle;
}
