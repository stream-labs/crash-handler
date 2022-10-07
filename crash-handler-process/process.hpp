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

#include <thread>
#include <mutex>
#ifdef WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/types.h>
#include <signal.h>
#endif

class Process {
public:
	static std::unique_ptr<Process> create(int32_t pid, bool isCritical);

	Process(){};
	virtual ~Process(){};

protected:
	int32_t PID = 0;
	bool critical = false;
	bool alive = false;
	bool recievedDmpEvent = false;

public:
	virtual int32_t getPID(void) = 0;
	virtual bool isCritical(void) = 0;
	virtual bool isAlive(void) = 0;
	virtual bool isUnResponsive(void) = 0;
	virtual void terminate(void) = 0;

public:
	virtual void startMemoryDumpMonitoring(const std::wstring &eventName_Start, const std::wstring &eventName_Fail, const std::wstring &eventName_Success,
					       const std::wstring &dumpPath, const std::wstring &dumpName) = 0;
};
