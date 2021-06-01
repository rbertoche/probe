//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <strstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "basemulticastserver.h"
#include "mensagem.h"
#include "dump.h"

using namespace std;
using namespace boost::asio;

class Cliente :
	public BaseMulticastServer
{
public:
	Cliente(boost::asio::io_service& io_service,
		const ip::address& multicast_address,
		unsigned int multicast_port,
		const ip::address& server_address,
		unsigned int server_port)
		: BaseMulticastServer(io_service,
				      multicast_address,
				      multicast_port)
		, endpoint(server_address, server_port)
	{
	}

	ip::tcp::endpoint endpoint;
	ip::tcp::acceptor acceptor;

	static const Origem origem_local = CLIENTE;

	virtual void respond(ip::udp::endpoint origin,
			     const vector<unsigned char>& data){
		ostringstream os;

		os << "Message " << message_count_++ << " ";
		os << "recipient: " << origin << endl;
		message_ = os.str();
		cerr << message_;
		dump(data);

		Mensagem m(Mensagem::unpack(data));
		if (m.origem() != ORQUESTRADOR){
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
			// ack
			Mensagem m(DISPARA, origem_local, 0, 0);
			send_to(socket_, Mensagem::pack(m), origin);
			// Executa teste
			run_test(m);
			{ // Notifica fim do teste
			Mensagem m(EXITO, origem_local, 0, 0);
			send_to(socket_, Mensagem::pack(m), origin);
			}
			break;
		}
		case EXITO:
			cerr << "ack do orquestrador recebido" << endl;
			break;
		case DESLIGA:
			cerr << "mensagem de shutdown recebida. Tchau!" << endl;
			socket_.get_io_service().stop();
			break;
		default:
			cerr << "Deveria ser inalcançável" << endl;
			break;
		}
	}

	void run_test(const Mensagem& m){
		vector<unsigned char> header(4);
		istrstream header_istream(reinterpret_cast<char*>(header.data()),
								 header.size());
		ostrstream header_ostream(reinterpret_cast<char*>(header.data()),
								 header.size());

		vector<unsigned char> conteudo(m.tamanho() - 4);
		istrstream conteudo_istream(reinterpret_cast<char*>(conteudo.data()),
								   conteudo.size());
		ostrstream conteudo_ostream(reinterpret_cast<char*>(conteudo.data()),
								   conteudo.size());

		ip::tcp::iostream tcp_stream(endpoint);
		for (unsigned int i=0; i < m.repeticoes(); i++)
		{
			{
			Mensagem m_test(ECO, origem_local, m.tamanho(), m.repeticoes());
			header = Mensagem::pack(m_test);
			// Timing!
			tcp_stream << header_istream;
			tcp_stream << conteudo_istream;
			}
			header_ostream << tcp_stream;
			Mensagem m_resposta(Mensagem::unpack(header));
			if (m.tamanho() != m_resposta.tamanho()){
				cerr << "Erro, recebi mensagem de tamanho incorreto. ";
				cerr << "Abortando!" << endl;
				cerr.flush();
				abort();
			}
			tcp_stream >> conteudo_ostream;
			// Timing!
#ifdef DEBUG
			cerr << "mensagem de teste recebida: ";
			dump(conteudo);
#else
			cerr << "mensagem de teste recebida" << endl;
#endif
		}
	}
};


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 5)
		{
			cerr << "Usage: cliente <multicast_address> <multicast_port>\n";
			cerr << "               <server_address> <server_port>\n";
			cerr << "  For IPv4, try:\n";
			cerr << "    cliente 239.255.0.1 9900 127.0.0.1 8800\n";
			cerr << "  For IPv6, try:\n";
			cerr << "    cliente ff31::8000:1234 9900 127.0.0.1 8800\n";
			return 1;
		}

		io_service io_service;
		Cliente cliente(io_service,
				ip::address::from_string(argv[1]),
				atoi(argv[2]),
				ip::address::from_string(argv[3]),
				atoi(argv[4]));
		io_service.run();
	}
	catch (exception& e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

