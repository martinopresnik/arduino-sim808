#include "SIM808.h"

TOKEN(RDY);

SIM808::SIM808(uint8_t resetPin, uint8_t pwrKeyPin, uint8_t statusPin)
{
	_resetPin = resetPin;
	_pwrKeyPin = pwrKeyPin;
	_statusPin = statusPin;
	_httpTimeout = HTTP_TIMEOUT;

	pinMode(_resetPin, OUTPUT);
	if(_pwrKeyPin != SIM808_UNAVAILABLE_PIN) pinMode(_pwrKeyPin, OUTPUT);
	if (_statusPin != SIM808_UNAVAILABLE_PIN) pinMode(_statusPin, INPUT);
	
	if(_pwrKeyPin != SIM808_UNAVAILABLE_PIN) digitalWrite(_pwrKeyPin, HIGH);
	digitalWrite(_resetPin, HIGH);
	_userAgent = NULL;

	for(int i = 0; i < PORTS_NUM; ++i){
		portClients[i] = nullptr;
	}
}

SIM808::~SIM808() {
	if(_userAgent != NULL){
		delete[] _userAgent;
	}
}

#pragma region Public functions

void SIM808::init()
{
	SIM808_PRINT_SIMPLE_P("Init...");

	reset();
	waitForReady();
	delay(1500);

	setEcho(SIM808Echo::Off);
}

void SIM808::reset()
{
	digitalWrite(_resetPin, HIGH);
	delay(10);
	digitalWrite(_resetPin, LOW);
	delay(200);

	digitalWrite(_resetPin, HIGH);
}

void SIM808::waitForReady()
{
	// we got AT, waiting for RDY
	//while (waitResponse(TO_F(TOKEN_RDY)) != 0);
	do
	{
		SIM808_PRINT_SIMPLE_P("Waiting for echo...");
		sendAT(S_F(""));
	// Despite official documentation, we can get an "AT" back without a "RDY" first.
	} while (waitResponse(TO_F(TOKEN_AT), TO_F(TOKEN_OK)) < 0);

}

bool SIM808::setEcho(SIM808Echo mode)
{
	sendAT(S_F("E"), (uint8_t)mode);

	return waitResponse() == 0;
}

size_t SIM808::sendCommand(const char *cmd, char *response, size_t responseSize)
{
	flushInput();
	sendAT(cmd);
	
	uint32_t timeout = SIMCOMAT_DEFAULT_TIMEOUT;
	readNext(response, responseSize, &timeout);
}

#pragma endregion


