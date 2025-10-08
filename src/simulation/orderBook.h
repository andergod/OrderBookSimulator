#pragma once

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <array>
#include <deque>
#include <unordered_map>
#include "config.hpp"
#include <memory_resource>
#include <optional>
#include <memory>

enum class Side : bool
{
    Buy = false,
    Sell = true
};

// represent the order we receive raw
struct orderReceived
{
    double price;
    std::int32_t quantity;
    Side side; // 0 means buy, and 1 sell
    std::chrono::system_clock::time_point timestamp;
    std::int32_t order_id;
    orderReceived(double p, std::int32_t q, Side s, std::chrono::system_clock::time_point t, std::int32_t oId)
        : price(p), quantity(q), side(s), timestamp(t), order_id(oId) {}
};

// Making a mimic as what info do we actually stores for our L2 book
struct order
{
    std::int32_t order_id;
    std::int32_t quantity;
    std::chrono::system_clock::time_point timestamp;
    order(const std::int32_t id, const std::int32_t q, const std::chrono::system_clock::time_point t)
        : order_id(id), quantity(q), timestamp(t) {}
};
// Not using yet
struct trade
{
    std::string trade_id;
    double price;
    int quantity;
    std::chrono::system_clock::time_point timestamp;
    // is this always gonna be const? i am not making any change while defining
    trade(const std::string id, const double p, const int q, const std::chrono::system_clock::time_point t)
        : trade_id(id), price(p), quantity(q), timestamp(t) {}
};

struct OrderLocation
{
    Side side;
    std::int32_t price_index;
    std::shared_ptr<order> order_pt; // smart pointer
    OrderLocation() = default;
    OrderLocation(const Side s, std::int32_t pxIdx, std::shared_ptr<order> it)
        : side(s), price_index(pxIdx), order_pt(it) {}
};

struct OrderIntrusive
{
    std::int32_t order_id;
    std::int32_t quantity;
    std::chrono::system_clock::time_point timestamp;
    OrderIntrusive *next;
    OrderIntrusive *prev;
    OrderIntrusive() = default;
    OrderIntrusive(const std::int32_t id, const std::int32_t q, const std::chrono::system_clock::time_point t)
        : order_id(id), quantity(q), timestamp(t), next(nullptr), prev(nullptr) {}
};

struct OrderLocationIntrusive
{
    Side side;
    std::int32_t price_index;
    OrderIntrusive *order_pt; // smart pointer
    OrderLocationIntrusive() = default;
    OrderLocationIntrusive(const Side s, std::int32_t pxIdx, OrderIntrusive *it)
        : side(s), price_index(pxIdx), order_pt(it) {}
};

struct tradeRecord
{
    // tradeRecord has two orders (buy and sell, and the actual transactions (trade struct))
    order order1;
    order order2;
    trade tradeDone;
};

enum class action : bool
{
    modify = true,
    cancel = false
};

struct amendOrder
{
    std::int32_t order_id;
    action act;
    std::optional<double> price;
    amendOrder(std::int32_t id, action a, std::optional<double> p = std::nullopt)
        : order_id(id), act(a), price(p) {}
};

struct OrderList
{
    OrderIntrusive *head;
    OrderIntrusive *tail;

    OrderList() : head(nullptr), tail(nullptr) {}

    void push_back(OrderIntrusive *order)
    {
        order->next = nullptr;
        order->prev = tail;
        if (tail)
        {
            tail->next = order;
        }
        else
        {
            head = order;
        }
        tail = order;
    }

    void erase(OrderIntrusive *order)
    {
        if (order->prev)
        {
            order->prev->next = order->next;
        }
        else
        {
            head = order->next;
        }
        if (order->next)
        {
            order->next->prev = order->prev;
        }
        else
        {
            tail = order->prev;
        }
        order->next = nullptr;
        order->prev = nullptr;
    }

    bool empty() const { return head == nullptr; }
};

class OrderPool
{
private:
    std::vector<OrderIntrusive> pool;
    std::vector<OrderIntrusive *> freeList;

public:
    OrderPool(std::int32_t size);
    OrderIntrusive *allocate(std::int32_t id, std::int32_t quantity, std::chrono::system_clock::time_point timestamp);
    void deallocate(OrderIntrusive *order);
};

class pmrPool
{
    std::pmr::unsynchronized_pool_resource &pool;
    std::pmr::vector<OrderIntrusive *> free_list;
    size_t capacity;

public:
    pmrPool(size_t cap, std::pmr::unsynchronized_pool_resource &p);
    OrderIntrusive *allocate(std::int32_t id, std::int32_t quantity, std::chrono::system_clock::time_point timestamp);
    void deallocate(OrderIntrusive *order);
};

class orderGenerator
{
private:
    // keep count of the Ids for each order generated
    std::int32_t idGenerated;
    std::vector<std::int32_t> activeIds;
    // Use for random selection of Ids for cancel and ammends
    std::unordered_map<std::int32_t, std::size_t> idToIdx;

public:
    // class definition
    orderGenerator();
    amendOrder cancelOrders();
    amendOrder modifyOrders();
    void ackCancel(std::int32_t orderId);
    void showActiveId();
    // method to generate random orders
    orderReceived generateOrder();
};