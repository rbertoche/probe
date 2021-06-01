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

MulticastReceiver::MulticastReceiver(io_service& io_service,
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

	// Bind cria uma closure que armazena o this e o socket e outros argumentos
	// e retorna um callable de 1 argumento size_t
	socket_.async_receive_from( buffer(data_, max_length), sender_endpoint_,
				    boost::bind(&MulticastReceiver::handle_receive_from,
						this,
						&socket_,
						placeholders::error,
						placeholders::bytes_transferred));
	cerr << "Ouvindo MC em " << multicast_address << ":" << multicast_port<< endl;
}


void MulticastReceiver::handle_receive_from(ip::udp::socket* socket_,
					    const boost::system::error_code& error,
					    size_t bytes_recvd)
{
	if (!error)
	{
		cout.write(data_, bytes_recvd);
		cout << endl;

		vector<char> buf(bytes_recvd);
		socket_->async_receive_from( buffer(&buf[0], buf.size()), sender_endpoint_,
					    boost::bind(&MulticastReceiver::handle_receive_from,
							this,
							socket_,
							placeholders::error,
							placeholders::bytes_transferred));

		respond(sender_endpoint_, buf);
	}
}


BaseMulticastServer::BaseMulticastServer(io_service& io_service,
		       const ip::address& multicast_address,
		       const ip::address& listen_address)
	: SocketOwner<ip::udp::socket>(io_service)
	, MulticastReceiver(io_service,
			    socket_,
			    multicast_address,
			    listen_address)
	, UDPSender(multicast_address)
{
}

UDPSender::UDPSender(const ip::address& multicast_address)
	: message_count_(0)
	, endpoint_(multicast_address, multicast_port)
{
}

void UDPSender::handle_send_to(const boost::system::error_code& error)
{
	if (!error)
	{
	}
}
