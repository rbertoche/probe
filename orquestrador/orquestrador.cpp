//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "basemulticastserver.h"

using namespace std;
using namespace boost::asio;

class Servidor :
	public BaseMulticastServer
{
public:
	Servidor(io_service &io_service,
	       const ip::address& multicast_address,
	       unsigned short port)
	: BaseMulticastServer(io_service,
		     multicast_address,
		     port)
	{}

	void start(){
		vector<unsigned char> dummy(10);
		respond(endpoint_,
			dummy);

	}

	virtual void respond(ip::udp::endpoint sender,
			     const vector<unsigned char>& data){

		ostringstream os;
		os << "Message " << message_count_++ << ": ";

		for (vector<unsigned char>::const_iterator it = data.begin();
		     it < data.end();
		     it++){
			os << setw(2) << hex << unsigned(*it) << " ";
		}
		os << "FIM" << endl;
		os << "recipient: " << sender << endl;
		message_ = os.str();
		cerr << message_;

		send_to(socket_, data, sender);
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
			cerr << "    receiver 239.255.0.1 9900\n";
			cerr << "  For IPv6, try:\n";
			cerr << "    receiver ff31::8000:1234 9900\n";
			return 1;
		}

		io_service io_service;
		Servidor server(io_service,
				ip::address::from_string(argv[1]),
				atoi(argv[2]));
		server.start();
		io_service.run();
	}
	catch (exception& e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

