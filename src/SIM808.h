#pragma once

#include <SIMComAT.h>
#include "SIM808.Types.h"
#include "SIMComTcp.h"
#include <memory>;

#define HTTP_TIMEOUT 10000L
#define GPS_ACCURATE_FIX_MIN_SATELLITES 4
#define SIM808_UNAVAILABLE_PIN 255

class SIM808TcpClient;

class SIM808 : public SIMComAT
{
	friend SIM808TcpClient;
protected:
	uint8_t _resetPin;
	uint8_t _statusPin;
	uint8_t _pwrKeyPin;
	char* _userAgent;
	uint32_t _httpTimeout;
	bool _gprsEnabled;

	std::shared_ptr<SIM808TcpClient> portClients[PORTS_NUM];

	/**
	 * Wait for the device to be ready to accept communcation.
	 */
	void waitForReady();	

	/**
	 * Set all the parameters up for a HTTP request to be fired next.
	 */
	bool setupHttpRequest(const char* url);
	/**
	 * Fire a HTTP request and return the server response code and body size.
	 */
	bool fireHttpRequest(const SIM808HttpAction action, uint16_t *statusCode, size_t *dataSize);
	/**
	 * Read the last HTTP response body into response.
	 */
	bool readHttpResponse(char *response, size_t responseSize, size_t dataSize);	
	bool setHttpParameter(ATConstStr parameter, ATConstStr value);
#if defined(__AVR__)
	bool setHttpParameter(ATConstStr parameter, const char * value);
#endif
	bool setHttpParameter(ATConstStr parameter, uint8_t value);
	/**
	 * Set the HTTP body of the next request to be fired.
	 */
	bool setHttpBody(const char* body);
	/**
	 * Initialize the HTTP service.
	 */
	bool httpInit();
	/**
	 * Terminate the HTTP service.
	 */
	bool httpEnd();
	/**
	 * Set one of the bearer settings for application based on IP.
	 */
	bool setBearerSetting(ATConstStr parameter, const char* value);

	void setHttpTimeout(uint32_t timeout);


	int8_t openPortConnection(SIM808TcpClient *client, const char* host, uint16_t port);
	int8_t writeToPort(SIM808TcpClient *client, const uint8_t *buf, size_t size, bool waitForTransmission = true);
	TcpStatus portStatus(SIM808TcpClient *client);
	void closePort(SIM808TcpClient *client);
	void unexpectedResponse(char* response) override;
public:
	SIM808(uint8_t resetPin, uint8_t pwrKeyPin = SIM808_UNAVAILABLE_PIN, uint8_t statusPin = SIM808_UNAVAILABLE_PIN);
	~SIM808();	

	/**
	 * Get a boolean indicating wether or not the device is currently powered on.
	 * The power state is read from either the statusPin if set, or from a test AT command response.
	 */
	bool powered();
	/**
	 * Power on or off the device only if the requested state is different than the actual state.
	 * Returns true if the power state has been changed as a result of this call.
	 * Unavailable and returns false in all cases if pwrKeyPin is not set.
	 * 
	 * See powered()
	 */
	bool powerOnOff(bool power);
	/**
	 * Get current charging state, level and voltage from the device.
	 */
	SIM808ChargingStatus getChargingState();

	/**
	 * Get current phone functionality mode.
	 */
	SIM808PhoneFunctionality getPhoneFunctionality();
	/**
	 * Set the phone functionality mode.
	 */
	bool setPhoneFunctionality(SIM808PhoneFunctionality fun);
	/**
	 * Configure slow clock, allowing the device to enter sleep mode.
	 */
	bool setSlowClock(SIM808SlowClock mode);

	void init();
	void reset();

	/**
	 * Send an already formatted command and read a single line response. Useful for unimplemented commands.
	 */
	size_t sendCommand(const char* cmd, char* response, size_t responseSize);

	bool setEcho(SIM808Echo mode);
	/**
	 * Unlock the SIM card using the provided pin. Beware of failed attempts !
	 */
	bool simUnlock(const char* pin);

