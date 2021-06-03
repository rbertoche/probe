//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2021 Raphael Bertoche (rbertoche@cpti.cetuc.puc-rio.br)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <strstream>
#include <iomanip>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "basemulticastserver.h"
#include "mensagem.h"
#include "dump.h"
#include "rtclock.h"

using namespace std;
using namespace boost::asio;

//#define DEBUG_1
//#define DEBUG

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
		, io_service(io_service)
		, endpoint(server_address, server_port)
	{
		rtt_file.open("rtt_file", ios::out | ios::app);
	}

	boost::asio::io_service& io_service;
	ip::tcp::endpoint endpoint;

	static const Origem origem_local = CLIENTE;
	ofstream rtt_file;


	virtual void respond(ip::udp::endpoint /*origin*/,
			     const vector<unsigned char>& data){
		ostringstream os;
#ifdef DEBUG_1
		os << "Message " << message_count_++ << " ";
//		os << "recipient: " << origin << endl;
		message_ = os.str();
		cerr << message_;
		dump(data);
#endif // DEBUG_1

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

	double time_0;

	void trata_mensagem(const Mensagem& m,
			    ip::udp::endpoint origin) {
		switch (m.tipo()){
		case DISPARA:
		{
			// ack
			Mensagem ack(DISPARA, origem_local, m.tamanho(), m.repeticoes());
			send_to(socket_, Mensagem::pack(ack), origin);
			// Executa teste
			time_0 = rt_clock();
			int ret = run_test(m);
			double rtt = rt_clock() - time_0;
			// Notifica fim do teste
			rtt_file << setw(2) << setprecision(5);
			rtt_file << long(time_0) << "\t"
				 << m.tamanho() << "\t"
				 << m.repeticoes() << "\t"
				 << rtt << endl;
			rtt_file.flush();
			Mensagem exito(EXITO, origem_local, m.tamanho(), ret);
			send_to(socket_, Mensagem::pack(exito), origin);
			break;
		}
		case EXITO:
			cerr << "ack do orquestrador recebido" << endl;
			break;
		case DESLIGA:
		{
			Mensagem ack(DESLIGA, origem_local, m.tamanho(), m.repeticoes());
			send_to(socket_, Mensagem::pack(ack), origin);
			cerr << "mensagem de shutdown recebida. Tchau!" << endl;
			socket_.get_io_service().stop();
			break;
		}
		default:
			cerr << "Deveria ser inalcançável" << endl;
			break;
		}
	}

	int run_test(const Mensagem& m){
		if (m.tamanho() < 4){
			cerr << "Erro, disparo de teste com tamanho inválido "
			      << m.tamanho() << endl;
			return -1;
		}
		cerr << "inciando teste" << endl;
		vector<unsigned char> header(4);
		vector<unsigned char> conteudo(m.tamanho() - 4);

		ip::tcp::socket tcp_socket(io_service);
		try {
//			cout << 1 << " " << rt_clock() - time_0 << endl;
			tcp_socket.connect(endpoint);
		} catch (boost::system::system_error const& err){
			cerr << "Error on connect: ";
			cerr << err.what() << endl;
			abort();
		}

		cerr << "Conectando em " << endpoint << endl;
		for (unsigned int i=0; i < m.repeticoes(); i++)
		{
			Mensagem m_test(ECO, origem_local, m.tamanho(), m.repeticoes());
			header = Mensagem::pack(m_test);
			// Timing!
#ifdef DEBUG_1
			cerr << "enviando mensagem de teste via tcp:\t";
			dump(header);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "enviando mensagem de teste via tcp" << endl;
#endif // DEBUG
#endif // DEBUG_1
			try {
//				cout << 2 << " " << rt_clock() - time_0 << endl;
				tcp_socket.send(buffer(header));
//				cout << 3 << " " << rt_clock() - time_0 << endl;
			} catch (boost::system::system_error const& err){
				cerr << "Error on receive: ";
				cerr << err.what() << endl;
				return -2;
			}
			if (conteudo.size()){ // Trata teste com 4 bytes (sem conteudo)
//				cout << 4 << " " << rt_clock() - time_0 << endl;
				tcp_socket.send(buffer(conteudo));
//				cout << 5 << " " << rt_clock() - time_0 << endl;
#ifdef DEBUG_1
				dump(conteudo);
#endif // DEBUG_1
			}
#ifdef DEBUG
			cerr << "leitura bloqueante..." << endl;
#endif // DEBUG
			size_t read_bytes;
			try {
//				cout << 6 << " " << rt_clock() - time_0 << endl;
				read_bytes = tcp_socket.receive(buffer(header));
//				cout << 7 << " " << rt_clock() - time_0 << endl;
			} catch (boost::system::system_error const& err){
				cerr << "Error on receive: ";
				cerr << err.what() << endl;
				return -3;
			}
			if (read_bytes != 4){
				cerr << "Não foi possível ler a mensagem.";
				abort();
			}
#ifdef DEBUG_1
			cerr << "mensagem recebida via tcp: ";
			dump(header);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "mensagem recebida via tcp" << endl;
#endif // DEBUG
#endif // DEBUG_1
			Mensagem m_resposta(Mensagem::unpack(header));
			if (m.tamanho() != m_resposta.tamanho()){
				cerr << "Erro, recebi mensagem de tamanho incorreto. ";
				cerr.flush();
				return -4;
			} else if (m_resposta.tipo() != ECO){
				cerr << "Erro, recebi mensagem de tipo "
				     << m_resposta.tipo() << " incorreto. ";
				cerr.flush();
				return -5;
			} else {
				if (conteudo.size()){ // Trata teste com 4 bytes (sem conteudo)
					size_t read_bytes = 0;

//					cout << 8 << " " << rt_clock() - time_0 << endl;
					while (read_bytes < m.tamanho() - header.size()){
						read_bytes += tcp_socket.receive(buffer(conteudo));
					}
//					cout << 9 << " " << rt_clock() - time_0 << endl;
					if (read_bytes != (m.tamanho() - header.size())){
						cerr << "Erro, fim prematuro da mensagem.";
						cerr.flush();
						return -3;
					} else {
#ifdef DEBUG
						cerr << "eco ok." << endl;
#endif // DEBUG
					}
#ifdef DEBUG_1
					dump(conteudo);
#endif // DEBUG_1
				}
				// Timing!
			}
		}
//		cout << 10 << " " << rt_clock() - time_0 << endl;
		cerr << "Teste completado com sucesso." << endl;
		return 0;
	}
};


int main(int argc, char* argv[])
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

	return 0;
}

