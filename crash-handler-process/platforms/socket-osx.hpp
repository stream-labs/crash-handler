#include "../socket.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../logger.hpp"

class Socket_OSX : public Socket {
private:
	std::string name;
	std::string name_exit;

public:
	Socket_OSX();
	virtual ~Socket_OSX() {};

public:
	virtual std::vector<char> read() override;
	virtual int write(bool exit, std::vector<char> buffer) override;
	virtual void disconnect() override;
};