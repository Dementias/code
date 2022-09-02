#include <string>
#include <cstdlib>
#include "BitmexClient.h"
#include "WSUtils.h"
#include <fstream>

const char *SUBSCRIBE_SUCCESS = "{\"success\":true,\"subscribe\":";
const char *TABLE = "table";
const char *DATA = "data";
const char *ACTION = "action";
const char *PARTIAL_ACTION = "partial";
const char *UPDATE_ACTION = "update";
const char *DELETE_ACTION = "delete";
const char *INSERT_ACTION = "insert";
const char *ID = "id";
const char *SYMBOL = "symbol";
const char *SIDE = "side";
const char *BUY = "Buy";
const char *SELL = "Sell";
const char *PRICE = "price";
const char *SIZE = "size";
const char *TIMESTAMP = "timestamp";

SubscriptionInfo::SubscriptionInfo() : subscription_status(NOT_SUBSCRIBED)
{

}

SubscriptionInfo::SubscriptionInfo(const std::string &topic, const std::string &currency) :
		topic(topic), currency(currency), subscription_status(NOT_SUBSCRIBED)
{

}

bool operator==(SubscriptionInfo &a, SubscriptionInfo &b)
{
	return (a.topic == b.topic) && (a.currency == b.currency);
}

BitmexClient::BitmexClient() : all_subscribed(false)
{
	client = new(std::nothrow)WSClient;
	if (client == nullptr)
	{
		perror("Cannot allocate memory for BitmexClient\n");
		exit(1);
	}
}

BitmexClient::~BitmexClient()
{
	delete client;
}

std::string GenSubscribeMessage(const std::string &topic, const std::string &currency)
{
	std::string s;
	s = "{\"op\":\"subscribe\",\"args\":[\"" + topic + ":" + currency + "\"]}";
	return s;
}

void BitmexClient::ResetSubscriptions()
{
	for (int i = 0; i < subscriptions_list.size(); ++i)
	{
		subscriptions_list[i].subscription_status = NOT_SUBSCRIBED;
	}
	all_subscribed = false;
}

void BitmexClient::SetSubscriptionsList(const std::vector<SubscriptionInfo> &subscriptions)
{
	subscriptions_list.clear();
	for (int i = 0; i < subscriptions.size(); ++i)
	{
		subscriptions_list.push_back(subscriptions[i]);
	}
	all_subscribed = false;
}

int BitmexClient::Send(const unsigned char *msg, uint64_t len)
{
	return client->SendToServer(msg, len);
}

void BitmexClient::Process()
{
	int ret;

	client->Process();

	switch (client->GetStatus())
	{
		case CONNECTING:
		{
			ResetSubscriptions();
		}
		case CONNECTED:
		{
			/* Check if there are any troubles with subscribing */
			if (!all_subscribed)
			{
				all_subscribed = true;
				for (int i = 0; i < subscriptions_list.size(); ++i)
				{
					switch (subscriptions_list[i].subscription_status)
					{
						case NOT_SUBSCRIBED:
						{
							std::string s = GenSubscribeMessage(subscriptions_list[i].topic,
							                                    subscriptions_list[i].currency);
							client->AddMSGToSendBuffer((const unsigned char *)s.c_str(), s.length());
							subscriptions_list[i].subscription_status = SUBSCRIBE_MESSAGE_SENDING;
							all_subscribed = false;
							break;
						}
						case SUBSCRIBE_MESSAGE_SENDING:
						{
							all_subscribed = false;
							break;
						}
						case SUBSCRIBED:
						{
							break;
						}
					}
				}
			}
			break;
		}
		case HAVE_DATA:
		{
			/* Parse available trading_data from receive_buffer */
			Parse();
			break;
		}
	}
}

SubscriptionInfo GetSubInfoFromMSG(const unsigned char *buf)
{
	SubscriptionInfo sub_info;
	/* ignore first symbol = " */
	int i = 1;
	while (buf[i] != ':')
	{
		sub_info.topic += buf[i];
		++i;
	}
	++i;
	while (buf[i] != '\"')
	{
		sub_info.currency += buf[i];
		++i;
	}
	return sub_info;
}

void BitmexClient::UpdateSubscriptionStatus(SubscriptionInfo sub_info)
{
	for (int i = 0; i < subscriptions_list.size(); ++i)
	{
		if (subscriptions_list[i] == sub_info)
		{
			subscriptions_list[i].subscription_status = SUBSCRIBED;
			return;
		}
	}
}

