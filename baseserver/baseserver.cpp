//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <iostream>

#include "baseserver.h"

const short multicast_port = 9921;

const int max_message_count = 10;

using namespace std;
using namespace boost::asio;

// O socket_ aqui é necessário pois não é possível
// executar métodos virtuais. Note que a referência
// não é copiada.

Receiver::Receiver(io_service& io_service,
		   ip::udp::socket& socket_,
		   const ip::address& multicast_address,
		   const ip::address& listen_address)
{
	// Create the socket so that multiple may be bound to the same address.
	ip::udp::endpoint listen_endpoint(listen_address, multicast_port);
	socket_.open(listen_endpoint.protocol());
	socket_.set_option(ip::udp::socket::reuse_address(true));
	socket_.bind(listen_endpoint);

	// Join the multicast group.
	ip::multicast::join_group option(multicast_address);
	boost::system::error_code ec;
	socket_.set_option( option, ec);

	socket_.async_receive_from( buffer(data_, max_length), sender_endpoint_,
				    boost::bind(&Receiver::handle_receive_from, this,
						placeholders::error,
						placeholders::bytes_transferred));
	cerr << "Ouvindo MC em " << multicast_address << ":" << multicast_port<< endl;
}


void Receiver::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
	if (!error)
	{
		cout.write(data_, bytes_recvd);
		cout << endl;

		vector<char> buf(bytes_recvd);
		socket().async_receive_from( buffer(&buf[0], buf.size()), sender_endpoint_,
					    boost::bind(&Receiver::handle_receive_from, this,
							placeholders::error,
							placeholders::bytes_transferred));

		respond(sender_endpoint_, buf);
	}
}


ip::udp::endpoint Sender::getEndpoint()
{
	return endpoint_;
}

Sender::Sender(io_service& io_service,
	       const ip::address& multicast_address)
	: timer_(io_service)
	, message_count_(0)
	, endpoint_(multicast_address, multicast_port)
{
}

void Sender::handle_send_to(const boost::system::error_code& error)
{
	if (!error && message_count_ < max_message_count)
	{
		timer_.expires_from_now(boost::posix_time::seconds(1));
		timer_.async_wait( boost::bind(&Sender::handle_timeout, this,
					       placeholders::error));
	}
}


void Sender::handle_timeout(const boost::system::error_code& error)
{
	if (!error)
	{
		ostringstream os;
		os << "Message " << message_count_++;
		message_ = os.str();

		socket().async_send_to( buffer(message_), endpoint_,
				       boost::bind(&Sender::handle_send_to, this,
						   placeholders::error));
	}
}


BaseServer::BaseServer(io_service& io_service,
		       const ip::address& multicast_address,
		       const ip::address& listen_address)
	: SocketOwner(io_service)
	, Receiver(io_service,
		   socket_,
		   multicast_address,
		   listen_address)
	, Sender(io_service,
		 multicast_address)
{}

ip::udp::socket&BaseServer::socket()
{
	return socket_;
}
