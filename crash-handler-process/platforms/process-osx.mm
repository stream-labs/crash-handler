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

bool Process_OSX::isAlive(void) {
    pid_t pids[2048];
    int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
    int n_proc = bytes / sizeof(pids[0]);
    for (int i = 0; i < n_proc; i++) {
         struct proc_bsdinfo proc;
         int st = proc_pidinfo(pids[i], PROC_PIDTBSDINFO, 0,
                              &proc, PROC_PIDTBSDINFO_SIZE);
		if (pids[i] == PID &&
			(strcmp(proc.pbi_name, "obs64") == 0 ||
			strncmp(proc.pbi_name, "Electron", strlen("Electron")) == 0 || // DEV env
			strncmp(proc.pbi_name, "Streamlabs", strlen("Streamlabs")) == 0)) // PRODUCTION env
			return true;
    }
	return false;
}

void Process_OSX::terminate(void) {
	kill(PID, SIGKILL);
}
