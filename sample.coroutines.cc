#include <iostream>
#include <sstream>
#include <thread>

#include "duty.hpp"

void print( const char *text ) {
    std::stringstream ss;
    ss << "Thread " << std::this_thread::get_id() << ": " << text << std::endl;
    std::cout << ss.str();
}

void waits() {
    print("child is waiting...");
    duty::wait("exiting-signal");
    print("child is returning...");
}

void signals() {
    print("master is notifying...");
    duty::signal("exiting-signal");
}

int main( int argc, const char **argv )
{
    std::thread t1(waits), t2(waits), t3(waits), t4(signals);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return 0;
}
