#include "socket-osx.hpp"

std::unique_ptr<Socket> Socket::create() {
	return std::make_unique<Socket_OSX>();
}

std::vector<char> Socket_OSX::read() {

}