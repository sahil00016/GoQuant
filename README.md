# Cryptocurrency Matching Engine

## Project Overview

This project implements a high-performance cryptocurrency matching engine, designed to process and match buy and sell orders for various cryptocurrency pairs. The engine is built with concurrency in mind to handle high throughput and provides an HTTP API for external interaction.

## Assignment Goals

The primary objective of this assignment was to develop a cryptocurrency matching engine with the following key characteristics:

*   **High-Performance:** The engine should be capable of efficiently processing a large volume of orders. This implies efficient data structures, concurrent processing, and minimal latency.
*   **Support for Various Order Types:** The engine must handle common order types found in financial markets, including:
    *   **Limit Orders:** Orders to buy or sell at a specified price or better.
    *   **Market Orders:** Orders to buy or sell immediately at the best available current price.
    *   **Immediate-Or-Cancel (IOC) Orders:** Orders that are executed immediately and any unfulfilled portion is canceled.
    *   **Fill-Or-Kill (FOK) Orders:** Orders that must be entirely filled immediately or canceled completely.
*   **"REG NMS-Inspired Principles":** While REG NMS (Regulation National Market System) is a U.S. regulation for equities, its core principle of achieving "best execution" for orders is relevant. For this cryptocurrency engine, this translates to prioritizing efficient matching, fair pricing, and robust handling of order execution scenarios (like IOC and FOK).
*   **Real-time Operation:** The engine should process orders and update market data in real-time.
*   **API for Interaction:** Provide an interface for clients to submit orders, query market data, and receive updates.

## Features Implemented

Through the development process, we have successfully implemented the following features:

*   **Core Matching Engine Logic:**
    *   A robust order book mechanism that manages buy (bids) and sell (asks) orders for specific cryptocurrency symbols.
    *   Efficient matching algorithms that correctly pair opposing orders based on price and time priority.
*   **Comprehensive Order Type Support:**
    *   **Limit Orders:** Orders are placed on the order book if not immediately filled.
    *   **Market Orders:** Execute immediately against available liquidity on the order book.
    *   **Immediate-Or-Cancel (IOC):** Any remaining quantity after immediate execution is canceled.
    *   **Fill-Or-Kill (FOK):** The entire order must be filled immediately, or it is canceled.
*   **Asynchronous Order Processing:**
    *   The `MatchingEngine` utilizes a dedicated background thread (`processing_thread_`) to asynchronously process order events (submit, cancel, modify) from a queue. This prevents the main application thread from blocking and ensures sequential, thread-safe processing of orders.
    *   Mutexes (`std::mutex`) and condition variables (`std::condition_variable`) are employed to ensure thread safety when accessing shared data structures like the order queue and order books.
*   **HTTP API for Client Interaction:**
    *   A RESTful API is provided using the `cpp-httplib` library, allowing external clients to interact with the matching engine.
    *   **Endpoints:**
        *   `POST /order`: Submit new buy/sell orders.
        *   `GET /orderbook/:symbol`: Retrieve the current depth (price levels and total quantities) for a specific cryptocurrency symbol.
        *   `DELETE /order/:symbol/:id`: Cancel an existing order by its ID.
*   **Robustness and Error Handling:**
    *   Implemented `try-catch` blocks in critical sections (e.g., HTTP server startup, order processing loop) to catch and log exceptions, improving the application's stability.
    *   Addressed and resolved complex concurrency issues, including a "device or resource busy" error caused by recursive mutex locking, ensuring the application runs without crashing during order processing.
*   **Build System:** Configured with CMake for easy compilation across different platforms, managing dependencies via `vcpkg`.

## How to Run the Project

This guide provides step-by-step instructions to set up and run the cryptocurrency matching engine on your local machine.

### Prerequisites

Before you begin, ensure you have the following installed on your system:

