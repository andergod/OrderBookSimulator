#include "orderBook.h"
#include <memory_resource>
#include <vector>
#include <iostream>

pmrPool::pmrPool(size_t cap, std::pmr::unsynchronized_pool_resource& p)
    : pool(p), free_list(std::pmr::polymorphic_allocator<OrderIntrusive*>(&p)), capacity(cap) {
    for (size_t i = 0; i < capacity; ++i) { 
        void* mem = pool.allocate(sizeof(OrderIntrusive), alignof(OrderIntrusive));
        free_list.push_back(static_cast<OrderIntrusive*>(mem));
    }
}

OrderIntrusive* pmrPool::allocate(std::int32_t id, std::int32_t quantity, std::chrono::system_clock::time_point timestamp) {
    if (free_list.empty()) return nullptr;
    // no space left 
    OrderIntrusive* slot = free_list.back(); 
    free_list.pop_back(); 
    return new (slot) OrderIntrusive(id, quantity, timestamp);
}


void pmrPool::deallocate(OrderIntrusive* o) {
    if (!o) return;
    o->~OrderIntrusive(); // call destructor 
    free_list.push_back(o);
}