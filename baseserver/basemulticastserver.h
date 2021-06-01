//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BASESERVER_H
#define BASESERVER_H

#include <boost/asio.hpp>
#include <socketowner.h>


using namespace boost::asio;

class MulticastReceiver
{
protected:
	MulticastReceiver(io_service& io_service,
		 ip::udp::socket& socket_,
		 const ip::address& multicast_address,
		 unsigned short port,
		 const ip::address& listen_address);

	void handle_receive_from(ip::udp::socket* socket_,
				 const boost::system::error_code& error,
				 size_t bytes_recvd);

	virtual void respond(ip::udp::endpoint sender,
			     std::vector<char>& data) = 0;

private:
	ip::udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
};

const ip::address default_listen_address(ip::address::from_string("0.0.0.0"));

class UDPSender
{
protected:
	UDPSender(const ip::address& multicast_address,
		  unsigned int port);

	void handle_send_to(const boost::system::error_code& error);

	void send_to(ip::udp::socket& socket_,
		     const std::vector<char>& data,
		     ip::udp::endpoint destination);

	int message_count_;
	std::string message_;
	ip::udp::endpoint endpoint_;
};

class BaseMulticastServer
	: public SocketOwner<ip::udp::socket>,
		public MulticastReceiver,
		public UDPSender
{
public:
	BaseMulticastServer(io_service &io_service,
			    const ip::address& multicast_address,
			    unsigned short port,
			    const ip::address& listen_address=default_listen_address);

};

#endif // BASESERVER_H
