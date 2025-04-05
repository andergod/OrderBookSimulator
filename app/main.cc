#include <iostream>
#include "config.hpp"
#include "my_lib.h"
#include <map>

int main()
{
    std::cout << project_name << '\n';
    std::cout << project_version << '\n';

    std::map<double, order> order_book = initiate_order_book();
    
    orderGenerator generator;

    for (int i = 0; i < 5; ++i) {
        order o = generator.generateOrder();
        std::cout << "Order ID: " << o.order_id 
                  << ", Price: " << o.price
                  << ", Quantity: " << o.quantity
                  << ", Side: " << o.side << std::endl;
    }

    return 0;
}
