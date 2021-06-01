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
#include "mensagem.h"
#include "dump.h"

using namespace std;
using namespace boost::asio;

class Orquestrador :
	public BaseMulticastServer
{
public:
	Orquestrador(io_service &io_service,
	       const ip::address& multicast_address,
	       unsigned short multicast_port)
	: BaseMulticastServer(io_service,
		     multicast_address,
		     multicast_port)
	, mc_endpoint(multicast_address, multicast_port)
	, exito_servidor(false)
	, exito_cliente(false)
	, desliga_servidor(false)
	, desliga_cliente(false)
	, ack_servidor(false)
	, ack_cliente(false)
	{
	}

	ip::udp::endpoint mc_endpoint;
	static const Origem origem_local = ORQUESTRADOR;

	bool exito_servidor;
	bool exito_cliente;
	bool desliga_servidor;
	bool desliga_cliente;
	bool ack_servidor;
	bool ack_cliente;

	void start(){
		exito_servidor = false;
		exito_cliente = false;
		ack_servidor = false;
		ack_cliente = false;
		Mensagem m(DISPARA, origem_local, 1, 1);
		send_to(socket_, Mensagem::pack(m), mc_endpoint);
	}

	virtual void respond(ip::udp::endpoint sender,
			     const vector<unsigned char>& data){

		ostringstream os;
#ifdef DEBUG_1
		os << "Message " << message_count_++ << " ";
		os << "recipient: " << origin << endl;
		message_ = os.str();
		cerr << message_;
		dump(data);
#endif

		Mensagem m(Mensagem::unpack(data));
		if (m.origem() == origem_local){
			// Ignora
		} else if (m.tipo() == ECO){
			cerr << "mensagem de eco recebido pelo multicast: ";
			cerr << m.tipo() << " ";
			cerr << m.origem() << " ";
			cerr << m.tamanho() << " ";
			cerr << m.repeticoes() << endl;
			dump(data);
		} else {
			trata_mensagem(m, origin);
		}
	}
	void trata_mensagem(const Mensagem& m,
			    ip::udp::endpoint origin) {
		switch (m.tipo()){
		case DISPARA:
		{
			switch (m.origem()){
			case SERVIDOR:
				cerr << "Servidor respondeu (ack): ";
				dump(m.pack());
				ack_servidor = true;
				break;
			case CLIENTE:
				cerr << "Cliente respondeu (ack): ";
				dump(m.pack());
				ack_cliente = true;
				break;
			default:
				cerr << "origem inválida" << endl;
				cerr << "Deveria ser inalcançável 🐛" << endl;
				break;
			}

			break;
		}
		case EXITO:
			switch (m.origem()){
			case SERVIDOR:
				cerr << "Servidor respondeu com êxito: ";
				dump(m.pack());
				exito_servidor = true;
				if (exito_servidor && exito_cliente){
					Mensagem m(DESLIGA, origem_local, 0x13, 0x37);
					send_to(socket_, Mensagem::pack(m), mc_endpoint);
				}
				break;
			case CLIENTE:
				cerr << "Cliente respondeu com êxito: ";
				dump(m.pack());
				exito_cliente = true;
				if (exito_servidor && exito_cliente){
					cerr << "todo mundo ja foi embora. Tchau!" << endl;
					socket_.get_io_service().stop();
				}
				break;
			default:
				cerr << "origem inválida" << endl;
				cerr << "Deveria ser inalcançável 🐛" << endl;
				break;
			}
			// TODO: Marcar um bit e dar erro caso nao seja marcado?
			break;
		case DESLIGA:
			switch (m.origem()){
			case SERVIDOR:
				cerr << "Servidor respondeu reconhecendo o desliga: ";
				dump(m.pack());
				desliga_servidor = true;
				if (desliga_servidor && desliga_cliente){
					cerr << "todo mundo ja foi embora. Tchau!" << endl;
					socket_.get_io_service().stop();
				}
				break;
			case CLIENTE:
				cerr << "Cliente respondeu reconhecendo o desliga: ";
				dump(m.pack());
				desliga_servidor = true;
				if (desliga_servidor && desliga_cliente){
					cerr << "todo mundo ja foi embora. Tchau!" << endl;
					socket_.get_io_service().stop();
				}
				break;
			default:
				cerr << "origem inválida" << endl;
				cerr << "Deveria ser inalcançável 🐛" << endl;
				break;
			}
			break;
		default:
			cerr << "Deveria ser inalcançável 🐛" << endl;
			break;
		}
	}

};


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 3)
		{
			cerr << "Usage: orquestrador <multicast_address> <multicast_port>\n";
			cerr << "       or\n";
			cerr << "       orquestrador <multicast_address> <multicast_port>\n";
			cerr << "  For IPv4, try:\n";
			cerr << "    orquestrador 239.255.0.1 9900 127.0.0.1 8800\n";
			cerr << "  For IPv6, try:\n";
			cerr << "    orquestrador ff31::8000:1234 9900 127.0.0.1 8800\n";
			return 1;
		}

		io_service io_service;
		Orquestrador orquestrador(io_service,
					  ip::address::from_string(argv[1]),
					  atoi(argv[2]));
		orquestrador.start();
		io_service.run();
	}
	catch (exception& e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

