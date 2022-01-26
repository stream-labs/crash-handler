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

#include "process-manager.hpp"

#include <iostream>

ProcessManager::ProcessManager() {
    m_applicationCrashed = false;
}

ProcessManager::~ProcessManager() {
    this->processes.clear();
    this->socket->disconnect();
    this->socket.reset();
}

void ProcessManager::runWatcher() {
    this->watcher = new ThreadData();
    this->watcher->isRunnning = false;
    this->watcher->stop = false;
    this->watcher->worker =
        new std::thread(&ProcessManager::watcher_fnc, this);

    if (this->watcher->worker->joinable())
        this->watcher->worker->join();
}

void ProcessManager::watcher_fnc() {
    log_info << "Start Watcher" << std::endl;
    this->socket = Socket::create();
    if (this->socket->initialization_failed)
        return;
    bool registered = false;

    while (!this->watcher->stop) {
        std::vector<char> buffer = this->socket->read();
        if (!buffer.size()) {
            {
                const std::lock_guard<std::mutex> lock(this->mtx);
                if (registered && this->processes.size() > 0 && !this->processes[0]->isRunning())
                    break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        Message msg(buffer);
        switch (static_cast<Action>(msg.readUInt8())) {
            case Action::REGISTER: {
                uint32_t pid = msg.readUInt32();
                size_t size = registerProcess(pid);
                registered = true;
                break;
            }
            case Action::UNREGISTER: {
		        uint32_t pid = msg.readUInt32();
                unregisterProcess(pid);
                break;
            }
            case Action::REGISTERMEMORYDUMP: {
                uint32_t pid = msg.readUInt32();
                std::wstring eventName_Start = msg.readWstring();
                std::wstring eventName_Fail = msg.readWstring();
                std::wstring eventName_Success = msg.readWstring();
                std::wstring dumpPath = msg.readWstring();
                std::wstring dumpName = msg.readWstring();
                registerProcessMemoryDump(pid, eventName_Start, eventName_Fail, eventName_Success, dumpPath, dumpName);
                break;
            }
            default:
                break;
        }
    }
    log_info << "End Watcher" << std::endl;
}

size_t ProcessManager::registerProcess(uint32_t PID) {
    log_info << "register process" << std::endl;
    log_info << "pid " << PID << std::endl;

    const std::lock_guard<std::mutex> lock(this->mtx);

    auto it = 
        std::find_if(this->processes.begin(),
                    this->processes.end(),
                    [&PID](std::unique_ptr<Process>& p) {
            return p->getPID() == PID;
    });
    if (it == this->processes.end()) {
        this->processes.push_back(Process::create(PID));
    }
    log_info << "Processes size: " << this->processes.size() << std::endl;
    return this->processes.size();
}

void ProcessManager::unregisterProcess(uint32_t PID) {
    const std::lock_guard<std::mutex> lock(this->mtx);

    auto it = 
        std::find_if(this->processes.begin(),
                    this->processes.end(),
                    [&PID](std::unique_ptr<Process>& p) {
            return p->getPID() == PID;
    });

    if (it == this->processes.end())
        return;

    log_info << "unregister process" << std::endl;
    log_info << "pid " << PID << std::endl;

    this->processes.erase(it);
}

void ProcessManager::registerProcessMemoryDump(uint32_t PID, const std::wstring& eventName_Start, const std::wstring& eventName_Fail, const std::wstring& eventName_Success, const std::wstring& dumpPath, const std::wstring& dumpName) {
    const std::lock_guard<std::mutex> lock(this->mtx);

    log_info << "requested memory dump on crash for pid = " << PID << std::endl;
    auto it =
        std::find_if(this->processes.begin(),
                    this->processes.end(),
                    [&PID](std::unique_ptr<Process>& p) {
            return p->getPID() == PID;
    });

    if (it == this->processes.end())
        return;

    log_info << "register for memory dump" << std::endl;
    (*it)->startMemoryDumpMonitoring(eventName_Start, eventName_Fail, eventName_Success, dumpPath, dumpName);
}

void ProcessManager::handleCrash(std::wstring path) {
    log_info << "Handling crash" << std::endl;

    terminateAll();
    Util::restartApp(path);
}

void ProcessManager::sendExitMessage(bool appCrashed) {
    std::vector<char> buffer;
    buffer.push_back(appCrashed);

    if (this->socket->write(true, buffer) <= 0)
        log_info << "Failed to send exit message" << std::endl;
}

void ProcessManager::terminateAll(void) {

    std::vector<std::thread> workers;

    // What if the head process is busy? Do we really want the other processes to keep running?
    // Instead, seperate these out into workers so that non-busy processes are killed asap
    for (auto& process : this->processes) {
		workers.push_back(std::thread([](Process* ptr) { ptr->terminate(); }, process.get()));
	}
     
    for (auto& itr : workers) {
		if (itr.joinable())
			itr.join();
    }
}