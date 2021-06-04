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
#include <numeric>
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
	vector<double> rtt;


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
			int ret = run_test(m);
			double average = std::accumulate(rtt.begin(), rtt.end(), 0.0) / rtt.size();
			double variance = 0;
			for (vector<double>::const_iterator it = rtt.begin();
			     it < rtt.end();
			     it++){
				variance += pow(*it - average, 2);
			}
			double st_dev = sqrt(variance / m.repeticoes());

			rtt_file << fixed << showpoint
				 << long(time_0) << " "
				 << setw(20) << endpoint << " "
				 << setw(10) << m.tamanho() << " "
				 << setw(8) << m.repeticoes() << " "
				 << setw(8) << average << " "
				 << setw(8) << st_dev << endl;
			rtt_file.flush();
			// Notifica fim do teste
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
		vector<unsigned char> buffer_(m.tamanho());
		rtt.resize(m.repeticoes());
#ifdef CONTEUDO_NAO_NULO
		for (vector<unsigned char>::iterator it= buffer_.begin();
		     it < buffer_.end();
		     it++){
			*it = it - buffer_.begin();
		}
#endif // CONTEUDO_NAO_NULO
		ip::tcp::socket tcp_socket(io_service);
		try {
//			cout << 1 << " " << rt_clock() - time_0 << endl;
			tcp_socket.connect(endpoint);
		} catch (boost::system::system_error const& err){
			cerr << "Error on connect: ";
			cerr << err.what() << endl;
			abort();
		}

		double i_time = 0;
		cerr << "Conectado em " << endpoint << endl;
		for (unsigned int i=0; i < m.repeticoes(); i++)
		{
			double t = rt_clock();
			if (t - i_time > 0.5){
				cerr << "Iniciando repetição " << i + 1 << " / "
				     << m.repeticoes() << endl;
				i_time = t;
			}
			Mensagem m_test(ECO, origem_local, m.tamanho(), m.repeticoes());
			Mensagem::pack(buffer_, m_test);
			// Timing!

#ifdef DEBUG_1
			cerr << "enviando resposta a mensagem de teste via tcp: ";
			dump(buffer_);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "enviando resposta a mensagem de teste via tcp" << endl;
#endif // DEBUG
#endif // DEBUG_1
			const unsigned content_1 = 5, content_2 = buffer_.size() - 1;
			buffer_[content_1] = content_1;
			buffer_[content_2] = content_2;


			time_0 = rt_clock();
			size_t sent_bytes = 0;
			try {
				sent_bytes = write(tcp_socket, buffer(buffer_));
			} catch (boost::system::system_error& e){
				cerr << "Error on send: ";
				cerr << e.what() << endl;
				return -3;
			}

			double rtt_sum = rt_clock() - time_0;
#ifdef DEBUG_1
			cerr << "enviado mensagem de teste via tcp:\t";
			dump(buffer_);
#else // DEBUG_1
#ifdef DEBUG
			cerr << "enviado mensagem de teste via tcp" << endl;
#endif // DEBUG
#endif // DEBUG_1
			t = rt_clock();
			size_t read_bytes = 0;
			try {
				read_bytes = read(tcp_socket, buffer(buffer_));
			} catch (boost::system::system_error& e){
				cerr << "Error on receive: ";
				cerr << e.what() << endl;
				return -3;
			}
			rtt_sum += rt_clock() - time_0;
			rtt[i] = rtt_sum;

#ifdef DEBUG
			cerr << hex << unsigned(buffer_[5]) << " "
			     << unsigned(buffer_[buffer_.size() - 1]) << endl;
			cerr << dec;
#endif // DEBUG
			Mensagem m_resposta(Mensagem::unpack(buffer_));
			if (m.tamanho() != m_resposta.tamanho()){
				cerr << "Erro, recebi mensagem de tamanho incorreto. " << endl;
				cerr.flush();
				return -5;
			} else if (read_bytes != m.tamanho()){
				cerr << "Erro, fim prematuro da mensagem: ";
				cerr << read_bytes << endl;
				cerr.flush();
				return -6;
			} else if (m_resposta.tipo() != ECO){
				cerr << "Erro, recebi mensagem de tipo "
				     << m_resposta.tipo() << " incorreto. " << endl;
				cerr.flush();
				return -7;
			} else if(buffer_[content_1] != content_1 ||
					buffer_[content_2] != uint8_t(content_2))
			{
				cerr << "Erro, conteúdo não preservado. " << endl;
				cerr.flush();
				return -5;
			}
				// Timing!
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

