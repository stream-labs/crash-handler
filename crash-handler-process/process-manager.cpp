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
    m_criticalCrash = false;
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

    while (!this->watcher->stop) {
        std::vector<char> buffer = this->socket->read();
        if (!buffer.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        Message msg(buffer);
        switch (static_cast<Action>(msg.readUInt8())) {
            case Action::REGISTER: {
                bool isCritical = msg.readBool();
                uint32_t pid = msg.readUInt32();
                size_t size = registerProcess(isCritical, (int32_t)pid);

                if (size == 1)
                    startMonitoring();

                break;
            }
            case Action::UNREGISTER: {
		        uint32_t pid = msg.readUInt32();
                unregisterProcess(pid);
                break;
            }
            default:
                break;
        }
    }
    log_info << "End Watcher" << std::endl;
}

void ProcessManager::monitor_fnc() {
    log_info << "Start monitoring" << std::endl;
    bool criticalCrash = false;
    bool unresponsiveMarked = false;

    while (!this->monitor->stop) {
        bool detectedUnresponsive = false;
        if (this->mtx.try_lock()) {
            for (auto & process : this->processes) {
                if (!process->isAlive()) {
                    // Log information about the process that just crashed
                    log_info << "process died" << std::endl;
                    log_info << "process.pid: " << process->getPID() << std::endl;
                    log_info << "process.isCritical: " << process->isCritical() << std::endl;

                    m_criticalCrash = process->isCritical();
                    m_applicationCrashed = this->monitor->stop = true;
                } else {
                    detectedUnresponsive |= process->isResponsive();
                }
            }

            this->mtx.unlock();
            if(unresponsiveMarked && !detectedUnresponsive) 
            {
                log_info << "Unresponsive window not detected anymore " << std::endl;
                Util::updateAppState(true);
                unresponsiveMarked = false;
            } else if(!unresponsiveMarked && detectedUnresponsive) 
            {
                log_info << "Unresponsive window detected " << std::endl;
                Util::updateAppState(false);
                unresponsiveMarked = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    this->watcher->stop = true;
#ifdef __APPLE__
    if (m_applicationCrashed) {
        std::vector<char> buffer;
        buffer.push_back('-1');
        this->socket->write(false, buffer);
    }
#endif
    log_info << "End monitoring" << std::endl;
}

void ProcessManager::startMonitoring() {
    this->monitor = new ThreadData();
    this->monitor->isRunnning = false;
    this->monitor->stop = false;

    this->monitor->worker =
        new std::thread(&ProcessManager::monitor_fnc, this);
}

void ProcessManager::stopMonitoring() {
    this->monitor->stop = true;

    if (this->monitor->worker->joinable())
        this->monitor->worker->join();
}

size_t ProcessManager::registerProcess(bool isCritical, uint32_t PID) {
    log_info << "register process" << std::endl;
    log_info << "pid " << PID << std::endl;
    log_info << "isCritical " << isCritical << std::endl;

    const std::lock_guard<std::mutex> lock(this->mtx);

    auto it = 
        std::find_if(this->processes.begin(),
                    this->processes.end(),
                    [&PID](std::unique_ptr<Process>& p) {
            return p->getPID() == PID;
    });
    if (it == this->processes.end()) {
        this->processes.push_back(Process::create(PID, isCritical));
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
    log_info << "isCritical " << (*it)->isCritical() << std::endl;

    if ((*it)->isCritical()) {
        this->stopMonitoring();
        this->watcher->stop = true;
        this->sendExitMessage(false);
    }

    this->processes.erase(it);
}

void ProcessManager::handleCrash(std::wstring path) {
    log_info << "Handling crash - process alive: " << std::endl;
    for (auto & process : this->processes) {
        if (process->isAlive()) {
            log_info << "----" << std::endl;
            log_info << "process.pid: " << process->getPID() << std::endl;
            log_info << "process.isCritical: " << process->isCritical() << std::endl;
            log_info << "----" << std::endl;
        }
    }

    bool shouldRestart = false;
    if (m_criticalCrash) {
        terminateAll();
    } else {
        // Blocking operation that will return once the user
        // decides to terminate the application
        Util::runTerminateWindow(shouldRestart);
        log_info << "Send exit message" << std::endl;
        this->sendExitMessage(true);
        log_info << "Terminate non critical processes" << std::endl;
        terminateNonCritical();
    }

    if (shouldRestart)
        Util::restartApp(path);
}

void ProcessManager::sendExitMessage(bool appCrashed) {
    std::vector<char> buffer;
    buffer.push_back(appCrashed);

    if (this->socket->write(true, buffer) <= 0)
        log_info << "Failed to send exit message" << std::endl;
}

void ProcessManager::terminateAll(void) {
    for (auto & process : this->processes) 
        process->terminate();
}

void ProcessManager::terminateNonCritical(void) {
    for (auto & process : this->processes) {
        if (!process->isCritical())
            process->terminate();
    }
}