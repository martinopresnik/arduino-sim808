#include "SIM808.h"

TOKEN_TEXT(TCP_CIPMUX, "+CIPMUX=%d");
TOKEN_TEXT(TCP_CGATT, "+CGATT=%d");
TOKEN_TEXT(TCP_CSTT, "+CSTT=\"%s\",\"%s\",\"%s\"");
TOKEN_TEXT(TCP_CIICR, "+CIICR");
TOKEN_TEXT(TCP_CIFSR, "+CIFSR");
TOKEN_TEXT(TCP_CIPSEND, "+CIPSEND=%d,%d");

TOKEN_TEXT(TCP_CIPSTART, "+CIPSTART=%d,\"%s\",\"%s\",\"%d\"");
TOKEN_TEXT(TCP_CIPCLOSE, "+CIPCLOSE=%d,0");

bool SIM808::initTcpUdp(const char *apn, const char *user, const char *password) {
	bool allGood = true;
	sendFormatAT(TO_F(TOKEN_TCP_CIPMUX), 1);
	allGood &= waitResponse() == 0;
	sendFormatAT(TO_F(TOKEN_TCP_CGATT), 1);
	allGood &= waitResponse() == 0;
	sendFormatAT(TO_F(TOKEN_TCP_CSTT), apn, user, password);
	allGood &= waitResponse() == 0;
	sendFormatAT(TO_F(TOKEN_TCP_CIICR));
	allGood &= waitResponse() == 0;
	sendFormatAT(TO_F(TOKEN_TCP_CIFSR));
	allGood &= waitResponse() == 0;
	return allGood;
}

std::shared_ptr<SIM808TcpClient> SIM808::getClient(PortType portType) {
	for (int i = 0; i < PORTS_NUM; ++i) {
		if (portClients[i] == nullptr) {
			std::shared_ptr<SIM808TcpClient> r(
					new SIM808TcpClient(this, 0, portType));
			portClients[i] = r;
			return r;
		}
	}
	return nullptr;
}

int8_t SIM808::openPortConnection(
		SIM808TcpClient *client,
		const char *host,
		uint16_t port) {
	char *portTypeStr;
	if (client->portType == TCP) {
		portTypeStr = "TCP";
	} else {
		portTypeStr = "UDP";
	}
	sendFormatAT(TO_F(TOKEN_TCP_CIPSTART), client->index, portTypeStr, host,
			port);
	waitResponse();
	char indexStr[2];
	sprintf(indexStr, "%d", client->index);
	waitResponse(_httpTimeout, indexStr);

	return 0;
}

int8_t SIM808::writeToPort(
		SIM808TcpClient *client,
		const uint8_t *buf,
		size_t size) {
	sendFormatAT(TO_F(TOKEN_TCP_CIPSEND), client->index, size);
	uint32_t timeout = SIMCOMAT_DEFAULT_TIMEOUT;
	auto length = readNext(replyBuffer, BUFFER_SIZE, &timeout, '>');
	write(buf, size);

	timeout = _httpTimeout;
	char response[20];
	readNext(response, 20, &timeout, '\n');

	return size;
}

void SIM808::closePort(SIM808TcpClient *client) {
	sendFormatAT(TO_F(TOKEN_TCP_CIPCLOSE), client->index);
	waitResponse();
}

void SIM808::unexpectedResponse(char *response) {
	// Check if received something on TCP/UDP port
	char *p = strstr_P(response, TO_P("+RECEIVE"));
	if (response == p) {
		size_t dataSize;
		uint8_t portIndex;

		//+RECEIVE,0,6:

		char seperator[3];
		strcpy(seperator, ",");

		p = strstr(response, TO_P(","))+1; // Finds first char after ","
		portIndex = (uint8_t)strtoull(p, &p, 10); // p is now at 2nd ,
		p++; // move p to 2nd number

		dataSize = strtoull((char*)p, NULL, 10);

		if (portClients[portIndex] == nullptr) {
			// Port not opened, something is wrong, just clear serial buffer
			for (int i = 0; i < dataSize; ++i) {
				while (!available())
					;
				read();
			}
		}

		portClients[portIndex]->receiveBuffer.push_back(std::vector<uint8_t>());
		auto bufferChunk = &portClients[portIndex]->receiveBuffer.back();
		bufferChunk->resize(dataSize);

		RECEIVEARROW;
		for (int i = 0; i < dataSize; ++i) {
			while (!available())
				;
			uint8_t c = (uint8_t)read();
			SIM808_PRINT_CHAR(c);
			(*bufferChunk)[i] = c;
		}
		SIM808_PRINT_SIMPLE_P(TO_P(""));
	}

	// When TCP/UDP port is closed
	// Examle1: '0, CLOSED'
	// Examle1: '0, CLOSE OK'
	if(strstr_P(response, TO_P(", CLOSE")) == &response[1]){
		uint8_t index = (uint8_t)strtoul(response, NULL, 10);
		if(index >= PORTS_NUM){
			SIM808_PRINT_SIMPLE_P("Wrong index, this should never occur!!!");
		}else{
			portClients[index]->_connected = false;
		}

	}
}

