#include <iostream>
#include "Tunnel.h"

using namespace muduo;
using namespace muduo::net;

EventLoop *g_loop;
InetAddress *g_serverAddr;
std::map<std::string, TunnelPtr> g_tunnels;

void onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << conn->name() << (conn->connected() ? " UP " : " DOWN ");
    if ( conn->connected() ) {
        conn->setTcpNoDelay(true);
        TunnelPtr tunnel(new Tunnel(g_loop, *g_serverAddr, conn));
        tunnel->setup();
        tunnel->connect();
        g_tunnels[conn->name()] = tunnel;
    }
    else {
        assert(g_tunnels.find(conn->name()) != g_tunnels.end());
        g_tunnels[conn->name()]->disconnect();
        g_tunnels.erase(conn->name());
    }
}

void onServerMessage(const TcpConnectionPtr& conn,  Buffer* buf, Timestamp timestamp)
{
    LOG_DEBUG << "main::onServerMessage " << buf->readableBytes();
    if ( !conn->getContext().empty() ) {
        const TcpConnectionPtr& clientConn = boost::any_cast<const TcpConnectionPtr>(conn->getContext());
        clientConn->send(buf);
    }
}

int main() {
//    std::cout << "Hello, World!" << std::endl;
    LOG_INFO << "PID = " << getpid() << " tid = " << CurrentThread::tid();
    // size_t mb = 1024 * 1024;

    const char *ip = "127.0.0.1";
    uint16_t port = static_cast<uint16_t>(atoi("22"));
    InetAddress serverAddr(ip, port);
    g_serverAddr = &serverAddr;

    // accept port
    uint16_t acceptPort = static_cast<uint16_t>(atoi("6666"));
    InetAddress listenAddr(acceptPort);

    EventLoop loop;
    g_loop = &loop;

    TcpServer server(&loop, listenAddr, "TcpRelay");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onServerMessage);

    server.start();
    loop.loop();

    return 0;
}
