#ifndef SSL_CLIENT_CLIENT_H
#define SSL_CLIENT_CLIENT_H

#include <iostream>
#include <netinet/in.h>

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/x509.h"

#include "mbedtls/net_sockets.h"
#include "mbedtls/certs.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <string.h>
#include <stdio.h>
#include <cstdlib>
#include <chrono>

const size_t MAX_RECONNECTION_ATTEMPTS = 5;
const size_t DEFAULT_PING_TIMER = 5000; /* ms */

const size_t STANDARD_RECEIVE_BUFFFER_SIZE = 204800;
const size_t STANDARD_SEND_BUFFER_SIZE = 204800;

const int BAD_INPUT_DATA = -4802;
const int CONNECTION_CLOSED = -6279;

const int DEFAULT_AUTHMODE = MBEDTLS_SSL_VERIFY_NONE;
const bool DEFAULT_DEBUG = false;

extern const char *DEFAULT_CA_PATH;
extern const char *DEFAULT_HOSTNAME;
extern const char *DEFAULT_GET_PATH;
extern const char *DEFAULT_PORT;
extern const char *PING_MESSAGE;
extern const char *PONG_MESSAGE;

enum Status
{
	CONNECTING, /* 0. Establishing connection with server */
	SSL_HANDSHAKING, /* 1. Performing sll handshake */
	HTTP_HANDSHAKING, /* 2. Sending http handshake */
	RECEIVING_HTTP_RESPONSE, /* 3. Waiting for http handshake response */
	CONNECTED, /* 4. HTTP handshake successful, you can send trading_data to server */
	HAVE_DATA /* 5. I have trading_data for you in receive_buffer, and you haven't parsed it yet */
};

struct Wrapper
{
	mbedtls_net_context server_fd; /* Wrapper type for sockets */
	mbedtls_entropy_context entropy; /* Entropy context structure */
	mbedtls_ctr_drbg_context ctr_drbg; /* The CTR_DRBG context structure */
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;
};

struct Buffer
{
	unsigned char *header;
	unsigned char *text;
	const size_t size;
	size_t current_index;
	size_t left_to_process;

	Buffer();

	Buffer(size_t);

	~Buffer();

	void ResetBuffer();
};

class WSClient
{
public:
	WSClient();

	~WSClient();

	Status GetStatus();

	Status Process();

	int ReceiveFromServer();

	int SendToServer();

	int SendToServer(const unsigned char *, size_t);

	void AddMSGToSendBuffer(const unsigned char *msg, uint64_t len);

	Buffer *GetRecvBuffer();

private:
	Status wsclient_status;
	Wrapper *conn_info;

	Buffer *send_buffer;
	Buffer *receive_buffer;
	std::chrono::time_point<std::chrono::steady_clock> wsclient_last_ping_time;
	std::string ca_path;
	std::string hostname;
	std::string port;
	std::string get_path;

	int conn_attempts;
	bool debug;
	int authmode;

	/*
	std::chrono::steady_clock::time_point clock_begin;
	 */

private:
	int SendToServerSSL();

	int ReceiveFromServerSSL();

	int InitWrapper();

	int InitCAfromfile();

	int SetHostname();

	int SetConfigDefaults();

	void SetDebug();

	int SetupSSL();

	int NetConnectSSL();

	int HandshakeSSL();

	int ServerVerify();

	void SetGetRequest();

	void HandleSendError(int);

	void HandleReceiveError(int);

	void CheckForEndOfFragment();

	void ResetBuffers();

	void ResetWrapper();

	void Ping();
};

#endif //SSL_CLIENT_CLIENT_H