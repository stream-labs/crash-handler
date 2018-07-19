#include <vector>
#include <memory>
#include <windows.h>

#define MINIMUM_BUFFER_SIZE 512

class NamedSocket {
public:
	static std::unique_ptr<NamedSocket> create(std::string path);
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual size_t read(char* buf, size_t length) = 0;
	virtual size_t write(const char* buf, const size_t length) = 0;
	virtual bool flush() = 0;
	virtual HANDLE getHandle() = 0;
};