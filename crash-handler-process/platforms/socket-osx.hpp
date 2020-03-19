#include "../socket.hpp"

class Socket_OSX : public Socket {
public:
	Socket_OSX() {};
	virtual ~Socket_OSX() {};

public:
	virtual std::vector<char> read() override;
};