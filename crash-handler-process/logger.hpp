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

#pragma once

#include <iostream>
#include <fstream>
#include <string>


const std::string getTimeStamp();

extern std::wstring log_output_path;
extern std::ofstream log_output_file;
extern bool log_output_working;

#define log_info if (!log_output_working) {} else log_output_file << "INF:" << getTimeStamp() <<  ": " 
#define log_debug if (!log_output_working) {} else log_output_file << "DBG:" << getTimeStamp() <<  ": "
#define log_error if (!log_output_working) {} else log_output_file << "ERR:" << getTimeStamp() <<  ": "

void logging_start(std::wstring & log_path);
void logging_end();