# sdbus-cpp-asio

A lightweight, high-performance C++ integration wrapper that connects **sdbus-cpp** with the **boost::asio** asynchronous event loop using modern **C++20 Coroutines**.

This library allows you to run standard D-Bus event loops directly inside a Boost.Asio execution context (`boost::asio::io_context`), eliminating the need for separate worker threads or blocking calls.

## Features

- **Asynchronous & Non-blocking:** Integrates [sdbus-cpp](https://github.com/Kistler-Group/sdbus-cpp) event processing cleanly into the Boost.Asio event loop.
- **C++20 Coroutines:** Leverages `boost::asio::awaitable` and `co_await` for clean, linear asynchronous code structure.
- **Efficient Resource Management:** Uses `boost::asio::posix::stream_descriptor` to asynchronously monitor D-Bus file descriptors (`fd`).
- **Dual sdbus-cpp API Support:** Fully compatible with both older `sdbus-cpp` versions and the modern **v2.0+ API** (automatically handles separate poll data events via `SDBUS_2_0_API`).

## Interface Overview

```cpp
namespace SDBus {
    class AsioCoroConnector {
    public:
        // Takes ownership of a configured sdbus connection
        explicit AsioCoroConnector(std::unique_ptr<sdbus::IConnection>&&);
        virtual ~AsioCoroConnector();

        // Starts the asynchronous processing loop (should be co_awaited within an asio context)
        [[nodiscard]] boost::asio::awaitable<void> sdbusEventLoop(void);

        // Cancels the running D-Bus event loop safely
        void sdbusLoopCancel(void);
    };
}
```

## Prerequisites

To build and use this wrapper, you need:
- A C++20 compliant compiler (e.g., GCC 10+, Clang 11+, or MSVC 2019+)
- **Boost Libraries** (version 1.80+ recommended, specifically `boost::asio`)
- **sdbus-cpp** library (supports both v1.x and v2.x)
- An active D-Bus system or session daemon

## Quick Start Example

Here is a basic example of how to initialize and run the `AsioCoroConnector` within a `boost::asio::io_context`:

```cpp
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include "sdbus_cpp_asio.hpp"

boost::asio::awaitable<void> runApp() {
    try {
        // 1. Create a standard sdbus connection (e.g., to the Session Bus)
        auto sdbusConnection = sdbus::createSessionBusConnection("com.example.AsioService");
        
        // (Optional) Register your D-Bus objects, interfaces, and methods here
        // ...

        // 2. Create the connector and pass ownership of the connection
        SDBus::AsioCoroConnector connector(std::move(sdbusConnection));

        std::cout << "D-Bus event loop is running via Boost.Asio coroutine..." << std::endl;
        
        // 3. Co_await the loop. It will run until canceled or the context stops.
        co_await connector.sdbusEventLoop();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in coroutine: " << e.what() << std::endl;
    }
}

int main() {
    boost::asio::io_context io_ctx;

    // Spawn the coroutine within the io_context
    boost::asio::co_spawn(io_ctx, runApp(), boost::asio::detached);

    // Run the Boost.Asio event loop
    io_ctx.run();

    return 0;
}
```

## Compilation

If you are using the newer `sdbus-cpp` **v2.0 API**, make sure to define the corresponding macro during compilation:

```bash
g++ -std=c++20 main.cpp sdbus_cpp_asio.hpp -DSDBUS_2_0_API -lsdbus-c++ -lboost_system
```

## License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.
