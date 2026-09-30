/***************************************************************************
 *   Copyright © 2026 by Andrey Afletdinov <public.irkutsk@gmail.com>      *
 *                                                                         *
 *   https://github.com/AndreyBarmaley/sdbus-cpp-asio                      *
 *                                                                         *
 *   MIT License                                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef _TEST_SESSION_PROXY_
#define _TEST_SESSION_PROXY_

#include <atomic>
#include <boost/asio.hpp>

#include "asio_test_proxy.h"

namespace Test {
    class SessionProxy : public sdbus::ProxyInterfaces<org::sdbus_cpp::asio::service_proxy> {
        boost::asio::io_context ioc_;
        boost::asio::signal_set signals_;
        boost::asio::steady_timer timer_sdbus_;

        std::atomic<bool> success_{true};
        std::atomic<int> value_{0};

      protected:
        void timerCalled(const boost::system::error_code &);
        void stop(void);

        void onTestSignal(const int32_t & id, const int32_t & val) override;

      public:
        SessionProxy();
        virtual ~SessionProxy();

        int start(void);
    };
}

#endif // _TEST_PROXY_
