// Duty is a lightweight C++11 task manager for parallel coroutines and serial jobs. ZLIB/LibPNG licensed
// - rlyeh

// duty graph map
// +---> serial jobs (tasks)
// |
// v
// parallel coroutines (threads)
//
// graph sample:
// thread #1: tasks( a -> b -> c ) -> wait("ready") -> tasks( d ) -> idle()
// thread #2: tasks( g -> h -> i -> e -> f ) -> signal("ready") -> idle()
// thread #3: idle()
// thread #4: idle()
// [...]

// @todo
// bool tasks::suspend( bool on = true );      // suspend to ram
// bool tasks::resume();                       // resume from ram
// bool tasks::hibernate( bool on = true );    // suspend to disk
// bool tasks::wakeup();                       // resume from disk

#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

/* public coroutines API */
namespace duty
{
    // put coroutine to wink (shortest sleep time)
    void wink( unsigned times );
    // put coroutine to sleep
    void sleep( double seconds );

    // put coroutine to sleep until requested signal is triggered
    void wait( const int &id );
    void wait( const size_t &id );
    void wait( const std::string &id );

    // send global trigger to all waiting coroutines
    void signal( const int &id );
    void signal( const size_t &id );
    void signal( const std::string &id );

    // send *single* signal to *any* waiting coroutine
    void single_signal( const int &id );
    void single_signal( const size_t &id );
    void single_signal( const std::string &id );

    // send signals to all waiting coroutines
    void signals();
}

/* public time API */
namespace duty
{
    // get seconds since start of program
    double now();
    // get seconds since unix epoch
    double epoch();
}

/* public tasks API */
namespace duty
{
    typedef std::function<bool (void)> task;

    struct status
    {
        double   taken;                 // time taken
        unsigned begin, end;            // related timestamps (seconds)
        double   eta;                   // related timestamps (seconds)
        bool     has_started;           // started?
        bool     has_finished;          // finished? ( started < finished )
        bool     has_suceeded;          // suceeded? ( started < finished < suceeded )
        bool     has_crashed;           // crashed?
        float    progress;              // [0..100]
        unsigned exitcode;              // http codes
        unsigned memory;                // used memory (kbytes)
        unsigned cpu, cpus;             // used cpu (percent), cpus (number)
        void    *callstack[32];         // saved callstack (if crashed)

        status();

        void clear();

        std::string str() const;

        std::string graph() const;
    };

    class tasks : public std::vector< task >
    {
        enum settings { verbose = true };

        public:

        duty::status status;

        tasks()
        {}

        bool operator()();

        template<typename T>
        inline tasks &operator=( const T &t ) {
            this->clear();
            this->push_back( t );
            return *this;
        }

        template<typename T>
        inline tasks &operator+=( const T &t ) {
            this->push_back( t );
            return *this;
        }

        template<typename T>
        inline tasks &operator<<( const T &t ) {
            this->push_back( t );
            return *this;
        }

        void async();
        void wait();
        bool get();

        bool sync();
    };
}

std::ostream &operator <<( std::ostream &os, const duty::status &t );

