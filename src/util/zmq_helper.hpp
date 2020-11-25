#pragma once
#include <zmq.hpp>

namespace horizon::zmq_helper {
bool recv(zmq::socket_t &sock, zmq::message_t &msg);
bool send(zmq::socket_t &sock, zmq::message_t &msg);
} // namespace horizon::zmq_helper