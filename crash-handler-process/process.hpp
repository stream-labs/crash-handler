/******************************************************************************
    Copyright (C) 2016-2019 by Streamlabs (General Workings Inc)
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
#endif

class Process {
public:
	Process(uint64_t pid, bool isCritical);
	~Process();

private:
	uint64_t m_pid;
	bool m_isCritical;
	std::thread* m_worker;
	bool m_isAlive;
	bool m_stop;
	uint64_t m_stopTime = 0;
	std::string m_name;
	// HANDLE m_hdl;

public:
	std::mutex mutex;

	uint64_t getPID(void);
	// DWORD getPIDDWORD(void);
	void setPID(uint64_t pid);
	bool getCritical(void);
	void setCritical(bool isCritical);
	bool getAlive(void);
	void setAlive(bool isAlive);
	bool checkAlive(void);
	bool getStopped(void);
    uint64_t getStopTime(void);
	std::thread* getWorker(void);
	// HANDLE getHandle(void);

	void stopWorker();
};