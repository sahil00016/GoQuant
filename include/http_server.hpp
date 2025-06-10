#pragma once

#include "matching_engine.hpp"
#include <httplib.h>

namespace crypto_matching_engine {

class HttpServer {
public:
    explicit HttpServer(MatchingEngine& engine);
    void start(int port);

private:
    MatchingEngine& engine_;
    httplib::Server server_;
};

} // namespace crypto_matching_engine 