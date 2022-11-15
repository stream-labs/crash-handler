#include "socket-osx.hpp"

std::wstring Socket_OSX::ipc_path;

void Socket::set_ipc_path(const std::wstring &new_ipc_path)
{
	Socket_OSX::ipc_path = new_ipc_path;
}

Socket_OSX::Socket_OSX()
{
	this->name = std::string(ipc_path.begin(), ipc_path.end());
	this->name_exit = "/tmp/exit-slobs-crash-handler";

	remove(this->name.c_str());
	if (mkfifo(this->name.c_str(), S_IRUSR | S_IWUSR) < 0) {
		initialization_failed = true;
		log_info << "Could not create " << strerror(errno) << std::endl;
	} else {
		log_info << "Pipe created " << this->name << std::endl;
	}
}

std::unique_ptr<Socket> Socket::create()
{
	return std::make_unique<Socket_OSX>();
}

std::vector<char> Socket_OSX::read()
{
	// Previously, |open| blocked if there was not any data.
	// So the process did not exit when it had to.
	// The workaround is O_NONBLOCK + select + 500 ms timeout.
	// It emulates the Windows implementation behavior.

	std::vector<char> buffer;
	int file_descriptor = open(this->name.c_str(), O_RDONLY|O_NONBLOCK);
	if (file_descriptor < 0) {
		log_info << "Could not open; |open| error: " << strerror(errno) << std::endl;
		return buffer;
	}

	fd_set set;
	FD_ZERO(&set);
	FD_SET(file_descriptor, &set);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000; // 500 ms

	int rv = select(file_descriptor + 1, &set, NULL, NULL, &timeout);
	if (rv <= 0) {
		if (rv < 0) {
			log_info << "Could not open; |select| error: " << strerror(errno) << std::endl;
		}
		close(file_descriptor);
		return buffer;
	}

	buffer.resize(30000, 0);
	int bytes_read = ::read(file_descriptor, buffer.data(), buffer.size());
	buffer.resize(bytes_read < 0 ? 0 : bytes_read);
	close(file_descriptor);

	return buffer;
}

int Socket_OSX::write(bool exit, std::vector<char> buffer)
{
	int file_descriptor = open(exit ? this->name_exit.c_str() : this->name.c_str(), O_WRONLY | O_DSYNC);
	if (file_descriptor < 0) {
		log_info << "Could not open " << strerror(errno) << std::endl;
	}
	int bytes_wrote = ::write(file_descriptor, buffer.data(), buffer.size());
	close(file_descriptor);
	return bytes_wrote;
}

void Socket_OSX::disconnect()
{
	remove(this->name.c_str());
}