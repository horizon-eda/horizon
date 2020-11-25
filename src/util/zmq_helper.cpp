#include "zmq_helper.hpp"

#ifdef CPPZMQ_VERSION
#if CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1)
#define V431
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

} // namespace horizon::zmq_helper
