#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

namespace crypto_matching_engine {

class WebSocketServer {
public:
    using MessageCallback = std::function<void(const std::string&, const std::string&)>;
    using ConnectionCallback = std::function<void(websocketpp::connection_hdl)>;
    
    WebSocketServer();
    ~WebSocketServer();
    
    void start(uint16_t port);
    void stop();
    
    void setMessageCallback(MessageCallback callback);
    void setConnectionCallback(ConnectionCallback callback);
    void setDisconnectionCallback(ConnectionCallback callback);
    
    void broadcast(const std::string& message);
    void send(websocketpp::connection_hdl hdl, const std::string& message);

private:
    using Server = websocketpp::server<websocketpp::config::asio>;
    using ConnectionHdl = websocketpp::connection_hdl;
    
    Server server_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    
    MessageCallback message_callback_;
    ConnectionCallback connection_callback_;
    ConnectionCallback disconnection_callback_;
    
    std::unordered_map<ConnectionHdl, std::string, std::hash<void*>> connections_;
    std::mutex connections_mutex_;
    
    void onMessage(ConnectionHdl hdl, Server::message_ptr msg);
    void onConnection(ConnectionHdl hdl);
    void onDisconnection(ConnectionHdl hdl);
    void runServer(uint16_t port);
};

} // namespace crypto_matching_engine 