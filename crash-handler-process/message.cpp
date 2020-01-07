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

#include "message.hpp"

Message::Message(std::vector<char> buffer) {
	m_buffer = buffer;
}

Message::~Message() {
}

bool Message::readBool() {
	bool value = reinterpret_cast<bool&> (m_buffer[index]);
	index++;
	return value;
}

uint8_t Message::readUInt8() {
	uint8_t value = reinterpret_cast<uint8_t&> (m_buffer[index]);
	index += sizeof(uint8_t);
	return value;
}

uint64_t Message::readUInt64() {
	uint64_t value = reinterpret_cast<uint64_t&> (m_buffer[index]);
	index += sizeof(uint64_t);
	return value;
}

uint32_t Message::readUInt32() {
	uint32_t value = reinterpret_cast<uint32_t&> (m_buffer[index]);
	index += sizeof(uint32_t);
	return value;
}