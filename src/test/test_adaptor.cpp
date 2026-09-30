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
#include "test_adaptor.h"

using namespace std::chrono_literals;
using namespace boost;

namespace Test {
    /// SessionAdaptor
    SessionAdaptor::SessionAdaptor(DBusConncetionPtr conn)
#ifdef SDBUS_2_0_API
        : AdaptorInterfaces(*conn, sdbus::ObjectPath {dbus_service_path}),
#else
        :
        AdaptorInterfaces(*conn, dbus_service_path),
#endif
          SDBus::AsioCoroConnector(std::move(conn)), signals_ {ioc_} {
        registerAdaptor();
    }

    SessionAdaptor::~SessionAdaptor() {
        unregisterAdaptor();
    }

    asio::awaitable<void> SessionAdaptor::signalsHandler(void) {
        signals_.add(SIGTERM);
        signals_.add(SIGINT);

        try {
            for(;;) {
                int signal = co_await signals_.async_wait(asio::use_awaitable);

                if(signal == SIGTERM || signal == SIGINT) {
                    asio::post(ioc_, std::bind(&SessionAdaptor::stop, this));
                    co_return;
                }
            }
        } catch(const system::system_error& err) {
            auto ec = err.code();

            if(ec != asio::error::operation_aborted) {
                Syslog::error("%s: system error: `%s', code: %d",
                              __FUNCTION__, ec.message().c_str(), ec.value());
            }
        }
    }

    asio::awaitable<void> SessionAdaptor::sdbusHandler(void) {
        try {
            co_await sdbusEventLoop();
        } catch(const system::system_error& err) {
            auto ec = err.code();
            Syslog::error("%s: system error: `%s', code: %d",
                          __FUNCTION__, ec.message().c_str(), ec.value());
        } catch(const sdbus::Error& err) {
            Syslog::error("%s: sdbus error: %s", __FUNCTION__, err.what());
            asio::post(ioc_, std::bind(&SessionAdaptor::stop, this));
        }

        co_return;
    }

    int SessionAdaptor::start(void) {
        Syslog::info("started, pid: %d", getpid());

        asio::thread_pool pool{static_cast<uint32_t>(threads_)};

        asio::co_spawn(ioc_, signalsHandler(), asio::detached);
        asio::co_spawn(ioc_, sdbusHandler(), asio::detached);

        for(auto it = 0; it < threads_; ++it) {
            asio::post(pool, [this]() {
                ioc_.run();
            });
        }

        pool.join();

        return EXIT_SUCCESS;
    }

    void SessionAdaptor::stop(void) {
        sdbusLoopCancel();
        signals_.cancel();

        Syslog::info("%s", __FUNCTION__);
    }

    int32_t SessionAdaptor::GetVersion(void) {
        Syslog::debug("%s", __FUNCTION__);
        return 20260901;
    }

    void SessionAdaptor::ServiceShutdown(void) {
        Syslog::debug("%s", __FUNCTION__);
        asio::post(ioc_, std::bind(&SessionAdaptor::stop, this));
    }

    void SessionAdaptor::TestJob(const int32_t & id, const int32_t & val) {
        asio::post(ioc_, [this, id, val]() {
            emitTestSignal(id, val);
        });
    }
}

int main(int argc, char** argv) {
    if(0 == getuid()) {
        std::cerr << "for users only" << std::endl;
        return EXIT_FAILURE;
    }

    try {
#ifdef SDBUS_2_0_API
        auto conn = sdbus::createSessionBusConnection(sdbus::BusName {Test::dbus_service_name});
#else
        auto conn = sdbus::createSessionBusConnection(Test::dbus_service_name);
#endif
        return Test::SessionAdaptor(std::move(conn)).start();
    } catch(const std::exception & err) {
        Syslog::error("%s: exception: %s", __FUNCTION__, err.what());
    }

    return EXIT_FAILURE;
}
