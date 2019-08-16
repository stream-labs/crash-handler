/******************************************************************************
    Copyright (C) 2016-2019 by Streamlabs (General Workings Inc)
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

#include "process.hpp"
#include <psapi.h>
#include <iostream>
#include <fstream>

#include "logger.hpp"

void check(void* p) {
	Process* proc = reinterpret_cast<Process*> (p);
	log_info << "check started" << std::endl;
	if (!WaitForSingleObject(proc->getHandle(), INFINITE)) {
		log_info << "check in wait for single object" << std::endl;
		proc->mutex.lock();
		proc->setAlive(false);
		proc->stopWorker();
		proc->mutex.unlock();
	}
}

Process::Process(uint64_t pid, bool isCritical) {
	log_info << "Process created" << std::endl;
	m_pid = pid;
	m_isCritical = isCritical;
	m_isAlive = true;
	m_stop = false;
	m_worker = nullptr;
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
	m_stop = true;
}

bool Process::getStopped(void) {
	return m_stop;
}

std::thread* Process::getWorker(void) {
	return m_worker;
}

HANDLE Process::getHandle(void) {
	return m_hdl;
}