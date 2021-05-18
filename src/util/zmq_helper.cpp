#include "zmq_helper.hpp"

#ifdef CPPZMQ_VERSION

#if CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1)
#define V431
#endif

/*
this should be .4,7,0 but the zmq.hpp packaged by debian claims to be 4.7.0 but doesn't provide zmq::sockopt
so bump this to 4.7.1 until this is fixed by debian
see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=988725
*/

#if CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 7, 1)
#define V470
#endif

#endif

namespace horizon::zmq_helper {

bool recv(zmq::socket_t &sock, zmq::message_t &msg)
{
#ifdef V431
    return sock.recv(msg).has_value();
#else
    return sock.recv(&msg);
#endif
}

bool send(zmq::socket_t &sock, zmq::message_t &msg)
{
#ifdef V431
    return sock.send(msg, zmq::send_flags::none).has_value();
#else
    return sock.send(msg);
#endif
}

void subscribe_int(zmq::socket_t &sock, uint32_t value)
{
#ifdef V470
    zmq::const_buffer buf(&value, sizeof(value));
    sock.set(zmq::sockopt::subscribe, buf);
#else
    sock.setsockopt(ZMQ_SUBSCRIBE, &value, sizeof(uint32_t));
#endif
}

Glib::RefPtr<Glib::IOChannel> io_channel_from_socket(zmq::socket_t &sock)
{
#ifdef V470
    auto fd = sock.get(zmq::sockopt::fd);
#else
#ifdef G_OS_WIN32
    SOCKET fd = sock_broadcast_rx.getsockopt<SOCKET>(ZMQ_FD);
#else
    int fd = sock.getsockopt<int>(ZMQ_FD);
#endif
#endif

#ifdef G_OS_WIN32
    return Glib::IOChannel::create_from_win32_socket(fd);
#else
    return Glib::IOChannel::create_from_fd(fd);
#endif
}

bool can_recv(zmq::socket_t &sock)
{
#ifdef V470
    return sock.get(zmq::sockopt::events) & ZMQ_POLLIN;
#else
    return sock.getsockopt<int>(ZMQ_EVENTS) & ZMQ_POLLIN;
#endif
}

void set_timeouts(zmq::socket_t &sock, int timeout)
{
#ifdef V470
    sock.set(zmq::sockopt::rcvtimeo, timeout);
    sock.set(zmq::sockopt::sndtimeo, timeout);
#else
    sock.setsockopt(ZMQ_RCVTIMEO, timeout);
    sock.setsockopt(ZMQ_SNDTIMEO, timeout);
#endif
}

std::string get_last_endpoint(const zmq::socket_t &sock)
{
#ifdef V470
    return sock.get(zmq::sockopt::last_endpoint);
#else
    char ep[1024];
    size_t sz = sizeof(ep);
    sock.getsockopt(ZMQ_LAST_ENDPOINT, ep, &sz);
    return ep;
#endif
}

} // namespace horizon::zmq_helper
