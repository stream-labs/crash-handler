#include <mutex>
#include <thread>
#include <atomic>

#include "socket.hpp"
#include "process.hpp"
#include "logger.hpp"
#include "util.hpp"

using stopper = std::atomic<bool>;

struct ThreadData {
    bool         isRunnning;
    stopper      stop = { false };
    std::mutex*  mtx;
    std::thread* worker;
};

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    void runWatcher();
    bool m_applicationCrashed;
    bool m_criticalCrash;

    void handleCrash(void);
    void sendExitMessage(void);

private:
    ThreadData* m_watcher;
    ThreadData* m_monitor;
    std::vector<std::unique_ptr<Process>> m_processes;
    std::mutex m_mtx;
    std::unique_ptr<Socket> m_socket;

    void watcher();
    void monitor();

    void startMonitoring();
    void stopMonitoring();

    size_t registerProcess(bool isCritical, uint32_t PID);
    size_t unregisterProcess(uint32_t PID);

    void terminateAll(void);
};