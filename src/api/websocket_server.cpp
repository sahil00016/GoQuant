#include "api/websocket_server.hpp"
#include <iostream>
#include <sstream>

namespace crypto_matching_engine {

WebSocketServer::WebSocketServer() {
    // Set up the server
    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.set_access_channels(websocketpp::log::alevel::connect);
    server_.set_access_channels(websocketpp::log::alevel::disconnect);
    server_.set_access_channels(websocketpp::log::alevel::app);
    
    // Initialize ASIO
    server_.init_asio();
    
    // Set up handlers
    server_.set_message_handler(
        [this](auto hdl, auto msg) { onMessage(hdl, msg); });
    server_.set_open_handler(
        [this](auto hdl) { onConnection(hdl); });
    server_.set_close_handler(
        [this](auto hdl) { onDisconnection(hdl); });
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start(uint16_t port) {
    if (running_) return;
    
    running_ = true;
    server_thread_ = std::thread(&WebSocketServer::runServer, this, port);
}

void WebSocketServer::stop() {
    if (!running_) return;
    
    running_ = false;
    server_.stop();
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebSocketServer::setMessageCallback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void WebSocketServer::setConnectionCallback(ConnectionCallback callback) {
    connection_callback_ = std::move(callback);
}

void WebSocketServer::setDisconnectionCallback(ConnectionCallback callback) {
    disconnection_callback_ = std::move(callback);
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (const auto& [hdl, _] : connections_) {
        try {
            server_.send(hdl, message, websocketpp::frame::opcode::text);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what() << std::endl;
        }
    }
}

void WebSocketServer::send(ConnectionHdl hdl, const std::string& message) {
    try {
        server_.send(hdl, message, websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

void WebSocketServer::onMessage(ConnectionHdl hdl, Server::message_ptr msg) {
    if (message_callback_) {
        std::string client_id;
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            auto it = connections_.find(hdl);
            if (it != connections_.end()) {
                client_id = it->second;
            }
        }
        message_callback_(client_id, msg->get_payload());
    }
}

void WebSocketServer::onConnection(ConnectionHdl hdl) {
    // Generate a unique client ID
    std::stringstream ss;
    ss << "client_" << hdl.lock().get();
    std::string client_id = ss.str();
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[hdl] = client_id;
    }
    
    if (connection_callback_) {
        connection_callback_(hdl);
    }
}

void WebSocketServer::onDisconnection(ConnectionHdl hdl) {
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(hdl);
    }
    
    if (disconnection_callback_) {
        disconnection_callback_(hdl);
    }
}

void WebSocketServer::runServer(uint16_t port) {
    try {
        server_.listen(port);
        server_.start_accept();
        server_.run();
    } catch (const std::exception& e) {
        std::cerr << "Error running WebSocket server: " << e.what() << std::endl;
    }
}

} // namespace crypto_matching_engine 