int BitmexClient::Parse()
{
	/* Parse receive_buffer.
	 * Also don't forget to update SubscriptionStatus,
	 * otherwise you will never know whether subscription was successful or not */

	if (client->GetStatus() != HAVE_DATA)
	{
		return -1;
	}

	Buffer *recv_buffer = client->GetRecvBuffer();

	const char *ptr = strstr((const char *)recv_buffer->text, TABLE);

	if (ptr)
	{
		trading_data.HandleAction((const char *)recv_buffer->text, recv_buffer->left_to_process);
		trading_data.PrnTradingData(&std::cout);
	}

	else if (IsPrefix(recv_buffer->text, (const unsigned char *)SUBSCRIBE_SUCCESS,
	                  strlen(SUBSCRIBE_SUCCESS)))
	{
		SubscriptionInfo sub_info = GetSubInfoFromMSG(recv_buffer->text + strlen(SUBSCRIBE_SUCCESS));
		UpdateSubscriptionStatus(sub_info);
		std::cout << "I subbed to: " << sub_info.topic << ":" << sub_info.currency << std::endl;
	}

	else
	{
		/* ignore this trading_data */
	}

	client->GetRecvBuffer()->ResetBuffer();
	return 0;
}

bool operator==(const DataLineID &a, const DataLineID &b)
{
	return (a.symbol == b.symbol) && (a.id == b.id);
}

size_t DataLineIDHasher::operator()(const DataLineID &key) const
{
	return std::hash<std::string>()(key.symbol) ^ std::hash<uint64_t>()(key.id);
}

std::string GetKeyValueFromBuffer(const char *buf, const char *key, size_t size)
{
	/* Gets value in front of first occurrence key in buffer "buf" size of "size" */
	const char *ptr = strstr(buf, key) + strlen(key) + 2;
	if (ptr == nullptr || ptr - buf >= size)
	{
		return "";
	}

	std::string s;
	int i = 0;
	if (ptr[i] == '\"')
	{
		++i;
	}
	while (ptr[i] != '\"' && ptr[i] != ',')
	{
		s += ptr[i];
		++i;
	}

	return s;
}

DataLineID GetDataLineID(const char *buf, size_t size)
{
	DataLineID dataline_id;
	dataline_id.symbol = GetKeyValueFromBuffer(buf, SYMBOL, size);
	dataline_id.id = std::stoull(GetKeyValueFromBuffer(buf, ID, size).c_str());
	return dataline_id;
}

void TradingData::PrnTradingData(std::ostream *output)
{
	for (auto [key, value]: beads)
	{
		*output << key.symbol << " " << key.id << " " << BUY << " "
		        << value.size << " " << value.price << " " << value.timestamp << std::endl;
	}

	for (auto [key, value]: offers)
	{
		*output << key.symbol << " " << key.id << " " << SELL << " "
		        << value.size << " " << value.price << " " << value.timestamp << std::endl;
	}
}

void UpdateDataLineContent(const char *buf, DataLineContent *content, size_t dataline_border)
{
	std::string s;
	s = GetKeyValueFromBuffer(buf, SIZE, dataline_border);
	if (s.length() != 0)
	{
		content->size = std::stoull(s);
	}

	s = GetKeyValueFromBuffer(buf, PRICE, dataline_border);
	if (s.length() != 0)
	{
		content->price = std::stof(s);
	}

	s = GetKeyValueFromBuffer(buf, TIMESTAMP, dataline_border);
	if (s.length() != 0)
	{
		content->timestamp = s;
	}
}

void TradingData::HandleAction(const char *buf, size_t size)
{
	std::string action = GetKeyValueFromBuffer(buf, ACTION, size);
	buf = strstr(buf, DATA);

	while (1)
	{
		buf = strchr(buf, '{');
		if (buf == nullptr)
		{
			break;
		}

		++buf;
		size_t dataline_border = strchr(buf, '}') - buf;
		DataLineID dataline_id = GetDataLineID(buf, dataline_border);
		std::string side = GetKeyValueFromBuffer(buf, SIDE, dataline_border);

		if (action == DELETE_ACTION)
		{
			/* Delete bead/offer from trading_data */
			if (side == BUY)
			{
				beads.erase(dataline_id);
			}
			if (side == SELL)
			{
				offers.erase(dataline_id);
			}
		}

		else if (action == UPDATE_ACTION)
		{
			/* Update already existing bead/offer */
			DataLineContent *content;

			if (side == BUY)
			{
				content = &beads[dataline_id];

			}
			if (side == SELL)
			{
				content = &offers[dataline_id];
			}

			UpdateDataLineContent(buf, content, dataline_border);
		}

		else if (action == INSERT_ACTION || action == PARTIAL_ACTION)
		{
			/* Add new bead/offer */
			DataLineContent content;
			UpdateDataLineContent(buf, &content, dataline_border);

			if (side == BUY)
			{
				beads[dataline_id] = content;
			}

			if (side == SELL)
			{
				offers[dataline_id] = content;
			}
		}

		else
		{
			perror("Unknown action\n");
			exit(1);
		}
	}
}