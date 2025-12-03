#pragma once

#include <netinet/in.h>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

/**
 * @file udp_sender.hpp
 * @brief Simple UDP sender class.
 *
 * This class wraps a UDP socket through a unique_ptr<int, SocketDeleter>.
 * It never exposes raw file descriptors and never requires a custom destructor.
 */

/// Custom deleter for UDP socket wrapped in unique_ptr.
/// Automatically closes socket on destruction.
struct SocketDeleter {
    void operator()(int* fd) const noexcept;
};

/**
 * @brief Simple UDP sender.
 *
 * Uses a unique_ptr<int, SocketDeleter> to manage the lifetime of the
 * socket descriptor.
 */
class UdpSender {
public:
    UdpSender() = default;

    /**
     * @brief Construct and open a UDP socket to given IP and port.
     */
    UdpSender(const std::string& ip, int port);

    /**
     * @brief Open or re-open a UDP socket for given destination address.
     *
     * @param ip    IPv4 address string ("224.1.1.1", "127.0.0.1", etc.)
     * @param port  Destination UDP port.
     */
    void open(const std::string& ip, int port);

    /**
     * @return true if socket is open and ready to send.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Send a vector of values over UDP.
     *
     * @tparam T  Element type
     * @param vec Vector containing data to transmit
     */
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void send(const std::vector<T>& vec) const
    {
        if (!is_open() || vec.empty())
            return;

        const void* data = static_cast<const void*>(vec.data());
        const std::size_t bytes = vec.size() * sizeof(T);

        send_bytes_internal(data, bytes);
    }

private:
    std::unique_ptr<int, SocketDeleter> sock_fd_{};
    sockaddr_in addr_{};

    void send_bytes_internal(const void* data, std::size_t len) const;
};
