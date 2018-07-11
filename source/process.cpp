#include "process.hpp"
#include <windows.h>
#include <psapi.h>
#include <iostream>

void check(void* p) {
	Process* proc = reinterpret_cast<Process*> (p);
		
	DWORD   lpidProcess[1024];
	DWORD   cb;
	DWORD	lpcbNeeded;

	while(!proc->getStopped()) {
		std::unique_lock<std::mutex> ulock(proc->mutex);
		proc->setAlive(false);

		bool success =  EnumProcesses(
			lpidProcess,
			sizeof(lpidProcess),
			&lpcbNeeded
		);

		if (success) {
			uint32_t index = 0;
			uint32_t procCount = (uint32_t)lpcbNeeded / sizeof(DWORD);

			while (!proc->getAlive() && index < procCount) {
				std::cout << lpidProcess[index] << std::endl;
				proc->setAlive(lpidProcess[index] == proc->getPID());
				index++;
			}
		}
		
		if (!proc->getAlive()) {
			proc->stopWorker();
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
}

Process::Process(uint64_t pid, bool isCritical, std::unique_ptr<NamedSocket>* sock) {
	m_pid = pid;
	m_isCritical = isCritical;
	m_isAlive = true;
	m_stop = false;
	m_sock = sock;

	m_worker = new std::thread(check, this);
}

Process::~Process() {

}

uint64_t Process::getPID(void) {
	return m_pid;
}

void Process::setPID(uint64_t pid) {
	m_pid = pid;
}

bool Process::getCritical(void) {
	return m_isCritical;
}

void Process::setCritical(bool isCritical) {
	m_isCritical = isCritical;
}

bool Process::getAlive(void) {
	return m_isAlive;
}

void Process::setAlive(bool isAlive) {
	m_isAlive = isAlive;
}

void Process::stopWorker() {
	m_stop = true;
}

bool Process::getStopped(void) {
	return m_stop;
}

std::thread* Process::getWorker(void) {
	return m_worker;
}
