#include "process.hpp"
#include <psapi.h>
#include <iostream>

void check(void* p) {
	Process* proc = reinterpret_cast<Process*> (p);
	proc->setAlive(true);

	if (!WaitForSingleObject(proc->getHandle(), INFINITE)) {
		proc->mutex.lock();
		proc->setAlive(false);
		proc->stopWorker();
		proc->mutex.unlock();
	}
}

Process::Process(uint64_t pid, bool isCritical) {
	m_pid = pid;
	m_isCritical = isCritical;
	m_isAlive = true;
	m_stop = false;
	m_hdl = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (m_hdl)
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
	m_stopTime = std::chrono::system_clock::now().time_since_epoch().count();
	m_stop = true;
}

bool Process::getStopped(void) {
	return m_stop;
}

long long Process::getStopTime(void) {
	return m_stopTime;
}

std::thread* Process::getWorker(void) {
	return m_worker;
}

HANDLE Process::getHandle(void) {
	return m_hdl;
}