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

#include <mutex>
#include <thread>
#include <atomic>

#include "socket.hpp"
#include "process.hpp"
#include "logger.hpp"
#include "util.hpp"

using stopper = std::atomic<bool>;

struct ThreadData {
    bool         isRunnning = false;
    stopper      stop = { false };
    std::mutex*  mtx = nullptr;
    std::thread* worker = nullptr;
};

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    void runWatcher();
    bool m_applicationCrashed;
    bool m_criticalCrash;

    void handleCrash(std::wstring path);
    void sendExitMessage(bool appCrashed);

private:
    ThreadData* watcher = nullptr;
    ThreadData* monitor = nullptr;
    std::vector<std::unique_ptr<Process>> processes;
    std::mutex mtx;
    std::unique_ptr<Socket> socket;

    void watcher_fnc();
    void monitor_fnc();

    void startMonitoring();
    void stopMonitoring();

    size_t registerProcess(bool isCritical, uint32_t PID);
    void unregisterProcess(uint32_t PID);
    void registerProcessMemoryDump(uint32_t PID, const std::wstring& eventName, const std::wstring& eventFinishedName, const std::wstring& dumpPath);

    void terminateAll(bool isNice=false);
    void terminateNonCritical(void);
};

