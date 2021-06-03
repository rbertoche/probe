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
using boost::asio::ip::tcp;

//#define DEBUG_1

class Servidor :
	public BaseMulticastServer
{
public:
	Servidor(boost::asio::io_service& io_service_,
		 const ip::address& multicast_address,
		 unsigned int multicast_port,
		 const ip::address& server_address,
		 unsigned int server_port)
		: BaseMulticastServer(io_service_,
				      multicast_address,
				      multicast_port)
		, io_service(io_service_)
		, endpoint(server_address, server_port)
		, acceptor(io_service, endpoint)
	{
		cerr << "Ouvindo em " << endpoint << endl;
	}

	Servidor(boost::asio::io_service& io_service_,
		 const ip::address& multicast_address,
		 unsigned int multicast_port,
		 unsigned int server_port)
		: BaseMulticastServer(io_service_,
				      multicast_address,
				      multicast_port)
		, io_service(io_service_)
		, endpoint(ip::tcp::v4(), server_port)
		, acceptor(io_service, endpoint)
	{
		cerr << "Ouvindo em " << endpoint << endl;
	}

	boost::asio::io_service& io_service;
	ip::tcp::endpoint endpoint;
	ip::tcp::acceptor acceptor;

	static const Origem origem_local = SERVIDOR;

	virtual void respond(ip::udp::endpoint origin,
			     const vector<unsigned char>& data){
		// TODO: Resposta está indo via unicast... ops!
		ostringstream os;
#ifdef DEBUG_1
		os << "Message " << message_count_++ << " ";
		os << "recipient: " << origin << endl;
		message_ = os.str();
		cerr << message_;
		dump(data);
#endif

		Mensagem m(Mensagem::unpack(data));
		if (m.origem() != ORQUESTRADOR){
			// Ignora
		} else if (m.tipo() == ECO){
			cerr << "Erro, mensagem de eco recebido pelo multicast: ";
			cerr << m.tipo() << " ";
			cerr << m.origem() << " ";
			cerr << m.tamanho() << " ";
			cerr << m.repeticoes() << endl;
			dump(data);
			abort();
		} else {
			trata_mensagem(m, mc_endpoint);
		}
	}

	void trata_mensagem(const Mensagem& m,
			    ip::udp::endpoint origin) {
		switch (m.tipo()){
		case DISPARA:
		{
			// ack
			Mensagem ack(DISPARA, origem_local, m.tamanho(), m.repeticoes());
			send_to(socket_, Mensagem::pack(ack), origin);
			// Executa teste
			int ret = run_test(m);
			// Notifica fim do teste
			Mensagem exito(EXITO, origem_local, m.tamanho(), ret);
			send_to(socket_, Mensagem::pack(exito), origin);
			break;
		}
		case EXITO:
			cerr << "ack do orquestrador recebido" << endl;
			// TODO: Marcar um bit e dar erro caso nao seja marcado?
			break;
		case DESLIGA:
		{
			Mensagem ack(DESLIGA, origem_local, m.tamanho(), m.repeticoes());
			vector<unsigned char> v = Mensagem::pack(ack);
			*reinterpret_cast<unsigned short*>(&v[2]) = 0x3713;
			send_to(socket_, v, origin);
			cerr << "mensagem de shutdown recebida. Tchau!" << endl;
			socket_.get_io_service().stop();
			break;
		}
		default:
			cerr << "tipo inválido" << endl;
			cerr << "Deveria ser inalcançável 🐛" << endl;
			break;
		}
	}

	int run_test(const Mensagem& m){
		if (m.tamanho() < 4){
			cerr << "Erro, disparo de teste com tamanho inválido "
			      << m.tamanho() << endl;
//			cerr << "Erro tentando desempacotar mensagem "
//				"de tamanho inválido "
//			      << m.tamanho() << endl;
			return -1;
		}

		vector<unsigned char> header(4);
		vector<unsigned char> conteudo(m.tamanho() - 4);

		ip::tcp::socket tcp_socket(io_service);
		acceptor.accept(tcp_socket);
		cerr << "Conexão aceita." << endl;

		for (unsigned int i=0; i < m.repeticoes(); i++)
		{
			size_t read_bytes;
			try {
				 read_bytes = tcp_socket.receive(buffer(header));
			} catch (boost::system::system_error& e){
				cerr << "Erro em receive: ";
				cerr << e.what() << endl;
				return -2;
			}
			if (read_bytes != 4){
				cerr << "Não foi possível ler a mensagem.";
				abort();
			}
#ifdef DEBUG_1
			cerr << "mensagem de teste recebida: ";
			dump(header);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "mensagem de teste recebida" << endl;
#endif // DEBUG
#endif // DEBUG_1
			Mensagem m_test(Mensagem::unpack(header));
#ifndef NO_TESTS
			if (m.tamanho() != m_test.tamanho()){
				cerr << "Erro, recebi mensagem de tamanho incorreto. ";
				cerr.flush();
				return -3;
			}
#endif // NO_TESTS
			if (m.tamanho() - header.size() > 0){
				size_t read_bytes = 0;
				try {
					while (read_bytes < m.tamanho() - header.size()){
						read_bytes += tcp_socket.receive(buffer(conteudo));
					}
				} catch (boost::system::system_error& e){
					cerr << e.what() << endl;
					cerr << e.code() << endl;
					cerr << e.code().message() << endl;
					return -4;
				}
#ifdef NO_TESTS
				if (read_bytes != (m.tamanho() - header.size())){
					cerr << "Erro, fim prematuro da mensagem: ";
					cerr << read_bytes << endl;
					cerr.flush();
					return -5;
				}
#endif // NO_TESTS
#ifdef DEBUG
				dump(conteudo);
#endif // DEBUG
			}
#ifdef DEBUG_1
			cerr << "mensagem de teste recebida: ";
			dump(conteudo);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "mensagem de teste recebida" << endl;
#endif // DEBUG
#endif // DEBUG_1
			Mensagem m_resposta(ECO, origem_local, m.tamanho(), m.repeticoes());
			header = Mensagem::pack(m_resposta);
#ifdef DEBUG_1
			cerr << "enviando resposta a mensagem de teste via tcp: ";
			dump(header);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "enviando resposta a mensagem de teste via tcp" << endl;
#endif // DEBUG
#endif // DEBUG_1
			tcp_socket.send(buffer(header));
			if (conteudo.size()){ // Trata teste com 4 bytes (sem conteudo)
#ifdef DEBUG
					dump(conteudo);
#endif // DEBUG
				tcp_socket.send(buffer(conteudo));
			}
		}
		cerr << "Teste completado" << endl;
		return 0;
	}
};


