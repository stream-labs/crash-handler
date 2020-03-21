#include "../socket.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../logger.hpp"

#define FILE_NAME "/tmp/slobs-crash-handler"

class Socket_OSX : public Socket {
public:
	Socket_OSX() {};
	virtual ~Socket_OSX() {};

public:
	virtual std::vector<char> read() override;
	virtual void disconnect()        override;
};