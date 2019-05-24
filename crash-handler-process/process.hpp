#include <thread>
#include <mutex>
#include <windows.h>

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
	HANDLE m_hdl;

public:
	std::mutex mutex;

	uint64_t getPID(void);
	void setPID(uint64_t pid);
	bool getCritical(void);
	void setCritical(bool isCritical);
	bool getAlive(void);
	void setAlive(bool isAlive);
	bool getStopped(void);
    uint64_t getStopTime(void);
	std::thread* getWorker(void);
	HANDLE getHandle(void);

	void stopWorker();
};