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

uint32_t Message::readUInt32() {
	uint32_t value = reinterpret_cast<uint64_t&> (m_buffer[index]);
	index += sizeof(uint64_t);
	return value;
}