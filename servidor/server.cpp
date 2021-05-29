
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

const short multicast_port = 30001;

const int max_message_count = 10;

using namespace boost::asio;

class Receiver
{
public:
	Receiver(io_service& io_service,
		 const ip::address& listen_address,
		 const ip::address& multicast_address)
		: sender_endpoint_(multicast_address, multicast_port)
		, socket_(io_service, sender_endpoint_.protocol())
	{
		// Create the socket so that multiple may be bound to the same address.
		ip::udp::endpoint listen_endpoint(multicast_address, multicast_port);
		socket_.open(listen_endpoint.protocol());
		socket_.set_option(ip::udp::socket::reuse_address(true));
		socket_.bind(listen_endpoint);

		// Join the multicast group.
		socket_.set_option( ip::multicast::join_group(multicast_address));

		socket_.async_receive_from( buffer(data_, max_length), sender_endpoint_,
					    boost::bind(&Receiver::handle_receive_from, this,
							placeholders::error,
							placeholders::bytes_transferred));
	}

	void handle_receive_from(const boost::system::error_code& error,
				 size_t bytes_recvd)
	{
		if (!error)
		{
			std::cout.write(data_, bytes_recvd);
			std::cout << std::endl;

			socket_.async_receive_from( buffer(data_, max_length), sender_endpoint_,
						    boost::bind(&Receiver::handle_receive_from, this,
								placeholders::error,
								placeholders::bytes_transferred));

			respond(sender_endpoint_);

			//      socket_.async_send_to(
			//          buffer(message_), sender_endpoint_,
			//          boost::bind(&receiver::handle_send_to, this,
			//            placeholders::error));
		}
	}

	virtual void respond(ip::udp::endpoint sender) = 0;

private:
	ip::udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
protected:
	ip::udp::socket socket_;


};



class Server
	: public Receiver
{
public:
	Server(io_service &io_service,
	       const ip::address& multicast_address,
	       const ip::address& listen_address)
	: Receiver(io_service, listen_address, multicast_address)
	, timer_(io_service)
	, message_count_(0)
	, endpoint_(multicast_address, multicast_port)
	{
	}

protected:
	virtual void respond(ip::udp::endpoint sender){


		std::ostringstream os;
		os << "Message " << message_count_++;
		message_ = os.str();

		socket_.async_send_to(
					buffer(message_), sender,
					boost::bind(&Server::handle_send_to, this,
						    placeholders::error));
	}

	void handle_send_to(const boost::system::error_code& error)
	{
		if (!error && message_count_ < max_message_count)
		{
			timer_.expires_from_now(boost::posix_time::seconds(1));
			timer_.async_wait( boost::bind(&Server::handle_timeout, this,
						       placeholders::error));
		}
	}


	void handle_timeout(const boost::system::error_code& error)
	{
		if (!error)
		{
			std::ostringstream os;
			os << "Message " << message_count_++;
			message_ = os.str();

			socket_.async_send_to( boost::asio::buffer(message_), endpoint_,
					       boost::bind(&Server::handle_send_to, this,
							   boost::asio::placeholders::error));
		}
	}

	deadline_timer timer_;
	int message_count_;
	std::string message_;
	ip::udp::endpoint endpoint_;
};


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 3)
		{
			std::cerr << "Usage: receiver <listen_address> <multicast_address>\n";
			std::cerr << "  For IPv4, try:\n";
			std::cerr << "    receiver 0.0.0.0 239.255.0.1\n";
			std::cerr << "  For IPv6, try:\n";
			std::cerr << "    receiver 0::0 ff31::8000:1234\n";
			return 1;
		}

		io_service io_service;
		Server server(io_service,
				ip::address::from_string(argv[1]),
				ip::address::from_string(argv[2]));
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
