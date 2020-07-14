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
}

Process_WIN::~Process_WIN() {
	if(this->hdl)
		CloseHandle(this->hdl);

	if (this->checker->joinable())
		this->checker->join();
}

int32_t Process_WIN::getPID(void) {
	return PID;
}

bool Process_WIN::isCritical(void) {
	return critical;
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
