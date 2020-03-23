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
	buffer.resize(bytes_read < 0 ? 0 : bytes_read);
	close(file_descriptor);
	return buffer;
}

int Socket_OSX::write(const char* filename, std::vector<char> buffer) {
	int file_descriptor = open(filename, O_WRONLY | O_DSYNC);
	if (file_descriptor < 0) {
        log_info << "Could not open " << strerror(errno) << std::endl;
    }
	int bytes_wrote = ::write(file_descriptor, buffer.data(), buffer.size());
	close(file_descriptor);
	return bytes_wrote;
}

void Socket_OSX::disconnect() {
	remove(FILE_NAME);
}