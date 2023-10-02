#include "SIM808.h"

TOKEN_TEXT(TCP_CIPMUX, "+CIPMUX=%d");
TOKEN_TEXT(TCP_CGATT, "+CGATT=%d");
TOKEN_TEXT(TCP_CSTT, "+CSTT=\"%s\",\"%s\",\"%s\"");
TOKEN_TEXT(TCP_CIICR, "+CIICR");
TOKEN_TEXT(TCP_CIFSR, "+CIFSR");
TOKEN_TEXT(TCP_CIPSEND, "+CIPSEND=%d,%d");

TOKEN_TEXT(TCP_CIPSTATUS, "+CIPSTATUS=%d");

TOKEN_TEXT(TCP_CIPSTART, "+CIPSTART=%d,\"%s\",\"%s\",\"%d\"");
TOKEN_TEXT(TCP_CIPCLOSE, "+CIPCLOSE=%d,0");
TOKEN_TEXT(TCP_CIPACK, "+CIPACK=%d");

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
	//allGood &= waitResponse() == 0;
	waitResponse();
	return allGood;
}

std::shared_ptr<SIM808TcpClient> SIM808::getClient(uint8_t number, PortType portType) {
	if(number == -1){
		for (int i = 0; i < PORTS_NUM; ++i) {
			if (portClients[i] == nullptr){
				number = i;
				break;
			}
		}
		if(number == -1){
			return nullptr;
		}
	}

	if (portClients[number] == nullptr) {
		std::shared_ptr<SIM808TcpClient> r(
				new SIM808TcpClient(this, 0, portType));
		portClients[number] = r;
	}
	return portClients[number];
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
	auto responseIndex = waitResponse(_httpTimeout, indexStr, TO_F(TOKEN_ERROR));
	client->_connected = responseIndex == 0;
	return responseIndex;
}

int8_t SIM808::writeToPort(
		SIM808TcpClient *client,
		const uint8_t *buf,
		size_t size,
		bool waitForTransmission) {
	uint32_t timeout = _httpTimeout;

	// Variable will be changed in "unexpectedResponse"
	client->transmissionState = SIM808TcpClient::TransmissionState::IN_PROGRESS;
	sendFormatAT(TO_F(TOKEN_TCP_CIPSEND), client->index, size);
	auto length = readNext(replyBuffer, BUFFER_SIZE, &timeout, '>');
	write(buf, size);


	client->transmissionsToConfirm++;
	//char response[20];
	//readNext(response, 20, &timeout, '\n');
	if(waitForTransmission){
		while(timeout && client->transmissionState == SIM808TcpClient::TransmissionState::IN_PROGRESS){
			waitResponse(timeout, NULL, NULL, NULL, NULL);
		}
		if(client->transmissionState != SIM808TcpClient::TransmissionState::SUCCESS){
			closePort(client);
			client->_connected = false;
			return -1;
		}
	}
	return size;
}

TcpStatus SIM808::portStatus(SIM808TcpClient *client){
	uint32_t timeout = 5000;
	bool receivedOk = false;
	bool receivedStatus = false;
	sendFormatAT(TOKEN_TCP_CIPSTATUS, client->index);

	do{
		auto response = waitResponse(&timeout, "+CIPSTATUS", "OK");
		receivedOk = response == 1 || receivedOk;
		receivedStatus = response == 0;
		if(!timeout){
			return UNKNOWN;
		}
		/*
		if(receivedOk){
			Serial.println("Received OK");
		}
		Serial.print("Len: ");
		Serial.println(strlen(replyBuffer));
		Serial.println(replyBuffer);
		*/
	}while(!receivedStatus);

	char* status = find(replyBuffer, ',', 5);
	if(status == NULL){
		return UNKNOWN;
	}

	char* statusOptions[] = {
			"\"INITIAL\"",
			"\"CONNECTING\"",
			"\"CONNECTED\"",
			"\"REMOTE CLOSING\"",
			"\"CLOSING\"",
			"\"CLOSED\""
	};

	TcpStatus result = UNKNOWN;
	for(int i = 0; i < 6; i++){
		if(strstr(status, statusOptions[i]) != NULL){
			result = (TcpStatus)i;
			break;
		}
	}

	while(!receivedOk){
		receivedOk = waitResponse(timeout) == 0;
		if(!timeout){
			return UNKNOWN;
		}
	}

	return result;
}

