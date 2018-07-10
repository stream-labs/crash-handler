#include <thread>
#include "namedsocket.hpp"

class Process {
public:
	Process(uint64_t pid, bool isCritical, NamedSocket* sock);
	~Process();

private:
	uint64_t m_pid;
	bool m_isCritical;
	std::thread* m_worker;
	bool m_isAlive;
	bool m_stop;
	NamedSocket* m_sock;
	std::string m_name;
	
public:
	uint64_t getPID(void);
	void setPID(uint64_t pid);
	bool getCritical(void);
	void setCritical(bool isCritical);
	bool getAlive(void);
	void setAlive(bool isAlive);
	bool getStopped(void);

	void stopWorker();
};