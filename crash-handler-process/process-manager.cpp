#include "process-manager.hpp"

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
    std::unique_ptr<Socket> socket = Socket::create();

    while (!m_watcher->stop) {
        std::vector<char> buffer = socket->read();
        if (!buffer.size())
            continue;

        Message msg(buffer);
        switch (static_cast<Action>(msg.readUInt8())) {
            case Action::REGISTER: {
                bool isCritical = msg.readBool();
                uint32_t pid = msg.readUInt32();
                size_t size = registerProcess(isCritical, pid);
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
                m_watcher->stop = true;
                socket->disconnect();
                socket.reset();
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
    // while (!m_monitor->stop) {
    //     // TODO
    // }
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
        std::find_if(processes.begin(), processes.end(), [&PID](Process* p) {
            return p->getPID() == PID;
    });
    if (it == processes.end()) {
        processes.push_back(new Process(PID, isCritical));
    }
    log_info << "Processes size: " << processes.size() << std::endl;
    return processes.size();
}

size_t ProcessManager::unregisterProcess(uint32_t PID) {
    log_info << "unregister process " << PID << std::endl;

    const std::lock_guard<std::mutex> lock(m_mtx);

    auto it = 
        std::find_if(processes.begin(), processes.end(), [&PID](Process* p) {
            return p->getPID() == PID;
    });

    if (it != processes.end()) {
        processes.erase(it);
    }
    return processes.size();
}