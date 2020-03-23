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
#ifdef WIN32
#include <windows.h>
#endif
#include "message.hpp"

#define MINIMUM_BUFFER_SIZE 512

#ifdef __APPLE__
#define FILE_NAME "/tmp/slobs-crash-handler"
#define FILE_NAME_EXIT "/tmp/exit-slobs-crash-handler"
#endif

class Socket {
public:
    static std::unique_ptr<Socket> create();

    virtual std::vector<char> read() = 0;
    virtual int write(const char* filename, std::vector<char> buffer) = 0;
    virtual void disconnect() = 0;
};