void SIM808::closePort(SIM808TcpClient *client) {
	sendFormatAT(TO_F(TOKEN_TCP_CIPCLOSE), client->index);
	waitResponse();
}

int16_t SIM808::nonAcknowlidgedBytes(SIM808TcpClient *client){
	sendFormatAT(TO_F(TOKEN_TCP_CIPACK), client->index);
	if(waitResponse("+CIPACK") != 0){
		return -1;
	}
	int16_t result;
	parse(replyBuffer, ',', 2, &result);
	return result;
}

void SIM808::unexpectedResponse(char *response) {
	// Check if received something on TCP/UDP port
	char *receive = strstr_P(response, TO_P("+RECEIVE"));
	if (response == receive) {
		// Receiving something on TCP/UDP port
		size_t dataSize;
		uint8_t portIndex;

		//+RECEIVE,0,6:

		char seperator[3];
		strcpy(seperator, ",");

		receive = strstr(response, TO_P(",")); // Find ","
		if(receive == nullptr){
			SIM808_PRINT("Missing part of line, this should not happen!");
			return;
		}
		receive = receive+1; // Move to first char after ","
		portIndex = (uint8_t)strtoull(receive, &receive, 10); // p is now at 2nd ,
		receive++; // move p to 2nd number

		dataSize = strtoull((char*)receive, NULL, 10);

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
		bufferChunk->reserve(dataSize);


		double tmpTimeout = dataSize * 1.05; // Calculated for 9600 baudrate, change for other baudrates
		unsigned long timeout = (unsigned long)(tmpTimeout) + 5000; // Add 5sec for reliability


		auto receiveStartTime = millis();

		RECEIVEARROW;
		for (volatile int i = 0; i < dataSize; ++i) {
			while (!available()){
				if(millis() - receiveStartTime > timeout){
					Serial.println("TCP/UDP Receive timeout!");
					portClients[portIndex]->stop();
					return;
				}
			}
			uint8_t c = (uint8_t)read();
			SIM808_PRINT_CHAR(c);
			bufferChunk->push_back(c);
		}
		SIM808_PRINT_SIMPLE_P(TO_P(""));
	}
	char *pdpDeact = strstr_P(response, TO_P("+PDP: DEACT"));
	if (response == pdpDeact) {
		disableGprs();
	}

	// if it starts with number, it's something with TCP/UDP
	if(isdigit(response[0])){

		uint8_t index = (uint8_t)strtoul(response, NULL, 10);
		if(index >= PORTS_NUM){
			SIM808_PRINT_SIMPLE_P("Wrong index, this should never occur!!!");
			return;
		}

		// When TCP/UDP port is closed
		// Examle1: '0, CLOSED'
		// Examle1: '0, CLOSE OK'
		if(strstr_P(response, TO_P(", CLOSE")) == &response[1]){
			portClients[index]->_connected = false;
			portClients[index]->transmissionState = SIM808TcpClient::TransmissionState::NONE;
		}


		if(strstr_P(response, TO_P(", SEND OK")) == &response[1]){
			portClients[index]->transmissionsToConfirm--;
			Serial.print("transmissionsToConfirm: ");
			Serial.println(portClients[index]->transmissionsToConfirm);
			if(portClients[index]->transmissionsToConfirm <= 0){
				portClients[index]->transmissionState = SIM808TcpClient::TransmissionState::SUCCESS;
			}
		}

		if(strstr_P(response, TO_P(", ERROR")) == &response[1]){
			portClients[index]->transmissionState = SIM808TcpClient::TransmissionState::ERROR;
		}
	}

}

