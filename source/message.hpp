#include <vector>

enum Action {
	REGISTER,
	UNREGISTER,
	EXIT
};

class Message {
public:
	Message(std::vector<char> buffer);
	~Message();
	
private:
	std::vector<char> m_buffer;
	uint64_t index = 0;
	
public:
	bool readBool();
	uint32_t readUInt32();
};