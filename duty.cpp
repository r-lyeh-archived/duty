#include <stdio.h>

#include <ctime>

#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

#include "duty.hpp"

#if defined( _WIN32 )
#   include <Windows.h>
#else
#   include <unistd.h>
#   include <sys/time.h>
#endif

namespace
{
    extern volatile bool closing;

    struct coroutine
    {
        bool on;
        bool owner;
        std::condition_variable *cv;
        std::mutex *cvm;

        coroutine() : on(false), owner(true)
        {
            cv = new std::condition_variable;
            cvm = new std::mutex;
        }

        ~coroutine()
        {
            if( !owner ) return;
            if( cv ) delete cv, cv = 0;
            if( cvm ) delete cvm, cvm = 0;
        }

        coroutine( const coroutine &s )
        {
            cv = s.cv;
            cvm = s.cvm;
            owner = true;

            //s.owner = false;
            ( const_cast< coroutine & >(s) ).owner = false;
        }

        void wait()
        {
            on = false;
            std::unique_lock<std::mutex> lk(*cvm);
            cv->wait(lk, [=](){ return closing || !cvm || !cv || on == true; } );
        }

        void single_signal()
        {
            on = true;
            cv->notify_one();
        }

        void signal()
        {
            on = true;
            cv->notify_all();
        }
    };

    std::map< std::string, coroutine > coroutines;

    void notify_exit()
    {
        closing = true;

        for( auto &it : coroutines )
        {
            /*
            std::stringstream ss;
            if( ss << "<duty/duty.cpp> says: aborting coroutine '" << it.first << "'" << std::endl )
                fprintf( stdout, "%s", ss.str().c_str() );
            */
            it.second.signal();
        }
    }

    bool volatile closing = ( atexit(notify_exit), false );
}

