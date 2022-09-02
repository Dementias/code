#ifndef SSL_CLIENT_BITMEX_H
#define SSL_CLIENT_BITMEX_H

#include <string>
#include <unordered_map>
#include <vector>

#include "WSClient.h"

extern const char *SUBSCRIBE_SUCCESS;
extern const char *TABLE;
extern const char *DATA;
extern const char *ACTION;
extern const char *PARTIAL_ACTION;
extern const char *UPDATE_ACTION;
extern const char *DELETE_ACTION;
extern const char *INSERT_ACTION;
extern const char *ID;
extern const char *SYMBOL;
extern const char *SIDE;
extern const char *BUY;
extern const char *SELL;
extern const char *PRICE;
extern const char *SIZE;
extern const char *TIMESTAMP;

enum SUBSCRIPTION_STATUS
{
	NOT_SUBSCRIBED, /* 0. WSClient is currently not subscribed to this topic */
	SUBSCRIBE_MESSAGE_SENDING, /* 1. WSClient added subscription message to send_buffer */
	SUBSCRIBED  /* 2. WSClient is subscribed to this topic */
};

struct SubscriptionInfo
{
	SubscriptionInfo();

	SubscriptionInfo(const std::string &, const std::string &);

	std::string topic;
	std::string currency;
	SUBSCRIPTION_STATUS subscription_status;
};

struct DataLineID
{
	std::string symbol;
	uint64_t id;
};

struct DataLineIDHasher
{
	size_t operator()(const DataLineID &key) const;
};

struct DataLineContent
{
	uint64_t size;
	float price;
	std::string timestamp;
};

struct TradingData
{
	void HandleAction(const char *, size_t);

	void PrnTradingData(std::ostream *);

	std::unordered_map<DataLineID, DataLineContent, DataLineIDHasher> beads;
	std::unordered_map<DataLineID, DataLineContent, DataLineIDHasher> offers;
};

class BitmexClient
{
public:
	BitmexClient();

	~BitmexClient();

	void Process();

	void SetSubscriptionsList(const std::vector<SubscriptionInfo> &);

	int Send(const unsigned char *, uint64_t);

private:
	WSClient *client;
	TradingData trading_data;

	std::vector<SubscriptionInfo> subscriptions_list;
	bool all_subscribed;

private:
	int Parse();

	void UpdateSubscriptionStatus(SubscriptionInfo);

	void ResetSubscriptions();
};

DataLineID GetDataLineID(const char *buf, size_t size);

void UpdateDataLineContent(const char *, DataLineContent *, size_t);

SubscriptionInfo GetSubInfoFromMSG(const unsigned char *);

bool operator==(const DataLineID &a, const DataLineID &b);

#endif //SSL_CLIENT_CLIENT_BITMEX_H