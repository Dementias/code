#include "WSClient.h"
#include "WSUtils.h"

const char *DEFAULT_CA_PATH = "/etc/ssl/certs";
const char *DEFAULT_HOSTNAME = "ws.bitmex.com";
const char *DEFAULT_GET_PATH = "/realtime";
const char *DEFAULT_PORT = "443";

const char *PING_MESSAGE = "ping";
const char *PONG_MESSAGE = "pong";

Buffer::Buffer() : size(0), text(nullptr), header(nullptr),
                   current_index(0), left_to_process(0)
{

}

Buffer::Buffer(size_t size) :
		size(size), current_index(0), left_to_process(0)
{
	header = new(std::nothrow) unsigned char[MAX_HEADER_SIZE + size];
	text = header + MAX_HEADER_SIZE;

	if (header == nullptr)
	{
		perror("Buffer(): Could not allocate memory \n");
		exit(1);
	}
}

Buffer::~Buffer()
{
	delete[] header;
}

void Buffer::ResetBuffer()
{
	memset(header, 0, size + MAX_HEADER_SIZE);
	current_index = 0;
	left_to_process = 0;
}

Status WSClient::GetStatus()
{
	return wsclient_status;
}

void WSClient::ResetBuffers()
{
	receive_buffer->ResetBuffer();
	send_buffer->ResetBuffer();
}

WSClient::WSClient() :
		wsclient_status(CONNECTING), ca_path(DEFAULT_CA_PATH), hostname(DEFAULT_HOSTNAME),
		port(DEFAULT_PORT), get_path(DEFAULT_GET_PATH), debug(DEFAULT_DEBUG),
		authmode(DEFAULT_AUTHMODE), conn_attempts(0), wsclient_last_ping_time(std::chrono::steady_clock::now())
{
	conn_info = new(std::nothrow) Wrapper();
	receive_buffer = new(std::nothrow) Buffer(STANDARD_RECEIVE_BUFFFER_SIZE);
	send_buffer = new(std::nothrow) Buffer(STANDARD_SEND_BUFFER_SIZE);

	if ((conn_info == nullptr || receive_buffer == nullptr || send_buffer == nullptr)
	    || (receive_buffer->text == nullptr) || (send_buffer->text == nullptr))
	{
		perror("WSClient: Could not allocate memory \n");
		exit(1);
	}
}

WSClient::~WSClient()
{
	delete conn_info;
	delete send_buffer;
	delete receive_buffer;
}

int WSClient::InitWrapper()
{
	mbedtls_ssl_init(&(conn_info->ssl));
	mbedtls_ssl_config_init(&(conn_info->conf));
	mbedtls_x509_crt_init(&(conn_info->cacert));
	mbedtls_ctr_drbg_init(&(conn_info->ctr_drbg));
	mbedtls_entropy_init(&(conn_info->entropy));

	int ret;
	const char *pers = "ssl_client";
	if ((ret = mbedtls_ctr_drbg_seed(&(conn_info->ctr_drbg), mbedtls_entropy_func,
	                                 &(conn_info->entropy), (const unsigned char *)pers,
	                                 strlen(pers))) != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}
	return ret;
}

int WSClient::InitCAfromfile()
{
	int ret;
	ret = mbedtls_x509_crt_parse_path(&(conn_info->cacert), ca_path.c_str());

	if (ret != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}

	return ret;
}

int WSClient::SetHostname()
{
	int ret;

	ret = mbedtls_ssl_set_hostname(&(conn_info->ssl), hostname.c_str());

	if (ret != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}

	return ret;
}

int WSClient::SetConfigDefaults()
{
	int ret;
	ret = mbedtls_ssl_config_defaults(&(conn_info->conf), MBEDTLS_SSL_IS_CLIENT,
	                                  MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);

	if (ret == 0)
	{
		mbedtls_ssl_conf_authmode(&(conn_info->conf), authmode);

		if (authmode == MBEDTLS_SSL_VERIFY_REQUIRED || authmode == MBEDTLS_SSL_VERIFY_OPTIONAL)
		{
			mbedtls_ssl_conf_ca_chain(&(conn_info->conf), &(conn_info->cacert), nullptr);
		}

		mbedtls_ssl_conf_rng(&(conn_info->conf), mbedtls_ctr_drbg_random, &(conn_info->ctr_drbg));
	}

	else
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}

	return ret;
}

