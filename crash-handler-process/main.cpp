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

#include "logger.hpp"
#include "process-manager.hpp"
#include "util.hpp"
#include <codecvt>


#if defined(WIN32)
	const std::wstring log_file_name = L"\\crash-handler.log";

#else // for __APPLE__ and other 
	const std::wstring log_file_name = L"/crash-handler.log";
#endif

int main(int argc, char** argv)
{
	std::string pid_path(Util::get_temp_directory());
	pid_path.append("crash-handler.pid");
	Util::check_pid_file(pid_path);
	Util::write_pid_file(pid_path);

	std::wstring path;
	std::wstring cache_path = L"";
	std::string version;
	std::string isDevEnv;
	std::wstring ipc_path;

	std::cout << "Launched with number of arguments = " << argc << std::endl;

	// Frontend passes as non-unicode
	if (argc >= 1)
		path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(argv[0]);
	if (argc >= 3)
		version = argv[2];
	if (argc >= 4)
		isDevEnv = argv[3];
	if (argc >= 5) {
		cache_path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(argv[4]);
		std::wstring log_path = cache_path + log_file_name;
		std::cout << "Path for logging = " << std::string(log_path.begin(), log_path.end()) << std::endl;
		logging_start(log_path);
		log_info << "=== Started CrashHandler ===" << std::endl;
		Util::setCachePath(cache_path);
	}
	if (argc >= 6) {
		ipc_path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(argv[5]);
		log_info << "ipc path option recieve : " << std::string(ipc_path.begin(), ipc_path.end()) << std::endl;
		Socket::set_ipc_path(ipc_path);
	}

	ProcessManager* pm = new ProcessManager();
	pm->runWatcher();

	if (pm->m_applicationCrashed)
		pm->handleCrash(path);

	delete pm;
	log_info << "=== Terminating CrashHandler ===" << std::endl;
	logging_end();
	return 0;
}




