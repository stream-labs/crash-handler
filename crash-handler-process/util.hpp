/******************************************************************************
	Copyright (C) 2016-2020 by Streamlabs (General Workings Inc)

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

#ifndef UTIL_H
#define UTIL_H

#include <string>

class Util {
public:
	static void runTerminateWindow(bool &shouldRestart);
	static void check_pid_file(std::string& pid_path);
	static void write_pid_file(std::string& pid_path);
	static std::string get_temp_directory();
	static void restartApp(std::wstring path);

	static bool saveMemoryDump(uint32_t pid, const std::wstring& dumpPath, const std::wstring& dumpFileName);
	static bool uploadToAWS(const std::wstring& dumpPath, const std::wstring& dumpFileName);
	static bool removeMemoryDump(const std::wstring& dumpPath, const std::wstring& dumpFileName);
	static void updateAppState(bool unresponsive_detected);
	static void setCachePath(std::wstring path);
};

#endif