/***************************************************************************
 *   Copyright © 2026 by Andrey Afletdinov <public.irkutsk@gmail.com>      *
 *                                                                         *
 *   https://github.com/AndreyBarmaley/sdbus-cpp-asio                      *
 *                                                                         *
 *   MIT License                                                           *
 *                                                                         *
 ***************************************************************************/

#include <unistd.h>

#include <chrono>
#include <iostream>
#include <functional>

#include "test_dbus.h"
#include "test_syslog.h"
#include "test_proxy.h"

using namespace std::chrono_literals;
using namespace boost;

namespace Test {
    /// SessionProxy
    SessionProxy::SessionProxy()
#ifdef SDBUS_2_0_API
        : ProxyInterfaces(sdbus::createSessionBusConnection(), sdbus::ServiceName {dbus_service_name}, sdbus::ObjectPath {dbus_service_path}),
#else
        :
        ProxyInterfaces(sdbus::createSessionBusConnection(), dbus_service_name, dbus_service_path),
#endif
          signals_ {ioc_}, timer_sdbus_ {ioc_, 1ms} {
        registerProxy();
    }

    SessionProxy::~SessionProxy() {
        unregisterProxy();
        stop();
    }

    void SessionProxy::timerCalled(const boost::system::error_code& ec) {
        if(! ec) {
            bool expected = true;

            if(success_.compare_exchange_strong(expected, false)) {
                value_++;
                TestJob(getpid(), value_);
            }

            timer_sdbus_.expires_after(1ms);
            timer_sdbus_.async_wait(std::bind(&SessionProxy::timerCalled, this, std::placeholders::_1));
        }
    }

    void SessionProxy::onTestSignal(const int32_t & id, const int32_t & val) {
        if(getpid() == id) {
            Syslog::info("recv TestSignal: %d", val);

            if(val == value_) {
                success_.store(true);
            } else {
                Syslog::error("recv invalid values: %d, %d", val, static_cast<int>(value_));
                asio::post(ioc_, std::bind(&SessionProxy::stop, this));
            }
        }
    }

    int SessionProxy::start(void) {
        Syslog::info("started, pid: %d", getpid());

        signals_.add(SIGINT);
        signals_.add(SIGTERM);

        signals_.async_wait([this](const boost::system::error_code & ec, int signal) {
            // skip canceled
            if(ec != boost::asio::error::operation_aborted && (signal == SIGTERM || signal == SIGINT)) {
                this->stop();
            }
        });

        timer_sdbus_.async_wait(std::bind(&SessionProxy::timerCalled, this, std::placeholders::_1));

        ioc_.run();
        return EXIT_SUCCESS;
    }

    void SessionProxy::stop(void) {
        signals_.cancel();
        timer_sdbus_.cancel();
    }
}

int main(int argc, char** argv) {
    if(0 == getuid()) {
        std::cerr << "for users only" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        Test::SessionProxy().start();
    } catch(const sdbus::Error & err) {
        Syslog::error("sdbus: [%s] %s", err.getName().c_str(), err.getMessage().c_str());
    } catch(const std::exception & err) {
        Syslog::error("%s: exception: %s", __FUNCTION__, err.what());
    }

    return EXIT_FAILURE;
}
