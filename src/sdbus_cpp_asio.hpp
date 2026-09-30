/***************************************************************************
 *   Copyright © 2026 by Andrey Afletdinov <public.irkutsk@gmail.com>      *
 *                                                                         *
 *   https://github.com/AndreyBarmaley/sdbus-cpp-asio                      *
 *                                                                         *
 *   MIT License                                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef _SDBUS_CPP_ASIO_
#define _SDBUS_CPP_ASIO_

#include <chrono>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include <sdbus-c++/sdbus-c++.h>

namespace SDBus {
    class AsioCoroConnector {
        std::unique_ptr<sdbus::IConnection> dbus_conn_;
        boost::asio::cancellation_signal sdbus_cancel_;

      protected:
        using posix_descriptor = boost::asio::posix::stream_descriptor;

        template<typename ExecutionContext>
        struct weak_stream_descriptor : posix_descriptor {
            weak_stream_descriptor(ExecutionContext & context, const posix_descriptor::native_handle_type & fd)
                : posix_descriptor(context, fd) {
            }

            ~weak_stream_descriptor() {
                release();
            }
        };

#ifdef SDBUS_2_0_API
        [[nodiscard]] boost::asio::awaitable<void> waitPollDataEventFd(const sdbus::IConnection::PollData & pollData) {
            auto ex = co_await boost::asio::this_coro::executor;
            weak_stream_descriptor sd{ex, pollData.eventFd};

            co_await sd.async_wait(posix_descriptor::wait_read,
                                   boost::asio::use_awaitable);

            co_return;
        }
#endif

        [[nodiscard]] boost::asio::awaitable<void> waitPollDataFd(const sdbus::IConnection::PollData & pollData) {
            auto ex = co_await boost::asio::this_coro::executor;
            weak_stream_descriptor sd{ex, pollData.fd};

            if((pollData.events & POLLIN) && (pollData.events & POLLOUT)) {
                using namespace boost::asio::experimental::awaitable_operators;
                co_await(
                    sd.async_wait(posix_descriptor::wait_read, boost::asio::use_awaitable) ||
                    sd.async_wait(posix_descriptor::wait_write, boost::asio::use_awaitable)
                );
            } else if(pollData.events & POLLIN) {
                co_await sd.async_wait(posix_descriptor::wait_read, boost::asio::use_awaitable);
            } else if(pollData.events & POLLOUT) {
                co_await sd.async_wait(posix_descriptor::wait_write, boost::asio::use_awaitable);
            }

            co_return;
        }

        [[nodiscard]] boost::asio::awaitable<void> waitTimeout(uint32_t timeout_ms) {
            auto ex = co_await boost::asio::this_coro::executor;
            boost::asio::steady_timer tm{ex, std::chrono::milliseconds(timeout_ms)};
            co_await tm.async_wait(boost::asio::use_awaitable);
            co_return;
        }

        [[nodiscard]] boost::asio::awaitable<void> sdbusEventProcess(void) {
            auto ex = co_await boost::asio::this_coro::executor;

            for(;;) {
                /*
                    https://github.com/Kistler-Group/sdbus-cpp/blob/v2.2.0/docs/using-sdbus-c++.md#using-sdbus-c-in-external-event-loops
                    -
                    Before each invocation of the I/O polling call,
                        IConnection::getEventLoopPollData() function should be invoked.
                    Returned PollData::fd file descriptor should be polled for the events indicated by PollData::events,
                        and the I/O call should block up to the returned PollData::timeout.
                    Additionally, returned PollData::eventFd should be polled for POLLIN events.
                    After each I/O polling call (for both PollData::fd and PollData::eventFd events),
                        the IConnection::processPendingEvent() method should be invoked.
                    This enables the bus connection to process any incoming or outgoing D-Bus messages.
                */
                auto pollData = dbus_conn_->getEventLoopPollData();

                if(const int timeout_ms = pollData.getPollTimeout(); 0 != timeout_ms) {
                    using namespace boost::asio::experimental::awaitable_operators;
                    // timeout_ms: -1 == infinity
#ifdef SDBUS_2_0_API
                    co_await(waitPollDataFd(pollData) || waitPollDataEventFd(pollData) || waitTimeout(timeout_ms));
#else
                    co_await(waitPollDataFd(pollData) || waitTimeout(timeout_ms));
#endif
                }

#ifdef SDBUS_2_0_API
                dbus_conn_->processPendingEvent();
#else
                dbus_conn_->processPendingRequest();
#endif
            }

            co_return;
        }

      public:
        explicit AsioCoroConnector(std::unique_ptr<sdbus::IConnection> && ptr) : dbus_conn_{std::move(ptr)} {
        }

        virtual ~AsioCoroConnector() {
            sdbus_cancel_.emit(boost::asio::cancellation_type::terminal);
        }

        void sdbusLoopCancel(void) {
            sdbus_cancel_.emit(boost::asio::cancellation_type::terminal);
        }

        [[nodiscard]] boost::asio::awaitable<void> sdbusEventLoop(void) {
            try {
                auto ex = co_await boost::asio::this_coro::executor;
                co_await boost::asio::co_spawn(ex, sdbusEventProcess(),
                                        boost::asio::bind_cancellation_slot(sdbus_cancel_.slot(), boost::asio::use_awaitable));
            } catch(const boost::system::system_error& err) {
                if(auto ec = err.code(); ec != boost::asio::error::operation_aborted) {
                    throw err;
                }
            }

            co_return;
        }

    };
} // SDBus namespace

#endif // _SDBUS_CPP_ASIO_