	/**
	 * Removes pin code. Should first unlock it using simUnlock.
	 */
	bool simRemovePin(const char* pin);
	/**
	 * Get a string indicating the current sim state.
	 */
	size_t getSimState(char* state, size_t stateSize);
	/**
	 * Get the device IMEI number.
	 */
	size_t getImei(char* imei, size_t imeiSize);

	/**
	 * Get current GSM signal quality, estimated attenuation in dB and error rate.
	 */
	SIM808SignalQualityReport getSignalQuality();

	bool setSmsMessageFormat(SIM808SmsMessageFormat format);
	/**
	 * Send a SMS to the provided number.
	 */
	bool sendSms(const char* addr, const char* msg);

	/**
	 * Get a boolean indicating wether or not GPRS is currently enabled.
	 */
	bool getGprsPowerState(bool *state);
	/**
	 * Reinitiliaze and enable GPRS.
	 */
	bool enableGprs(const char* apn, const char* user = NULL, const char *password = NULL);
	/**
	 * Shutdown GPRS properly.
	 */
	bool disableGprs();

	bool gprsEnabled();

	/*
	 * LTS system receives time from mobile network and forwards it to GPS.
	 * Check if LTS is enabled.
	 */
	bool ltsTimeEnabled();

	/*
	 * Enable or disable LTS.
	 */
	void ltsEnable(bool enable);
	/**
	 * Get the device current network registration status.
	 */
	SIM808NetworkRegistrationState getNetworkRegistrationStatus();

	/**
	 * Get a boolean indicating wether or not GPS is currently powered on.
	 */
	bool getGpsPowerState(bool *state);
	/**
	 * Power on or off the gps only if the requested state is different than the actual state.
	 * Returns true if the power state has been changed has a result of this call. 
	 */
	bool powerOnOffGps(bool power);
	/**
	 * Get the latest GPS parsed sequence and a value indicating the current
	 * fix status. 
	 * Response is only filled if a fix is acquired.
	 * If a fix is acquired, FIX or ACCURATE_FIX will be returned depending on 
	 * wether or not the satellites used is greater than minSatellitesForAccurateFix.
	 */
	SIM808GpsStatus getGpsStatus(char * response, size_t responseSize, uint8_t minSatellitesForAccurateFix = GPS_ACCURATE_FIX_MIN_SATELLITES);
	/**
	 * Extract the specified field from the GPS parsed sequence as a uint16_t.
	 */
	bool getGpsField(const char* response, SIM808GpsField field, uint16_t* result);
	/**
	 * Extract the specified field from the GPS parsed sequence as a float.
	 */
	bool getGpsField(const char* response, SIM808GpsField field, float* result);
	/**
	 * Return a pointer to the specified field from the GPS parsed sequence.
	 */
	void getGpsField(const char* response, SIM808GpsField field, char** result);
	/**
	 * Get and return the latest GPS parsed sequence.
	 */
	bool getGpsPosition(char* response, size_t responseSize);


	void setHttpUserAgent(const char* value);
	/**
	 * Send an HTTP GET request and read the server response within the limit of responseSize.
	 * 
	 * HTTP and HTTPS are supported, based on he provided URL. Note however that HTTPS request
	 * have a high failure rate that make them unusuable reliably.
	 */
	uint16_t httpGet(const char* url, char* response, size_t responseSize);
	/**
	 * Send an HTTP POST request and read the server response within the limit of responseSize.
	 * 
	 * HTTP and HTTPS are supported, based on he provided URL. Note however that HTTPS request
	 * have a high failure rate that make them unusuable reliably.
	 */
	uint16_t httpPost(const char* url, ATConstStr contentType, const char* body, char* response, size_t responseSize);	

	/**
	 * Set APN data and other parameters for TCP/UDP connections.
	 * Returns true if initialized without errors.
	 */
	bool initTcpUdp(const char *apn, const char* user, const char *password);

	std::shared_ptr<SIM808TcpClient> getClient(uint8_t number = -1, PortType portType=TCP);

};

