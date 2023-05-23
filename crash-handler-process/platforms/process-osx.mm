#include "process-osx.hpp"
#include <Cocoa/Cocoa.h>
#include <iostream>
#include "../logger.hpp"

#include <libproc.h>
#include <stdio.h>
#include <string.h>

std::unique_ptr<Process> Process::create(int32_t pid, bool isCritical)
{
	return std::make_unique<Process_OSX>(pid, isCritical);
}

Process_OSX::Process_OSX(int32_t pid, bool isCritical)
{
	PID = pid;
	critical = isCritical;
	alive = true;
}

int32_t Process_OSX::getPID(void)
{
	return PID;
}

bool Process_OSX::isValid(void)
{
	return true;
}

bool Process_OSX::isCritical(void)
{
	return critical;
}
bool Process_OSX::isUnResponsive(void)
{
	return false; // check for responsiveness not impemented
}
void Process_OSX::startMemoryDumpMonitoring(const std::wstring &eventName_Start, const std::wstring &eventName_Fail, const std::wstring &eventName_Success,
					    const std::wstring &dumpPath, const std::wstring &dumpName)
{
	return;
}

bool Process_OSX::isAlive(void)
{
	struct proc_bsdinfo proc;
	int st = proc_pidinfo(PID, PROC_PIDTBSDINFO, 0, &proc, PROC_PIDTBSDINFO_SIZE);
	if (st == PROC_PIDTBSDINFO_SIZE && (strcmp(proc.pbi_name, "obs64") == 0 || strncmp(proc.pbi_name, "Electron", strlen("Electron")) == 0 || // DEV env
					    strncmp(proc.pbi_name, "Streamlabs", strlen("Streamlabs")) == 0)) // PRODUCTION env
		return true;

	return false;
}

void Process_OSX::terminate(void)
{
	kill(PID, SIGKILL);
}
