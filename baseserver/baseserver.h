//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BASESERVER_H
#define BASESERVER_H

#include <boost/asio.hpp>


using namespace boost::asio;

class UDPSocketInterface
{
	// Permite acessar o socket fora do construtor
	// Dentro do construtor virtual nao funciona
protected:
	virtual ip::udp::socket& udp_socket() = 0;
};

class MulticastReceiver
	: public UDPSocketInterface
{
protected:
	MulticastReceiver(io_service& io_service,
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

class UDPSender
	: public UDPSocketInterface
{
public:

	ip::udp::endpoint getEndpoint()
	{
		return endpoint_;
	}

protected:
	UDPSender(io_service& io_service,
		       const ip::address& multicast_address);

	virtual void respond(ip::udp::endpoint origin,
			     std::vector<char>& data) = 0;

	void handle_send_to(const boost::system::error_code& error);

	void handle_timeout(const boost::system::error_code& error);

	deadline_timer timer_;
	int message_count_;
	std::string message_;
	ip::udp::endpoint endpoint_;

	virtual ip::udp::socket& udp_socket() = 0;
};
#endif // BASESERVER_H

template <typename Socket>
class SocketOwner // cada objeto deve possuir no máximo 1 recurso?
		  // na verdade esse objeto existe para prover um
		  // socket que permitiu desacoplar as outras classes
{
private:
	Socket socket__;
protected:
	SocketOwner(io_service &io_service)
		: socket__(io_service)
		, socket_(socket__)
	{}
	Socket& socket_;
};

class BaseMulticastServer
	: public SocketOwner<ip::udp::socket>,
		public MulticastReceiver,
		public UDPSender
{
public:
	BaseMulticastServer(io_service &io_service,
			    const ip::address& multicast_address,
			    const ip::address& listen_address=default_listen_address);

protected:
	virtual ip::udp::socket& udp_socket();

};
