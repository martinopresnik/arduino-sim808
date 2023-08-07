#pragma once

#include "Arduino.h"
#include <list>
#include <vector>

enum PortType {
	TCP, UDP
};

enum TcpStatus{
	INITIAL=0,
	CONNECTING=1,
	CONNECTED=2,
	REMOTE_CLOSING=3,
	CLOSING=4,
	CLOSED=5,
	UNKNOWN=6
};

#define PORTS_NUM 6

#include "SIM808.h"

class SIM808;

class SIM808TcpClient: public Client {
	friend class SIM808;

public:
	int connect(const char *host, uint16_t port);

	/**
	 * Sets default parameter to available() method.
	 */
	void setDefaultWaitForAvailable(bool waitResponse);

	// Used only when transmitting by one byte!
	void setTransmitBufferSize(size_t size);

	// Print override
	size_t write(const uint8_t *buffer, size_t size) override;
	size_t write(uint8_t data) override;

	// Stream override
	int available(void) override;
	/**
	 * Checks if anything was received on TCP/UDP port.
	 *
	 * @param waitResponse If true, it will wait for some time before returning to check if something was received.
	 * Should be set to true if only TCP/UDP connections are used, and can be set to false if other sim commands are called frequently.
	 * @return size of received data
	 */
	int available(bool waitResponse);
	int read() override;
	int peek() override;
	void flush() override;

	// Client override
	int connect(IPAddress address, uint16_t port) override;
	int read(uint8_t* buffer, size_t size) override;
	void stop() override;
	uint8_t connected() override;
	operator bool() {
		return connected();
	}
	TcpStatus status();

protected:
	const uint8_t index;
	SIM808TcpClient(SIM808 *sim, uint8_t index, PortType portType);
	const PortType portType;
	SIM808 *sim;
	bool defaultWaitForAvailable;
	size_t transmitBufferSize;
	std::vector<uint8_t> transmitBuffer;
	std::list<std::vector<uint8_t>> receiveBuffer;
	size_t bufferIndex;
	bool _connected;
	enum SendType {NONE, SUCCESS, ERROR} sendType;
};

