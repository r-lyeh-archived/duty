#include <cassert>
#include <iostream>
#include "duty.hpp"

bool is_true_func() {
    duty::sleep(1.0);
    return true;
}

class is_true_class {
    public:
    bool operator()() {
        duty::sleep(1.0);
        return true;
    }
};

bool is_bigger_than_1000( int n ) {
    return n > 1000;
}

int main() {

    using namespace duty;

    // task is a collection of boolean functions that runs while they return true
    // user can chain tasks on tasks
    // result can be retrieved with sync() or async()

    // queue
    tasks t;
    t
        << is_true_func
        << is_true_class()
        << [](){ std::cout << 3 << std::endl; return false; }
        << [](){ std::cout << 4 << std::endl; return true; }
        << std::bind(is_bigger_than_1000, 1337);
    assert( !t() );

    t[2] = nullptr;
    assert( t() );

    assert( t.sync() );

    t.async();
    t.wait();
    assert( t.get() );
    std::cout << t.status << std::endl;

    tasks t2 = t;
    assert( t2.size() == 5 );

    t = t2[0];
    assert( t.size() == 1 );
    assert( t() );

    t2.clear();
    assert( t2.size() == 0 );
    assert( t2() );

    return 0;
}