namespace duty
{
    void wait( const int &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].wait();
    }
    void wait( const size_t &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].wait();
    }
    void wait( const std::string &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].wait();
    }

    void single_signal( const int &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].single_signal();
    }
    void single_signal( const size_t &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].single_signal();
    }
    void single_signal( const std::string &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].single_signal();
    }

    void signal( const int &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].signal();
    }
    void signal( const size_t &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].signal();
    }
    void signal( const std::string &t )
    {
        std::stringstream ss;
        if( ss << t )
            coroutines[ ss.str() ].signal();
    }

    void signals()
    {
        closing = true;

        for( auto &it : coroutines )
            it.second.signal();

        closing = false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace duty
{
#ifdef _WIN32
#   define $handle                      LARGE_INTEGER
#   define $freq( handle )              { $handle fhandle; DWORD_PTR oldmask = ::SetThreadAffinityMask(::GetCurrentThread(), 0); ::QueryPerformanceFrequency( &fhandle ); ::SetThreadAffinityMask(::GetCurrentThread(), oldmask); frequency = 1000000.0 / double(fhandle.QuadPart); }
#   define $update( handle )            {                  DWORD_PTR oldmask = ::SetThreadAffinityMask(::GetCurrentThread(), 0); ::QueryPerformanceCounter  ( &handle ); ::SetThreadAffinityMask(::GetCurrentThread(), oldmask); }
#   define $setcounter( handle, value ) handle.QuadPart = value
#   define $diffcounter( handle1, handle2 ) ( ( handle2.QuadPart - handle1.QuadPart ) * frequency )
#   define $sleep( seconds_f )          Sleep( (int)(seconds_f * 1000) )
#   define $wink( units_t )             Sleep( units_t )
#else
//  hmmm... check clock_getres() as seen in http://tdistler.com/2010/06/27/high-performance-timing-on-linux-windows#more-350
//  frequency int clock_getres(clockid_t clock_id, struct timespec *res);
//  clock     int clock_gettime(clockid_t clock_id, struct timespec *tp);
//  nanosleep() instead?
#   define $handle                      timeval
#   define $freq( handle )
#   define $update( handle )            gettimeofday( &handle, NULL )
#   define $setcounter( handle, value ) do { handle.tv_sec = 0; handle.tv_usec = value; } while (0)
#   define $diffcounter( handle1, handle2 ) ( (handle2.tv_sec * 1000000.0) + handle2.tv_usec ) - ( (handle1.tv_sec * 1000000.0) + handle1.tv_usec )
#   define $sleep( seconds_f )          usleep( seconds_f * 1000000.f )
#   define $wink( units_t )             usleep( units_t )
//  do { float fractpart, intpart; fractpart = std::modf( seconds_f, &intpart); \
//    ::sleep( int(intpart) ); usleep( int(fractpart * 1000000) ); } while( 0 )
#endif

    namespace
    {
        class timer
        {
            public:

            timer()
            {
                reset();
            }

            double s() //const
            {
                return us() / 1000000.0;
            }

            double ms() //const
            {
                return us() / 1000.0;
            }

            double us() //const
            {
                $update( endCount );

                return $diffcounter( startCount, endCount );
            }

            double ns() //const
            {
                return us() * 1000.0;
            }

            void reset()
            {
                $freq( frequency ); //to dt2() ctor ?

                $setcounter( startCount, 0 );
                $setcounter( endCount, 0 );

                $update( startCount );
            }

            protected:

            $handle startCount, endCount;
            double frequency;
        } dt;

        double offset = 0;
    }

    double epoch()
    {
        return double( std::time(NULL) );
    }

    double now()
    {
        return offset + dt.s();
    }

    void sleep( double seconds ) {
        $sleep( seconds );
    }

    void wink( unsigned times ) {
        $wink( times );
    }

#   undef $handle
#   undef $freq
#   undef $update
#   undef $diffcounter
#   undef $setcounter
#   undef $sleep
#   undef $wink
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace duty {

status::status() {
    clear();
}

void status::clear() {
    taken = 0;
    begin = end = 0;
    eta = 0;
    has_started = has_finished = has_suceeded = has_crashed = 0;
    progress = 0;
    exitcode = 0;
    memory = 0;
    cpu = cpus = 0;
    callstack[0] = 0;
}

std::string status::str() const {
    std::stringstream ss;
    ss << "taken" ": " << taken << std::endl;
    ss << "begin" ": " << begin << std::endl;
    ss << "end" ": " << end << std::endl;
    ss << "eta" ": " << eta << std::endl;
    ss << "has_started" ": " << has_started << std::endl;
    ss << "has_finished" ": " << has_finished << std::endl;
    ss << "has_suceeded" ": " << has_suceeded << std::endl;
    ss << "has_crashed" ": " << has_crashed << std::endl;
    ss << "progress" ": " << progress << std::endl;
    ss << "exitcode" ": " << exitcode << std::endl;
    ss << "memory" ": " << memory << std::endl;
    ss << "cpu" ": " << cpu << std::endl;
    ss << "cpus" ": " << cpus << std::endl;
    ss << "callstack" ": " << callstack[0] << std::endl;
    return ss.str();
}

std::string status::graph() const {
    std::stringstream ss;

    // progress: width 100 stretched to 20 (100/5)
    ss << '[' << std::string(int(progress / 5), '=') << std::string(20 - int(progress / 5), '.') << ']';
    if( !has_started ) {
        ss << "(idle)";
    } else {
        if( has_finished ) {
            if( has_suceeded ) {
                ss << "(code:" << exitcode << ")";
                // cpu, mem
            } else {
                // has_crashed
                // callstack
            }
        } else {
            ss << "(eta:" << eta << ")";
        }
    }
    ss << std::endl;

    return ss.str();
}

bool tasks::sync() {

    double start = duty::now();

    status.clear();
    status.begin = (unsigned)std::time(0);
    status.has_started = true;

    if( verbose )
        std::cout << status.str() << std::endl;

    try {
        float progress_inc = this->empty() ? 0.f : 100.f / this->size();

        for( tasks::const_iterator
                in = this->begin(), end = this->end(); in != end; ++in ) {
            auto &it = *in;
            bool is_ok = ( it == nullptr || it() );

            status.progress += progress_inc;

            // calc eta & update taken {
            double now = duty::now();
            status.taken = now - start;
            status.eta = ( int( 100 - status.progress ) * status.taken ) / status.progress;
            // }

            if( is_ok ) {
                status.exitcode = 200;
                if( verbose )
                    std::cout << status.graph() << std::endl;
                continue;
            } else {
                status.exitcode = 404;
                if( verbose )
                    std::cout << status.graph() << std::endl;
                break;
            }
        }
    } catch(...) {
        status.has_crashed = true;
        //@todo callstack
    }

    status.end = (unsigned)std::time(0);
    status.taken = duty::now() - start;
    status.has_suceeded = status.exitcode < 300 && (!status.has_crashed);
    status.has_finished = true;

    if( verbose )
        std::cout << status.str() << std::endl;

    return status.has_suceeded;
}

bool tasks::operator()() {
    return sync();
}

namespace {
    std::future<bool> &future( tasks *self ) {
        static std::map< tasks *, std::future<bool> > map;
        return map[self];
    }
}

void tasks::async() {
    future(this) = std::async(std::launch::async,*this);
}

void tasks::wait() {
    future(this).wait();
}

bool tasks::get() {
    return future(this).get();
}

} // duty::

std::ostream &operator <<( std::ostream &os, const duty::status &t ) {
    return os << t.str(), os;
}

///////////////////////////////////////
