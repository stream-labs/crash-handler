#include <mutex>
#include <thread>
#include "socket.hpp"
#include "process.hpp"
#include "logger.hpp"

struct ThreadData {
    bool         isRunnning;
    bool         stop;
    std::mutex*  mtx;
    std::thread* worker;
};

class ProcessManager {
public:
    ProcessManager() {};
    ~ProcessManager() {};

    void runWatcher();

private:
    ThreadData* m_watcher;
    ThreadData* m_monitor;
    std::vector<std::unique_ptr<Process>> processes;
    std::mutex m_mtx;

    void watcher();
    void monitor();

    void startMonitoring();
    void stopMonitoring();

    size_t registerProcess(bool isCritical, uint32_t PID);
    size_t unregisterProcess(uint32_t PID);

    void handleCrash(bool criticalCrash);
};