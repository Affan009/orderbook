#include "orderbook/core/price.hpp"

int spread(int bid, int ask) {
    return ask - bid;
}
