#include <memory>
#include <windows.h>
#include "message.hpp"
#include "process.hpp"

#define MINIMUM_BUFFER_SIZE 512

class NamedSocket {
public:
	static std::unique_ptr<NamedSocket> create();
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual bool read(std::vector<Process*>*) = 0;
	virtual bool flush() = 0;
	virtual HANDLE getHandle() = 0;
};