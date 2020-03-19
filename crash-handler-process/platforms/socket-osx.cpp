#include "socket-osx.hpp"

std::unique_ptr<Socket> Socket::create() {
	remove(FILE_NAME);
    if (mkfifo(FILE_NAME, S_IRUSR | S_IWUSR) < 0)
		log_info << "Could not create " << strerror(errno) << std::endl;


	return std::make_unique<Socket_OSX>();
}

std::vector<char> Socket_OSX::read() {
	std::vector<char> buffer;
	buffer.resize(30000);
	int file_descriptor = open(FILE_NAME, O_RDONLY);
	if (file_descriptor < 0) {
        log_info << "Could not open " << strerror(errno) << std::endl;
    }
	int bytes_read = ::read(file_descriptor, buffer.data(), buffer.size());
	buffer.resize(bytes_read);
	return buffer;
}