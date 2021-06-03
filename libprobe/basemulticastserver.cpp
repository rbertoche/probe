//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <iostream>

#include "basemulticastserver.h"

#include "dump.h"

#define DEBUG

const int max_message_count = 10;

enum { max_length = 4 };

using namespace std;
using namespace boost::asio;

// O socket_ aqui é necessário pois não é possível
// executar métodos virtuais. Note que a referência
// não é copiada.

MulticastReceiver::MulticastReceiver(io_service& io_service,
				     ip::udp::socket& socket_,
				     const ip::address& multicast_address,
				     unsigned short port,
				     const ip::address& listen_address)
	: mc_endpoint(multicast_address, port)
	, data_(max_length)
{
	// Create the socket so that multiple may be bound to the same address.
	ip::udp::endpoint listen_endpoint(listen_address, port);
	socket_.open(listen_endpoint.protocol());
	socket_.set_option(ip::udp::socket::reuse_address(true));
	socket_.bind(listen_endpoint);

	// Join the multicast group.
	ip::multicast::join_group option(multicast_address);
	boost::system::error_code ec;
	socket_.set_option( option, ec);

	// Bind cria uma closure que armazena o this e o socket e outros argumentos
	// e retorna um callable de 1 argumento size_t
	socket_.async_receive_from( buffer(data_), sender_endpoint_,
				    boost::bind(&MulticastReceiver::handle_receive_from,
						this,
						&socket_,
						placeholders::error,
						placeholders::bytes_transferred));
	cerr << "Ouvindo MC em " << multicast_address << ":" << port << endl;
}


void MulticastReceiver::handle_receive_from(ip::udp::socket* socket_,
					    const boost::system::error_code& error,
					    size_t bytes_recvd)
{
	if (!error) {
//		cout.write((char *) data_, bytes_recvd);

//		vector<unsigned char> buf(reinterpret_cast<unsigned char*>(data_.data()),
//					  bytes_recvd);

		respond(sender_endpoint_, data_);

		socket_->async_receive_from( buffer(data_), sender_endpoint_,
					    boost::bind(&MulticastReceiver::handle_receive_from,
							this,
							socket_,
							placeholders::error,
							placeholders::bytes_transferred));
	} else {
		cerr << "send_to levantou um erro:" << endl;
		cerr << error.category().name() << endl;
		cerr << error.message() << endl;
		cerr << error.value() << endl;
	}
}


BaseMulticastServer::BaseMulticastServer(io_service& io_service,
		       const ip::address& multicast_address,
		       unsigned short port,
		       const ip::address& listen_address)
	: SocketOwner<ip::udp::socket>(io_service)
	, MulticastReceiver(io_service,
			    socket_,
			    multicast_address,
			    port,
			    listen_address)
	, UDPSender(multicast_address,
		    port)
{
}

UDPSender::UDPSender(const ip::address& multicast_address,
		     unsigned int port)
	: message_count_(0)
	, endpoint_(multicast_address,
		    port)
{
}

void UDPSender::handle_send_to(const boost::system::error_code& error)
{
	if (!error) {
	} else {
		cerr << "send_to levantou um erro:" << endl;
		cerr << error.category().name() << endl;
		cerr << error.message() << endl;
		cerr << error.value() << endl;
	}
}

void UDPSender::send_to(ip::udp::socket& socket_,
			const std::vector<unsigned char>& data,
			ip::udp::endpoint destination)
{
#ifdef DEBUG
	cerr << "send:\t\t\t";
	dump(data);
#endif
	socket_.async_send_to(buffer(data.data(), data.size()),
			      destination,
			      boost::bind(&UDPSender::handle_send_to,
					  static_cast<BaseMulticastServer*>(this),
					  placeholders::error));
}
