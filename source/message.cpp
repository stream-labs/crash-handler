#include "message.hpp"

Message::Message(std::vector<char> buffer) {
	deserialize(buffer);
}

Message::~Message() {
}

std::vector<char> Message::serialize(void) {
	std::vector<char> buffer;
	buffer.resize(512);

	size_t offset = 0;
	
	*reinterpret_cast<uint8_t*>(buffer.data() + offset) = uint8_t(m_action);

	offset += sizeof(uint8_t);

	*reinterpret_cast<size_t*>(buffer.data() + offset) = m_data.size();
	offset += sizeof(size_t);

	std::memcpy(buffer.data() + offset, m_data.data(), m_data.size());

	return buffer;
}

 void Message::deserialize(std::vector<char> buffer) {
	size_t offset = 0;

	// Get Action
	m_action = (Action)buffer[offset]; offset++;	

	// Get Data
	size_t* length =
		reinterpret_cast<size_t*> (buffer.data() + offset);
	offset += sizeof(size_t);

	m_data.resize(*length + 1);
	memcpy(m_data.data(), buffer.data() + offset,
		*length + 1);
}

 Action Message::getAction(void) {
	 return m_action;
 }

 void Message::setAction(Action action) {
	 m_action = action;
 }

 std::vector<char> Message::getData(void) {
	 return m_data;
 }

 void Message::setData(std::vector<char> data) {
	 m_data = data;
 }