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

#include <ctime>
#include <time.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <stdlib.h>
#if defined(WIN32)
#include <filesystem>
#include <process.h>
#else // for __APPLE__ and other 
#include <sys/stat.h>
#include <unistd.h>
#endif

bool log_output_working = false;

std::ofstream log_output_file;
static int pid = 0;

const std::string getTimeStamp()
{
	// Use of localtime_s localtime_r reverted 
	// Posible issue with mac os 10.13
	time_t t = time(NULL);
	struct tm * buf = localtime(&t); 

	char mbstr[64] = {0};
	std::strftime(mbstr, sizeof(mbstr), "%Y%m%d:%H%M%S.", buf);
	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	std::ostringstream ss;
	ss << pid << ":" << mbstr << std::setw(3) << std::setfill('0') << now % 1000;
	return ss.str();
}

void logging_start(std::wstring & log_path)
{
	if (!log_output_working && log_path.length()) {
#if defined(WIN32)
		pid = _getpid();
		try {
			std::uintmax_t size = std::filesystem::file_size(log_path);
			if (size > 1 * 1024 * 1024) {
				std::filesystem::path log_file = log_path;
				std::filesystem::path log_file_old = log_file;
				log_file_old.replace_extension("log.old");

				std::filesystem::remove_all(log_file_old);
				std::filesystem::rename(log_file, log_file_old);
			}
		}
		catch (...)	{
		}
#else  // for __APPLE__ and other 
		pid = getpid();
		
		FILE *fp = NULL;
		long size = 0;
		std::string log_file = std::string(log_path.begin(), log_path.end());
		// ! IMPORTANT ! 
		// macOS 10.13 and older not support cpp filesystem functionality
		fp = fopen(log_file.c_str(), "r");
		if (fp != NULL) {
			if (fseek(fp, 0, SEEK_END) != -1) {
				size = ftell(fp);
    			}
			fclose(fp);
		}

		if (size > 1*1024*1024) {
			std::string log_file_old = log_file + ".old";
			remove(log_file_old.c_str());
			rename(log_file.c_str(), log_file_old.c_str());
		}
#endif 	
		log_output_file.open(log_path, std::ios_base::out | std::ios_base::app);
		if (log_output_file.is_open()) {
			log_output_working = true;
		} else {
			std::cout << "Failed to open log file, error = " << strerror(errno) << std::endl; 
		}
	}
}

void logging_end()
{
	if (!log_output_working) {
		log_output_working = false;
		log_output_file.close();
	}
}
