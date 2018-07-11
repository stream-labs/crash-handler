#include <vector>

enum Action {
	REGISTER,
	HEARTBEAT,
	EXIT
};

class Message {
public:
	Message(std::vector<char> buffer);
	~Message();
	
private:
	Action m_action;
	std::vector<char> m_data;
	
public:
	Action getAction(void);
	void setAction(Action action);
	std::vector<char> getData(void);
	void setData(std::vector<char> data);

public:
	std::vector<char> serialize(void);
	void deserialize(std::vector<char> buffer);
};