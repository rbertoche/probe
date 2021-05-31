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
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "baseserver.h"

using namespace std;

class Server :
	public BaseServer
{
public:
	Server(io_service &io_service,
	       const ip::address& multicast_address,
	       const ip::address& listen_address)
	: BaseServer(io_service,
		     multicast_address,
		     listen_address)
	{}

	virtual void respond(ip::udp::endpoint recipient,
			     vector<char>& data){

		ostringstream os;
		os << "Message " << message_count_++ << ": ";

		for (vector<char>::iterator it = data.begin();
		     it < data.end();
		     it++){
			os << setw(2) << hex << unsigned(*it) << " ";
		}
		os << "FIM" << endl;
		os << "recipient: " << recipient << endl;
		message_ = os.str();
		cerr << message_;

		socket_.async_send_to(buffer(data), recipient,
				      boost::bind(&Server::handle_send_to,
						  static_cast<BaseServer*>(this),
						  placeholders::error));
	}
};


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 3)
		{
			cerr << "Usage: receiver <listen_address> <multicast_address>\n";
			cerr << "  For IPv4, try:\n";
			cerr << "    receiver 0.0.0.0 239.255.0.1\n";
			cerr << "  For IPv6, try:\n";
			cerr << "    receiver 0::0 ff31::8000:1234\n";
			return 1;
		}

		io_service io_service;
		Server server(io_service,
				ip::address::from_string(argv[1]),
				ip::address::from_string(argv[2]));
		vector<char> empty(10);
		server.respond(server.getEndpoint(),
			       empty);
		io_service.run();
	}
	catch (exception& e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