int main(int argc, char* argv[])
{
	io_service io_service;
//		tcp::endpoint endpoint(tcp::v4(), 13);
//		tcp::acceptor acceptor(io_service, endpoint);

	if  (argc == 4) {
		Servidor servidor(io_service,
				  ip::address::from_string(argv[1]),
				  atoi(argv[2]),
				  atoi(argv[3]));
		io_service.run();
	} else if (argc == 5) {
		Servidor servidor(io_service,
				  ip::address::from_string(argv[1]),
				  atoi(argv[2]),
				  ip::address::from_string(argv[3]),
				  atoi(argv[4]));
		io_service.run();
	} else {
		cerr << "Usage: servidor <multicast_address> <multicast_port>\n";
		cerr << "                <server_address> <server_port>\n";
		cerr << "       or\n";
		cerr << "       servidor <multicast_address> <multicast_port>\n";
		cerr << "                <server_port>\n";
		cerr << "  For IPv4, try:\n";
		cerr << "    servidor 239.255.0.1 9900 127.0.0.1 8800\n";
		cerr << "    or\n";
		cerr << "    servidor 239.255.0.1 9900 8800\n";
		cerr << "  For IPv6, try:\n";
		cerr << "    servidor ff31::8000:1234 9900 127.0.0.1 8800\n";
		cerr << "    or";
		cerr << "    servidor ff31::8000:1234 9900 8800\n";
		return 1;
	}

	return 0;
}

