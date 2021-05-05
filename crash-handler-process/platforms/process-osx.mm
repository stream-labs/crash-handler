#include "process-osx.hpp"
#include <Cocoa/Cocoa.h>
#include <iostream>
#include "../logger.hpp"

#include <libproc.h>
#include <stdio.h>
#include <string.h>

std::unique_ptr<Process> Process::create(int32_t pid, bool isCritical) {
	return std::make_unique<Process_OSX>(pid, isCritical);
}

Process_OSX::Process_OSX(int32_t pid, bool isCritical) {
	PID      = pid;
	critical = isCritical;
	alive    = true;
}

int32_t Process_OSX::getPID(void) {
	return PID;
}

bool Process_OSX::isCritical(void) {
	return critical;
}
bool Process_OSX::isResponsive(void) {
	return true; // check for responsiveness not impemented
}
void Process_OSX::startMemoryDumpMonitoring(const std::wstring& eventName, const std::wstring& eventFinishedName, const std::wstring& dumpPath) {
	return;
}

int Process_OSX::isAlive(void) {
	struct proc_bsdinfo proc;
	int st = proc_pidinfo(PID, PROC_PIDTBSDINFO, 0,
				&proc, PROC_PIDTBSDINFO_SIZE);
	if (st == PROC_PIDTBSDINFO_SIZE && (strcmp(proc.pbi_name, "obs64") == 0 ||
	strncmp(proc.pbi_name, "Electron", strlen("Electron")) == 0 || // DEV env
	strncmp(proc.pbi_name, "Streamlabs", strlen("Streamlabs")) == 0)) // PRODUCTION env
	{
		log_info << "Process_OSX::isAlive true [" << proc.pbi_name << "]" << " status " << proc.pbi_status << " xstatus: " << proc.pbi_xstatus << std::endl;
		return 1;
	}
	if (proc.pbi_status == 1 || proc.pbi_xstatus == 2)	{
		log_info << "Process_OSX::isAlive FALSE [" << proc.pbi_name << "]" << " status " << proc.pbi_status << " xstatus: " << proc.pbi_xstatus << std::endl;
		return 0;
	}
	log_info << "Process_OSX::isAlive process [" << PID << "]" << " EXIT OK " << std::endl;

	return 2;
}

void Process_OSX::terminate(void) {
	kill(PID, SIGKILL);
}

void Process_OSX::terminateNicely(void) {
	kill(PID, SIGTERM);
}