Status WSClient::Process()
{
	if (conn_attempts > MAX_RECONNECTION_ATTEMPTS)
	{
		perror("WSClient became INVALID\n");
		exit(1);
	}

	switch (wsclient_status)
	{
		case CONNECTING:
		{
			if (InitWrapper() != 0)
			{
				break;
			}
			if (InitCAfromfile() != 0)
			{
				break;
			}
			if (SetHostname() != 0)
			{
				break;
			}
			if (SetConfigDefaults() != 0)
			{
				break;
			}
			if (debug)
			{
				SetDebug();
			}
			if (SetupSSL() != 0)
			{
				break;
			}
			if (NetConnectSSL() != 0)
			{
				break;
			}
		}
		case SSL_HANDSHAKING:
		{
			if (HandshakeSSL() != 0)
			{
				break;
			}
			if (ServerVerify() != 0)
			{
				break;
			}
			SetGetRequest();
		}
		case HTTP_HANDSHAKING:
		{
			if (SendToServerSSL() != 0)
			{
				break;
			}
		}
		case RECEIVING_HTTP_RESPONSE:
		{
			if (ReceiveFromServerSSL() != 0)
			{
				break;
			}
		}
		case CONNECTED:
		case HAVE_DATA:
		{
			//Ping();
			ReceiveFromServer();
			SendToServer();
			break;
		}
	}

	return wsclient_status;
}

Buffer *WSClient::GetRecvBuffer()
{
	return receive_buffer;
}

static void my_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
	((void)level);

	const char *file_sep = rindex(file, '/');
	if (file_sep)
		file = file_sep + 1;

	printf("%s:%04d: %s", file, line, str);
}

void WSClient::SetDebug()
{
	mbedtls_debug_set_threshold(4);
	mbedtls_ssl_conf_dbg(&(conn_info->conf), my_debug, stdout);
}

int WSClient::SetupSSL()
{
	int ret;
	ret = mbedtls_ssl_setup(&(conn_info->ssl), &(conn_info->conf));
	if (ret != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}
	return ret;
}

int WSClient::NetConnectSSL()
{
	int ret;
	mbedtls_net_init(&(conn_info->server_fd));

	if ((ret = mbedtls_net_connect(&(conn_info->server_fd), hostname.c_str(),
	                               port.c_str(), MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
		return ret;
	}

	/* set timeout if needed */
	mbedtls_ssl_set_bio(&(conn_info->ssl), &(conn_info->server_fd),
	                    mbedtls_net_send, mbedtls_net_recv, nullptr);

	/* set the socket nonblocking */
	mbedtls_net_set_nonblock(&conn_info->server_fd);
	return ret;
}

void WSClient::ResetWrapper()
{
	delete conn_info;
	conn_info = new(std::nothrow) Wrapper();
}

int WSClient::HandshakeSSL()
{
	int ret;

	ret = mbedtls_ssl_handshake(&(conn_info->ssl));

	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
	    ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS || ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
	{
		wsclient_status = SSL_HANDSHAKING;
	}

	else if (ret != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}

	return ret;
}

int WSClient::ServerVerify()
{
	int ret;

	ret = mbedtls_ssl_get_verify_result(&(conn_info->ssl));

	if (ret != 0)
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}
	return ret;
}

void WSClient::SetGetRequest()
{
	std::string SEC_WEB_KEY = "dGhlIHNhbXBsZSBub25jZQ==";
	std::string SEC_WEB_VERSION = "13";
	std::string GET_REQUEST =
			"GET " + get_path + " HTTP/1.1\r\nHost: " + hostname + ":" + port +
			"\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " +
			SEC_WEB_KEY + "\r\nSec-WebSocket-Version: " + SEC_WEB_VERSION + "\r\n\r\n";

	int len = std::sprintf((char *)send_buffer->text, GET_REQUEST.c_str());

	send_buffer->current_index = 0;
	send_buffer->left_to_process = len;
	wsclient_status = HTTP_HANDSHAKING;
}

int WSClient::SendToServerSSL()
{
	int ret = mbedtls_ssl_write(&(conn_info->ssl), send_buffer->text + send_buffer->current_index,
	                            send_buffer->left_to_process);

	if (ret >= 0)
	{
		send_buffer->current_index += ret;
		send_buffer->left_to_process -= ret;

		if (send_buffer->left_to_process == 0)
		{
			send_buffer->current_index = 0;
			wsclient_status = RECEIVING_HTTP_RESPONSE;
			ret = 0;
		}
	}

	else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
	         ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS || ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
	{
	}

	else
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}

	return ret;
}

int WSClient::ReceiveFromServerSSL()
{
	int ret;
	ret = mbedtls_ssl_read(&conn_info->ssl, receive_buffer->text, receive_buffer->size);

	if (ret > 0)
	{
		receive_buffer->left_to_process = ret;
		wsclient_status = CONNECTED;
		ret = 0;
	}

	else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
	         ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS || ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
	{
		wsclient_status = RECEIVING_HTTP_RESPONSE;
	}

	else
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
		ret = CONNECTION_CLOSED;
	}

	return ret;
}

void PrnBuffer(const unsigned char *buffer, size_t len)
{
	if (len <= 0)
		return;

	for (int i = 0; i < len; ++i)
	{
		putchar(buffer[i]);
	}

	std::cout << std::endl;
}

void CopyDataTo(unsigned char *to, const unsigned char *from, uint64_t len)
{
	for (int i = 0; i < len; ++i)
	{
		to[i] = from[i];
	}
}

