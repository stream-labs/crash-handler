#include <vector>

enum Action {
	REGISTER,
	HEARTBEAT
};

class Message {
public:
	Message(std::vector<char> buffer);
	~Message();
	
private:
	Action* m_action;
	std::vector<char> m_data;
	
public:
	std::vector<char> serialize(void);
	void deserialize(std::vector<char> buffer);
};