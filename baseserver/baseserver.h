//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BASESERVER_H
#define BASESERVER_H


#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>


const short multicast_port = 30001;

const int max_message_count = 10;

using namespace boost::asio;


class Receiver
{
protected:
	Receiver(io_service& io_service,
		 const ip::address& multicast_address,
		 const ip::address& listen_address);

	void handle_receive_from(const boost::system::error_code& error,
				 size_t bytes_recvd);

	virtual void respond(ip::udp::endpoint sender) = 0;

private:
	ip::udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
protected:
	ip::udp::socket socket_;


};

const ip::address unspecified(ip::address::from_string("255.255.255.255"));



class BaseServer
	: public Receiver
{
protected:
	BaseServer(io_service &io_service,
		   const ip::address& multicast_address,
		   const ip::address& listen_address=unspecified)
	: Receiver(io_service,
		   multicast_address,
		   listen_address)
	, timer_(io_service)
	, message_count_(0)
	, endpoint_(multicast_address, multicast_port)
	{
	}

	virtual void respond(ip::udp::endpoint sender) = 0;

	void handle_send_to(const boost::system::error_code& error);


	void handle_timeout(const boost::system::error_code& error);

	deadline_timer timer_;
	int message_count_;
	std::string message_;
	ip::udp::endpoint endpoint_;
};
#endif // BASESERVER_H
