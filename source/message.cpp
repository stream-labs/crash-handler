#include "message.hpp"

Message::Message(std::vector<char> buffer) {
	deserialize(buffer);
}

std::vector<char> Message::serialize(void) {
	std::vector<char> buffer;

	size_t offset = 0;

	buffer[offset] = uint8_t(m_action);
	offset += sizeof(uint8_t);

	buffer[offset] = sizeof(m_data.size());
	offset += sizeof(size_t);

	std::memcpy(buffer.data() + offset, m_data.data(), m_data.size());

	return buffer;
}

 void Message::deserialize(std::vector<char> buffer) {
	size_t offset = 0;

	// Get Action
	m_action = reinterpret_cast<Action*> (buffer.data() + offset);
	offset += sizeof(Action);

	// Get Data
	size_t* length =
		reinterpret_cast<size_t*> (buffer.data() + offset);
	offset += sizeof(size_t);

	m_data.resize(*length);
	memcpy(m_data.data(), buffer.data() + offset,
		*length);
}