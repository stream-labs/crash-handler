#include <vector>
#include <memory>

#define MINIMUM_BUFFER_SIZE 512

class NamedSocket {
public:
	static std::unique_ptr<NamedSocket> create(std::string path);
	virtual size_t read(char* buf, size_t length) = 0;
	virtual size_t write(const char* buf, const size_t length) = 0;
	virtual bool flush() = 0;
};