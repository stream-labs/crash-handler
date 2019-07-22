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

#include "namedsocket.hpp"
#include <vector>

class NamedSocket_win : public NamedSocket {
public:
	NamedSocket_win();
	virtual ~NamedSocket_win();

public:
	HANDLE m_handle;

public:
	virtual bool connect() override;
	virtual bool read(std::vector<Process*>*, std::mutex* mu, bool* exit) override;
	virtual void disconnect() override;
	virtual bool flush() override;

	virtual HANDLE getHandle() override;
};