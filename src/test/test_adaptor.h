/***************************************************************************
 *   Copyright © 2026 by Andrey Afletdinov <public.irkutsk@gmail.com>      *
 *                                                                         *
 *   https://github.com/AndreyBarmaley/sdbus-cpp-asio                      *
 *                                                                         *
 *   MIT License                                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef _TEST_SESSION_ADAPTOR_
#define _TEST_SESSION_ADAPTOR_

#include <string>
#include <memory>

#include <boost/asio.hpp>

#include "asio_test_adaptor.h"
#include "sdbus_cpp_asio.hpp"

namespace Test {
    using StdoutBuf = std::vector<uint8_t>;
    using StatusStdout = sdbus::Struct<int32_t, StdoutBuf>;
    using ArgsList = std::vector<std::string>;

    using DBusConncetionPtr = std::unique_ptr<sdbus::IConnection>;

    class SessionAdaptor : public sdbus::AdaptorInterfaces<org::sdbus_cpp::asio::service_adaptor>, protected SDBus::AsioCoroConnector {
        const int threads_ = 2;
        boost::asio::io_context ioc_{threads_};
        boost::asio::signal_set signals_;
        int value_ = 0;

      protected:
        boost::asio::awaitable<void> signalsHandler(void);
        boost::asio::awaitable<void> sdbusHandler(void);
        void stop(void);

      public:
        SessionAdaptor(DBusConncetionPtr);
        virtual ~SessionAdaptor();

        int start(void);

        // dbus interface
        int32_t GetVersion(void) override;
        void ServiceShutdown(void) override;
        void TestJob(const int32_t & id, const int32_t & val) override;
    };
}

#endif // _TEST_SESSION_ADAPTOR_
