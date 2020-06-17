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

ProcessManager::ProcessManager() {
    m_applicationCrashed = false;
    m_criticalCrash = false;
}

ProcessManager::~ProcessManager() {
    m_socket->disconnect();
    m_socket.reset();
}

void ProcessManager::runWatcher() {
    m_watcher = new ThreadData();
    m_watcher->isRunnning = false;
    m_watcher->stop = false;
    m_watcher->worker =
        new std::thread(&ProcessManager::watcher, this);

    if (m_watcher->worker->joinable())
        m_watcher->worker->join();
}

void ProcessManager::watcher() {
    m_socket = Socket::create();

    while (!m_watcher->stop) {
        std::vector<char> buffer = m_socket->read();
        if (!buffer.size())
            continue;

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
                size_t size = unregisterProcess(pid);
                if (!size)
                    stopMonitoring();
                break;
            }
            case Action::EXIT: {
                log_info << "exit message received" << std::endl;
                stopMonitoring();
                m_watcher->stop = true;
                break;
            }
            case Action::CRASH_ID: {
                // Should we remove?
                break;
            }
            default:
                break;
        }
    }
}

void ProcessManager::monitor() {
    bool crashed       = false;
    bool criticalCrash = false;

    while (!m_monitor->stop) {
        m_mtx.lock();

        if (!m_processes.size())
            goto sleep;

        for (int i = 0; i < m_processes.size(); i++) {
            if (!m_processes.at(i)->isAlive()) {
                // Log information about the process that just crashed
                log_info << "process died" << std::endl;
                log_info << "process.pid: " << m_processes.at(i)->getPID() << std::endl;
                log_info << "process.isCritical: " << m_processes.at(i)->isCritical() << std::endl;

                m_criticalCrash = m_processes.at(i)->isCritical();
                m_applicationCrashed = m_monitor->stop = true;
            }
        }

sleep:
        m_mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Stop registering / unregistering new processes
    m_watcher->stop = true;
    std::vector<char> buffer;
    buffer.push_back('1');
    m_socket->write(FILE_NAME, buffer);
    // if(m_watcher->worker->joinable())
    //     m_watcher->worker->join();
}

void ProcessManager::startMonitoring() {
    log_info << "start monitoring" << std::endl;
    m_monitor= new ThreadData();
    m_monitor->isRunnning = false;
    m_monitor->stop = false;

    m_watcher->worker =
        new std::thread(&ProcessManager::monitor, this);
}

void ProcessManager::stopMonitoring() {
    log_info << "stop monitoring" << std::endl;
    m_monitor->stop = true;

    if (m_monitor->worker->joinable())
        m_monitor->worker->join();
}

size_t ProcessManager::registerProcess(bool isCritical, uint32_t PID) {
    log_info << "register process" << std::endl;
    log_info << "pid " << PID << std::endl;
    log_info << "isCritical " << isCritical << std::endl;

    const std::lock_guard<std::mutex> lock(m_mtx);

    auto it = 
        std::find_if(m_processes.begin(),
                    m_processes.end(),
                    [&PID](std::unique_ptr<Process>& p) {
            return p->getPID() == PID;
    });
    if (it == m_processes.end()) {
        m_processes.push_back(Process::create(PID, isCritical));
    }
    log_info << "Processes size: " << m_processes.size() << std::endl;
    return m_processes.size();
}

size_t ProcessManager::unregisterProcess(uint32_t PID) {
    log_info << "unregister process " << PID << std::endl;

    const std::lock_guard<std::mutex> lock(m_mtx);

    auto it = 
        std::find_if(m_processes.begin(),
                    m_processes.end(),
                    [&PID](std::unique_ptr<Process>& p) {
            return p->getPID() == PID;
    });

    if (it != m_processes.end()) {
        m_processes.erase(it);
    }
    return m_processes.size();
}

void ProcessManager::handleCrash(void) {
    // Log process alive for debuggigng purposes
    // TODO: also log processes names
    log_info << "process alive" << std::endl;
    for (int i = 0; i < m_processes.size(); i++) {
        if(m_processes.at(i)->isAlive()) {
            log_info << "----" << std::endl;
            log_info << "process.pid: " << m_processes.at(i)->getPID() << std::endl;
            log_info << "process.isCritical: " << m_processes.at(i)->isCritical() << std::endl;
            log_info << "----" << std::endl;
        }
    }

    if (m_criticalCrash) {
        log_info << "Critical process crashed" << std::endl;
    } else {
        // Blocking operation that will return once the user
        // decides to terminate the application
        Util::runTerminateWindow();
    }
    terminateAll();
}

void ProcessManager::sendExitMessage(void) {
    std::vector<char> buffer;
    buffer.push_back('1');

    if (m_socket->write(FILE_NAME_EXIT, buffer) <= 0)
        log_info << "Failed to send exit message";
}

void ProcessManager::terminateAll(void) {
    for (int i = 0; i < m_processes.size(); i++) {
        m_processes.at(i)->terminate();
    }
}