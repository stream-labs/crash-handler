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

#include "../process.hpp"
#include "../logger.hpp"

#include <windows.h>
#include <thread>
#include <mutex>

class Process_WIN : public Process {
private:
	std::thread *checker{nullptr};
	std::thread *memorydump{nullptr};
	std::mutex mtx;

	HANDLE handle_OpenProcess{INVALID_HANDLE_VALUE};
	HANDLE handle_event_Start{INVALID_HANDLE_VALUE};
	HANDLE handle_event_Fail{INVALID_HANDLE_VALUE};
	HANDLE handle_event_Success{INVALID_HANDLE_VALUE};

	std::wstring memorydumpPath;
	std::wstring memorydumpName;

	bool isValidHandle = false;

	HWND getTopWindow();

public:
	Process_WIN(int32_t pid, bool isCritical);
	~Process_WIN();

public:
	virtual int32_t getPID(void) override;
	virtual bool isValid(void) override;
	virtual bool isCritical(void) override;
	virtual bool isAlive(void) override;
	virtual bool isUnResponsive(void) override;
	virtual void terminate(void) override;

public:
	virtual void startMemoryDumpMonitoring(const std::wstring &eventName_Start, const std::wstring &eventName_Fail, const std::wstring &eventName_Success,
					       const std::wstring &dumpPath, const std::wstring &dumpName) override;

private:
	void worker();
	void memorydump_worker();
	DWORD getPIDDWORD();
	static bool isValidHandleValue(const HANDLE h);
	static void safeCloseHandle(HANDLE &h);
};