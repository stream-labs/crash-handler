#include "namedsocket-win.hpp"

NamedSocket_win::NamedSocket_win(std::string path) {
	m_handle= CreateNamedPipe(path.c_str(),
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE,
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

	if (errorCode == ERROR_SUCCESS) {
		return bytesRead;
	}
	else {
		return -1;
	}
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