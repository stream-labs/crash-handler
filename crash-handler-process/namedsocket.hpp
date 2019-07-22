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

#include <memory>
#include <windows.h>
#include "message.hpp"
#include "process.hpp"

#define MINIMUM_BUFFER_SIZE 512

class NamedSocket {
public:
	static std::unique_ptr<NamedSocket> create();
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual bool read(std::vector<Process*>*, std::mutex* mu, bool* exit) = 0;
	virtual bool flush() = 0;
	virtual HANDLE getHandle() = 0;
};