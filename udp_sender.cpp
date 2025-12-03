#include "udp_sender.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>


enum class AddressType {
    Unicast,
    Multicast,
    Broadcast
};

static AddressType detect_address_type(const in_addr& addr)
{
    uint32_t ip = ntohl(addr.s_addr);

    // 224.0.0.0/4  => Multicast
    if ((ip & 0xF0000000u) == 0xE0000000u)
        return AddressType::Multicast;

    if (ip == 0xFFFFFFFFu)
        return AddressType::Broadcast;

    // Directed broadcast: last octet == 255
    if ((ip & 0xFFu) == 0xFFu)
        return AddressType::Broadcast;

    return AddressType::Unicast;
}

void SocketDeleter::operator()(int* fd) const noexcept {
    if (fd) {
        if (*fd >= 0) {
            ::close(*fd);
        }
        delete fd;
    }
}


UdpSender::UdpSender(const std::string& ip, int port) {
    open(ip, port);
}

void UdpSender::open(const std::string& ip, int port) {
    sock_fd_.reset(new int(socket(AF_INET, SOCK_DGRAM, 0)));
    if (*sock_fd_ < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }

    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port   = htons(static_cast<uint16_t>(port));

    if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) != 1) {
        throw std::runtime_error("Invalid IPv4 address");
    }

    AddressType type = detect_address_type(addr_.sin_addr);

    switch (type) {
    case AddressType::Multicast: {
        unsigned char ttl = 1;
        setsockopt(*sock_fd_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

        unsigned char loop = 0;
        setsockopt(*sock_fd_, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        break;
    }
    case AddressType::Broadcast: {
        int yes = 1;
        setsockopt(*sock_fd_, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
        break;
    }
    case AddressType::Unicast:
    default:
        break;
    }
}

bool UdpSender::is_open() const noexcept {
    return sock_fd_ && *sock_fd_ >= 0;
}

void UdpSender::send_bytes_internal(const void* data, std::size_t len) const {
    if (!is_open() || !data || len == 0) {
        return;
    }

    sendto(*sock_fd_,
           data,
           len,
           0,
           reinterpret_cast<const sockaddr*>(&addr_),
           sizeof(addr_));
}
