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
	std::vector<char> buffer;
	buffer.resize(30000, 0);
	int file_descriptor = open(this->name.c_str(), O_RDONLY);
	if (file_descriptor < 0) {
		log_info << "Could not open " << strerror(errno) << std::endl;
		return buffer;
	}
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