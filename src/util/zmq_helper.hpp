#pragma once
#include <zmq.hpp>
#include <glibmm/iochannel.h>

namespace horizon::zmq_helper {
// silly wrappers to dodge warnings about deprecated functions
// can't use the non-deprecated functions as they're not available in debian stable
bool recv(zmq::socket_t &sock, zmq::message_t &msg);
bool send(zmq::socket_t &sock, zmq::message_t &msg);
void subscribe_int(zmq::socket_t &sock, uint32_t value);
Glib::RefPtr<Glib::IOChannel> io_channel_from_socket(zmq::socket_t &sock);
bool can_recv(zmq::socket_t &sock);
void set_timeouts(zmq::socket_t &sock, int timeout);
std::string get_last_endpoint(const zmq::socket_t &sock);
} // namespace horizon::zmq_helper
