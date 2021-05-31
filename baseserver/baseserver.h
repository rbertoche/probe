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


using namespace boost::asio;


class SocketInterface{
	// Permite acessar o socket fora do construtor
	// O nome socket_ visivel externamente nao está
	// acessível aqui, portanto é necessário esse método
	// Dentro do construtor virtual nao funciona
protected:
	virtual ip::udp::socket& socket() = 0;
};


class Receiver
	: public SocketInterface
{
protected:
	Receiver(io_service& io_service,
		 ip::udp::socket& socket_,
		 const ip::address& multicast_address,
		 const ip::address& listen_address);

	void handle_receive_from(const boost::system::error_code& error,
				 size_t bytes_recvd);

	virtual void respond(ip::udp::endpoint sender,
			     std::vector<char>& data) = 0;

private:
	ip::udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
};

const ip::address default_listen_address(ip::address::from_string("0.0.0.0"));

class Sender
	: public SocketInterface
{
public:

	ip::udp::endpoint getEndpoint();

protected:
	Sender(io_service &io_service,
	       const ip::address& multicast_address);

	virtual void respond(ip::udp::endpoint sender,
			     std::vector<char>& data) = 0;

	void handle_send_to(const boost::system::error_code& error);

	void handle_timeout(const boost::system::error_code& error);

	deadline_timer timer_;
	int message_count_;
	std::string message_;
	ip::udp::endpoint endpoint_;
};
#endif // BASESERVER_H

class SocketOwner // cada objeto deve possuir no máximo 1 recurso?
		  // na verdade esse objeto existe para prover um
		  // socket que permitiu desacoplar as outras classes
{
private:
	ip::udp::socket socket__;
protected:
	SocketOwner(io_service &io_service)
		: socket__(io_service)
		, socket_(socket__)
	{}
	ip::udp::socket& socket_;
};


class BaseServer
	: public SocketOwner,
		public Receiver,
		public Sender
{
public:
	BaseServer(io_service &io_service,
		   const ip::address& multicast_address,
		   const ip::address& listen_address=default_listen_address);

protected:
	virtual ip::udp::socket& socket();

};
