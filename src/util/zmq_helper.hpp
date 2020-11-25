#pragma once
#include <zmq.hpp>

namespace horizon::zmq_helper {
// silly wrappers to dodge warnings about deprecated functions
// can't use the non-deprecated functions as they're not available in debian stable
bool recv(zmq::socket_t &sock, zmq::message_t &msg);
bool send(zmq::socket_t &sock, zmq::message_t &msg);
} // namespace horizon::zmq_helper