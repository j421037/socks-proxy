//
// Created by Miaoyi on 2020/11/9.
//

#ifndef PROXY_TUNNEL_H
#define PROXY_TUNNEL_H
// std
#include <memory>

// thirties
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/TcpClient.h"

class Tunnel : public std::enable_shared_from_this<Tunnel> {
public:
    Tunnel(muduo::net::EventLoop* loop,
           const muduo::net::InetAddress& serverAddr,
           const muduo::net::TcpConnectionPtr& serverConn)
           :client_(loop, serverAddr, serverConn->name()),
           serverConn_(serverConn) {
        LOG_INFO << "Tunnel::Tunnel " << serverConn_->peerAddress().toIpPort()
                 << "<->" << serverAddr.toIpPort();
    }

    ~Tunnel() {
        LOG_INFO << "~Tunnel";
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect() {
        client_.disconnect();
    }

    void onClientConnection(const muduo::net::TcpConnectionPtr& conn)
    {
        LOG_DEBUG << (conn->connected() ? "UP" : "DOWN");
        if ( conn->connected() ) {
            conn->setTcpNoDelay(true);
            conn->setHighWaterMarkCallback(std::bind(
                    &Tunnel::onHighWaterMarkWeak,
                    std::weak_ptr<Tunnel>(shared_from_this()),
                    std::placeholders::_1,
                    std::placeholders::_2), 10 * 1024 * 1024);

            serverConn_->setContext(conn);
            if ( serverConn_->inputBuffer()->readableBytes() > 0 ) {
                conn->send(serverConn_->inputBuffer());
            }
        }
        else {
            tearDown();
        }
    }

    void onHighWaterMark(const muduo::net::TcpConnectionPtr& conn, size_t byteToSent)
    {
        LOG_INFO << "onHighWaterMark" << conn->name()
                 << "bytes" << byteToSent;
        disconnect();
    }

    static void onHighWaterMarkWeak(const std::weak_ptr<Tunnel>& wkTunnel,
                                    const muduo::net::TcpConnectionPtr& conn,
                                    size_t byteToSent)
    {
        std::shared_ptr<Tunnel> tunnel = wkTunnel.lock();
        if ( tunnel ) {
            tunnel->onHighWaterMark(conn, byteToSent);
        }
    }

    void onClientMessage(const muduo::net::TcpConnectionPtr& conn,
                         muduo::net::Buffer* buf,
                         muduo::Timestamp timestamp)
    {
        LOG_DEBUG << conn->name() << " | " << buf->readableBytes();
        if ( serverConn_ ) {
            serverConn_->send(buf);
            // LOG_INFO << "message: " << buf->retrieveAsString(buf->readableBytes());
        }
        else {
            buf->retrieveAll();
            abort();
        }
    }

    void tearDown()
    {
        client_.setConnectionCallback(muduo::net::defaultConnectionCallback);
        client_.setMessageCallback(muduo::net::defaultMessageCallback);

        if ( serverConn_ ) {
            serverConn_->setContext(boost::any());
            serverConn_->shutdown();
        }
    }

    void setup()
    {
        client_.setConnectionCallback(std::bind(&Tunnel::onClientConnection, shared_from_this(), std::placeholders::_1));
        client_.setMessageCallback(std::bind(&Tunnel::onClientMessage,
                                             shared_from_this(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3));
        serverConn_->setHighWaterMarkCallback(
                std::bind(&Tunnel::onHighWaterMarkWeak, std::weak_ptr<Tunnel>(shared_from_this()),
                        std::placeholders::_1,
                        std::placeholders::_2), 10 * 1024 * 1024);
    }
private:
    muduo::net::TcpClient client_;
    muduo::net::TcpConnectionPtr serverConn_;
    muduo::net::TcpConnectionPtr clientConn_;
};

using TunnelPtr = std::shared_ptr<Tunnel>;

#endif //PROXY_TUNNEL_H
