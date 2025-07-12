#include "Includes.h"
#include "Types.h"
#include "LevelInfo.h"
#include "Order.h"
#include "Trade.h"
#include "Orderbook.h"


int main() {
    Orderbook orderbook;
    const OrderId orderId = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    std::cout << orderbook.Size() << std::endl;
    orderbook.CancelOrder(orderId);
    std::cout << orderbook.Size() << std::endl;

    return 0;
}
