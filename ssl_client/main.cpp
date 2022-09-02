#include <string>
#include <vector>
#include "BitmexClient.h"

int main()
{
	size_t N = 1;
	std::vector<BitmexClient> cl(N);

	/* Set subscriptions_list for every client */
	std::vector<SubscriptionInfo> subscriptions;
	SubscriptionInfo this_subscription("orderBookL2_25", "XBTUSD");
	subscriptions.push_back(this_subscription);

	for (int i = 0; i < cl.size(); ++i)
	{
		cl[i].SetSubscriptionsList(subscriptions);
	}

	/* Main loop */
	while (1)
	{
		for (int i = 0; i < N; ++i)
		{
			cl[i].Process();
		}
	}
}

// bead - покупка
// offer - продажа. Сделать их отдельно