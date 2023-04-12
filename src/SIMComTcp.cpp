#include "SIMComTcp.h"

SIM808TcpClient::SIM808TcpClient(SIM808 *sim, uint8_t index, PortType portType) :
				sim(sim),
				index(index),
				portType(portType),
				bufferIndex(0),
				defaultWaitForAvailable(false),
				_connected(false){
}

int SIM808TcpClient::connect(const char *host, uint16_t port) {
	if (sim->openPortConnection(this, host, port) == 0) {
		return 0;
	}
	return 1;
}

void SIM808TcpClient::setDefaultWaitForAvailable(bool waitResponse) {
	defaultWaitForAvailable = waitResponse;
}

void SIM808TcpClient::setTransmitBufferSize(size_t size){
	if(size < transmitBuffer.size()){
		flush();
	}
	transmitBuffer.reserve(size);
}

size_t SIM808TcpClient::write(const uint8_t *buffer, size_t size) {
	return sim->writeToPort(this, buffer, size);
}

size_t SIM808TcpClient::write(uint8_t data) {
	transmitBuffer.push_back(data);
	if(transmitBuffer.size() == transmitBuffer.capacity()){
		flush();
	}
	return 1;
}

int SIM808TcpClient::available(void) {
	return available(defaultWaitForAvailable);
}

int SIM808TcpClient::available(bool check) {
	if (check) {
		sim->waitResponse(1000);
	}

	size_t size = 0;
	for (auto &chunk : receiveBuffer) {
		size += chunk.size();
	}
	return size;
}

int SIM808TcpClient::read() {
	if (receiveBuffer.size() == 0) {
		return -1;
	}
	uint8_t r = receiveBuffer.front()[bufferIndex++];
	if (bufferIndex == receiveBuffer.front().size()) {
		receiveBuffer.pop_front();
		bufferIndex = 0;
	}
	return r;
}

int SIM808TcpClient::peek() {
	if (receiveBuffer.size() == 0) {
		return -1;
	}
	return receiveBuffer.front()[bufferIndex];;
}

void SIM808TcpClient::flush() {
	write(transmitBuffer.data(), transmitBuffer.size());
	transmitBuffer.clear();
	return;
}

int SIM808TcpClient::connect(IPAddress address, uint16_t port) {
	String host = address.toString();
	return connect(host.c_str(), port);
}
int SIM808TcpClient::read(uint8_t* buffer, size_t size) {
	size_t readSize = min(size, (size_t)available(false));
	for(int i = 0; i < readSize; i++){
		buffer[i] = read();
	}
	return readSize;
}
void SIM808TcpClient::stop() {
	sim->closePort(this);
}
uint8_t SIM808TcpClient::connected() {
	return _connected;
}
