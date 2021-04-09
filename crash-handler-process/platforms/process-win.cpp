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

void Process_WIN::startMemoryDumpMonitoring(void) {
	if (memorydump && memorydump->joinable()) {
		return;
	}

	mds = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\OBSMEMORYDUMPEVENT");
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

			mdf = CreateEvent( &eventSecurityAttr, TRUE, FALSE, L"Global\\OBSMEMORYDUMPFINISHEDEVENT");
		}
	}
	LocalFree(securityDescriptor);

	this->memorydump = new std::thread(&Process_WIN::memorydump_worker, this);
}

void Process_WIN::memorydump_worker() {
	log_info << "Memory dump worker started" << std::endl;
	HANDLE handles[] = { hdl, mds };
	DWORD ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
	if (ret - WAIT_OBJECT_0 == 1) {
		log_info << "Memory dump event detected" << std::endl;
		bool dump_saved = false;

		int code = MessageBox( NULL, L"An app crash was detected."
			L"\n\nSystem will attempt to save a memory dump."
			L"\n\nIt can takes a time depend on a disk speed and a size of process."
			L"\n\n"
			L"\n\nPress OK to start a process.",
			L"Memory dump", MB_OK | MB_APPLMODAL);

		if (code == IDOK) {
			dump_saved = Util::saveMemoryDump(PID);
		}
		
		SetEvent(mdf);

		TerminateProcess(hdl, 1);
		CloseHandle(hdl);
		hdl = INVALID_HANDLE_VALUE;

		if (code == IDOK) {
			if (dump_saved) {
				MessageBox( NULL, L"A memory dump of a crashed process has been saved."
					L"\n\nDo not forget to remove a folder to disable saving of memory dumps\n\n",
					L"Memory dump", MB_OK | MB_APPLMODAL);
			} else {
				MessageBox( NULL, L"Failed to save a memory dump."
					L"\n\n",
					L"Memory dump failed", MB_OK | MB_APPLMODAL);
			}
		}
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
    if (this->hdl && this->hdl != INVALID_HANDLE_VALUE) {
        TerminateProcess(hdl, 1);
        CloseHandle(hdl);
    }

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
