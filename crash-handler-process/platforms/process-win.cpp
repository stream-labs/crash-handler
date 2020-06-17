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

std::unique_ptr<Process> Process::create(int32_t pid, bool isCritical) {
	return std::make_unique<Process_WIN>(pid, isCritical);
}

Process_WIN::Process_WIN(int32_t pid, bool isCritical) {
	PID = pid;
	critical = isCritical;
	alive = true;
    handle = NULL;
}

int32_t Process_WIN::getPID(void) {
	return PID;
}

bool Process_WIN::isCritical(void) {
	return critical;
}

void Process_WIN::worker() {
    if (!handle)
        return;

    if (!WaitForSingleObject(handle, INFINITE)) {
        std::unique_lock<std::mutex> ul(this->mtx);
        this->alive = false;
    }
}

bool Process_WIN::isAlive(void) {
    std::unique_lock<std::mutex> ul(this->mtx);
    return this->alive;
}

void Process_WIN::terminate(void) {
    HANDLE hdl = OpenProcess(PROCESS_TERMINATE, FALSE, getPIDDWORD());
    if (hdl && hdl != INVALID_HANDLE_VALUE) {
        TerminateProcess(hdl, 1);
        CloseHandle(hdl);
    }

    if (this->checker->joinable())
        this->checker->join();
}

DWORD Process_WIN::getPIDDWORD(void) {
	return static_cast<DWORD>(PID);
}