void WSClient::AddMSGToSendBuffer(const unsigned char *msg, uint64_t len)
{
	if ((MAX_HEADER_SIZE + len + send_buffer->current_index + send_buffer->left_to_process) >= send_buffer->size)
	{
		perror("You tried to send more trading_data than size of send_buffer\n");
		exit(1);
	}

	/* Set up header, put mask on buffer */
	int ret;
	int mask_index;
	ret = GenHeader(send_buffer->text + send_buffer->current_index + send_buffer->left_to_process, len);
	mask_index = ret - 4;

	CopyDataTo(send_buffer->text + send_buffer->current_index + send_buffer->left_to_process + ret, msg, len);
	PutMaskKeyOnBuffer(send_buffer->text + send_buffer->current_index + send_buffer->left_to_process + ret,
	                   send_buffer->text + send_buffer->current_index + send_buffer->left_to_process + mask_index, len);
	send_buffer->left_to_process += ret + len;
}

int WSClient::SendToServer()
{
	if (send_buffer->left_to_process == 0)
	{
		return BAD_INPUT_DATA;
	}

	int ret;

	ret = mbedtls_ssl_write(&(conn_info->ssl), send_buffer->text + send_buffer->current_index,
	                        send_buffer->left_to_process);
	if (ret > 0)
	{
		send_buffer->current_index += ret;
		send_buffer->left_to_process -= ret;

		/* Everything we wanted to send was delivered to the server */
		if (send_buffer->left_to_process == 0)
		{
			send_buffer->ResetBuffer();
		}
	}

	else
	{
		HandleSendError(ret);
	}

	return ret;
}

int WSClient::SendToServer(const unsigned char * msg, size_t len)
{
	AddMSGToSendBuffer(msg, len);
	return SendToServer();
}

void WSClient::CheckForEndOfFragment()
{
	if (receive_buffer->left_to_process == 0 && receive_buffer->current_index != 0)
	{
		receive_buffer->left_to_process = receive_buffer->current_index;
		receive_buffer->current_index = 0;
		wsclient_status = HAVE_DATA;
		std::cout << "I got from server this: ";
		PrnBuffer(receive_buffer->text, receive_buffer->left_to_process);
	}
}

void WSClient::HandleSendError(int ret)
{
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
	    ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS || ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
	{
	}

	else
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}
}

void WSClient::HandleReceiveError(int ret)
{
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
	    ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS || ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
	{
		wsclient_status = CONNECTED;
	}

	else
	{
		++conn_attempts;
		wsclient_status = CONNECTING;
		ResetBuffers();
		ResetWrapper();
	}
}

int WSClient::ReceiveFromServer()
{
	int ret;

	if (receive_buffer->current_index == 0)
	{
		uint64_t st_index;
		unsigned char temporary_buffer[MAX_HEADER_SIZE];

		if (receive_buffer->left_to_process == -1)
		{
			ret = 6 + mbedtls_ssl_read(&conn_info->ssl, temporary_buffer + 6, MAX_HEADER_SIZE - 6);

			if (ret <= 6)
			{
				return -1;
			}

			CopyDataTo(temporary_buffer, receive_buffer->header, 6);
		}
		else
		{
			wsclient_status = CONNECTED;
			receive_buffer->ResetBuffer();

			ret = mbedtls_ssl_read(&conn_info->ssl, temporary_buffer, 6);
			if (ret > 0)
			{
				if (IsPrefix(temporary_buffer + 2, (const unsigned char *)PONG_MESSAGE, 4))
				{
					return 6;
				}
				else
				{
					receive_buffer->left_to_process = -1;
					CopyDataTo(receive_buffer->header, temporary_buffer, 6);
					return 6;
				}
			}
		}

		if (ret > 0)
		{
			uint64_t len;
			st_index = DecryptHeader(temporary_buffer, &len);
			CopyDataTo(receive_buffer->header + MAX_HEADER_SIZE - st_index, temporary_buffer, ret);
			receive_buffer->left_to_process = len - (ret - st_index);
			receive_buffer->current_index = (ret - st_index);
			wsclient_status = CONNECTED;
			CheckForEndOfFragment();
		}
		else
		{
			HandleReceiveError(ret);
		}
	}

	else
	{
		ret = mbedtls_ssl_read(&conn_info->ssl, receive_buffer->text + receive_buffer->current_index,
		                       receive_buffer->left_to_process);
		if (ret > 0)
		{
			receive_buffer->current_index += ret;
			receive_buffer->left_to_process -= ret;
			CheckForEndOfFragment();
		}
		else
		{
			HandleReceiveError(ret);
		}
	}


	return ret;
}

void WSClient::Ping()
{
	std::chrono::duration<double> interval = std::chrono::steady_clock::now() - wsclient_last_ping_time;
	if (1000.0 * interval.count() >= DEFAULT_PING_TIMER)
	{
		std::cout << " sending ping" << std::endl;
		SendToServer((const unsigned char *)PING_MESSAGE, strlen(PING_MESSAGE));
		wsclient_last_ping_time = std::chrono::steady_clock::now();
	}
}
