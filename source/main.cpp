#include "message.hpp"
#include "process.hpp"

std::vector<Process*> processes;

void main() {
	std::unique_ptr<NamedSocket> socket = 
		NamedSocket::create("\\\\.\\pipe\\slobs-crash-handler");

	bool stop = false;

	while (!stop) {
		std::vector<char> buffer; 
		buffer.resize(512);

		size_t byteRead = socket->read(buffer.data(), buffer.size());

		if (byteRead > 0) {
			Message msg(buffer);
			if (msg.getAction() == REGISTER) {
				std::vector<char> data = msg.getData();
				bool isCritical = reinterpret_cast<bool&>
					(data[0]);

				size_t pid = reinterpret_cast<size_t&>
					(data[1]);

				processes.push_back(new Process(pid, isCritical, &socket));
			}
			else if (msg.getAction() == EXIT) {
				stop = true;
				for (size_t i = 0; i < processes.size(); i++) {
					processes.at(i)->stopWorker();
				}
				for (size_t i = 0; i < processes.size(); i++) {
					processes.at(i)->getWorker()->join();
				}
			}
			else {
				throw std::exception("Invalid Action");
			}
		}
	}
}