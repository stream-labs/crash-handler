#include "namedsocket-win.hpp"

NamedSocket_win::NamedSocket_win(std::string path) {
	m_handle= CreateNamedPipe(path.c_str(),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
		PIPE_UNLIMITED_INSTANCES,
		0,
		MINIMUM_BUFFER_SIZE,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	if (m_handle == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		throw std::runtime_error("Unable to create socket.");
	}
}

std::unique_ptr<NamedSocket> NamedSocket::create(std::string path) {
	return std::make_unique<NamedSocket_win>(path);
}

NamedSocket_win::~NamedSocket_win() {
	DisconnectNamedPipe(m_handle);
}

size_t NamedSocket_win::read(char* buf, size_t length) {
	DWORD bytesRead = 0;
	DWORD errorCode = 0;

	if (length > MINIMUM_BUFFER_SIZE)
		return -1;

	// Attempt to read from the handle.
	SetLastError(ERROR_SUCCESS);
	ReadFile(m_handle, buf, (DWORD)length, &bytesRead, NULL);

	// Test for actual return code.
	errorCode = GetLastError();

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	return bytesRead;
}

size_t NamedSocket_win::write(const char* buf, const size_t length) {
	DWORD bytesWritten = 0;

	SetLastError(ERROR_SUCCESS);
	WriteFile(m_handle, buf, (DWORD)length, &bytesWritten, NULL);

	DWORD res = GetLastError();
	if (res == ERROR_SUCCESS) {
		return bytesWritten;
	}
	else {
		return -1;
	}
}

bool NamedSocket_win::flush() {
	return true;
}