1.  **Git:** For cloning the repository.
    *   [Download Git](https://git-scm.com/downloads)
2.  **CMake (version 3.10 or higher):** For managing the build process.
    *   [Download CMake](https://cmake.org/download/)
3.  **C++ Compiler:** A modern C++ compiler that supports C++20 standard (e.g., MSVC on Windows, GCC/Clang on Linux/macOS).
    *   **Windows:** Visual Studio with C++ desktop development workload.
    *   **Linux/macOS:** GCC or Clang (usually installed with build essentials).
4.  **Vcpkg:** A C++ package manager used for managing external dependencies like `nlohmann/json` and `cpp-httplib`.
    *   **Installation (PowerShell on Windows):**
        ```powershell
        git clone https://github.com/microsoft/vcpkg
        .\vcpkg\bootstrap-vcpkg.bat
        ```
    *   **Installation (Linux/macOS):**
        ```bash
        git clone https://github.com/microsoft/vcpkg
        ./vcpkg/bootstrap-vcpkg.sh
        ```

### Setup Steps

1.  **Clone the Repository:**
    Navigate to your desired directory and clone the project:
    ```bash
    git clone <Your-Repository-URL> # Replace with your actual GitHub repository URL
    cd goQuant # Or whatever your repository's root directory is named
    ```

2.  **Install Dependencies using Vcpkg:**
    Navigate to the `vcpkg` directory within your cloned repository and install the required libraries. This might take some time as `vcpkg` compiles the dependencies.

    *   **Windows (PowerShell):**
        ```powershell
        cd vcpkg
        .\vcpkg.exe install nlohmann-json:x64-windows cpp-httplib:x64-windows
        cd .. # Navigate back to the project root
        ```
    *   **Linux/macOS (Bash):**
        ```bash
        cd vcpkg
        ./vcpkg install nlohmann-json cpp-httplib
        cd .. # Navigate back to the project root
        ```

3.  **Build the Project:**
    Create a `build` directory and use CMake to configure and build the project. This process will compile the source code and link the necessary libraries.

    *   **Windows (PowerShell - from project root, e.g., `goQuant`):**
        ```powershell
        mkdir build
        cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
        cmake --build . --config Debug # Or Release for optimized build
        ```
    *   **Linux/macOS (Bash - from project root, e.g., `goQuant`):**
        ```bash
        mkdir build
        cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
        cmake --build .
        ```
    If the build is successful, an executable named `matching_engine.exe` (on Windows) or `matching_engine` (on Linux/macOS) will be generated in the `Debug` (or `Release`) subdirectory within your `build` folder.

4.  **Run the Application:**
    From the `build` directory, execute the generated program.

    *   **Windows (PowerShell):**
        ```powershell
        .\Debug\matching_engine.exe
        ```
    *   **Linux/macOS (Bash):**
        ```bash
        ./matching_engine
        ```

### Expected Output in Terminal

Upon running the application, you should see output similar to this, indicating the HTTP server has started and test orders are being submitted:

```
Starting HTTP server on port 8081
Submitting order: {"id":1,"price":312.9164648456355,"quantity":5.0866156661231665,"side":"sell","symbol":"BTC/USD","timestamp":1749573727597,"type":"market"}
Submitting order: {"id":2,"price":199.4675212740445,"quantity":7.626646301543418,"side":"sell","symbol":"BTC/USD","timestamp":1749573727697,"type":"market"}
Submitting order: {"id":3,"price":917.1456004522128,"quantity":2.6731677175336515,"side":"sell","symbol":"BTC/USD","timestamp":1749573727807,"type":"ioc"}
# ... (more order submission logs) ...
```

The application will continue to run and keep the HTTP server active until you manually stop it (e.g., by pressing `Ctrl+C` in the terminal).

### API Usage Examples (using `curl`)

Once the server is running on `localhost:8081`, you can interact with it using `curl` or any API client (like Postman/Insomnia).

*   **Submit a Limit Buy Order:**
    ```bash
    curl -X POST http://localhost:8081/order -H "Content-Type: application/json" -d "{
        \"id\": 101,
        \"symbol\": \"BTC/USD\",
        \"side\": \"buy\",
        \"type\": \"limit\",
        \"quantity\": 0.5,
        \"price\": 50000.0
    }"
    ```

*   **Submit a Market Sell Order:**
    ```bash
    curl -X POST http://localhost:8081/order -H "Content-Type: application/json" -d "{
        \"id\": 102,
        \"symbol\": \"BTC/USD\",
        \"side\": \"sell\",
        \"type\": \"market\",
        \"quantity\": 1.2
    }"
    ```

*   **Get Order Book Depth for BTC/USD:**
    ```bash
    curl http://localhost:8081/orderbook/BTC/USD
    ```
    (Expected output will be a JSON array of price-quantity pairs for bids and asks.)

*   **Cancel an Order (e.g., order ID 101 for BTC/USD):**
    ```bash
    curl -X DELETE http://localhost:8081/order/BTC/USD/101
    ```

---

Feel free to explore the codebase. The main components are in `src/` and `include/` directories.

**Enjoy your high-performance matching engine!** 