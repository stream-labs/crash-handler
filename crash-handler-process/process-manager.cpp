#include "process-manager.hpp"

void ProcessManager::runWatcher() {
    m_watcher = new ThreadData();
    m_watcher->isRunnning = false;
    m_watcher->stop = false;
    m_watcher->worker =
        new std::thread(&ProcessManager::watcher, this, m_watcher);

    if (m_watcher->worker->joinable())
        m_watcher->worker->join();
}

void ProcessManager::watcher(ThreadData* td) {
    std::unique_ptr<Socket> socket = Socket::create();
    while (!td->stop) {
        std::vector<char> buffer = socket->read();
        if (!buffer.size())
            continue;

        Message msg(buffer);
        switch (static_cast<Action>(msg.readUInt8())) {
            case Action::REGISTER: {
                bool isCritical = msg.readBool();
                uint32_t pid = msg.readUInt32();
                registerProcess(isCritical, pid);
                break;
            }
            case Action::UNREGISTER: {
		        uint32_t pid = msg.readUInt32();
                unregisterProcess(pid);
                break;
            }
            case Action::EXIT: {
                td->stop = true;
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

void ProcessManager::monitor(ThreadData* td) {
    while (!td->stop) {
        // TODO
    }
}

void ProcessManager::startMonitoring() {
    m_monitor= new ThreadData();
    m_monitor->isRunnning = false;
    m_monitor->stop = false;

    m_watcher->worker =
        new std::thread(&ProcessManager::monitor, this, m_monitor);
}

void ProcessManager::stopMonitoring() {
    if (m_monitor->worker->joinable())
        m_monitor->worker->join();
}

size_t ProcessManager::registerProcess(bool isCritical, uint32_t PID) {
    const std::lock_guard<std::mutex> lock(m_mtx);

    auto it = 
        std::find_if(processes->begin(), processes->end(), [&PID](Process* p) {
            return p->getPID() == PID;
    });
    if (it == processes->end()) {
        processes->push_back(new Process(PID, isCritical));
    }
    return processes->size();
}

size_t ProcessManager::unregisterProcess(uint32_t PID) {
    const std::lock_guard<std::mutex> lock(m_mtx);

    auto it = 
        std::find_if(processes->begin(), processes->end(), [&PID](Process* p) {
            return p->getPID() == PID;
    });

    if (it != processes->end()) {
        processes->erase(it);
    }
    return processes->size();
}