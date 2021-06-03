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
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "basemulticastserver.h"
#include "mensagem.h"
#include "cli.h"
#include "dump.h"

using namespace std;
using namespace boost::asio;


class Orquestrador :
	public BaseMulticastServer
{
public:
	Orquestrador(io_service &io_service,
	       const ip::address& multicast_address,
	       unsigned short multicast_port,
	       boost::mutex* mutex,
	       boost::condition_variable* cond)
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
	, teste_acontecendo(false)
	, teste_falhou(false)
	, mutex(mutex)
	, cond(cond)
	, io_service_(io_service)
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
	bool teste_acontecendo;
	bool teste_falhou;

	boost::mutex* mutex;
	boost::condition_variable* cond;

	io_service& io_service_;

	~Orquestrador(){
		boost::lock_guard<boost::mutex> guard(*mutex);
		if (teste_acontecendo){
			teste_acontecendo = false;
			cond->notify_one();
		}
	}

	void close(){
		io_service_.stop();
	}

	void dispara(uint16_t tamanho, uint16_t repeticoes){
		exito_servidor = false;
		exito_cliente = false;
		ack_servidor = false;
		ack_cliente = false;
		Mensagem m(DISPARA, origem_local, tamanho, repeticoes);
		vector<unsigned char> data = Mensagem::pack(m);
		teste_acontecendo = true;

		Mensagem m2 = Mensagem::unpack(Mensagem::pack(m));
		cerr << "Disparando teste: " << m2.tamanho() << " " << m2.repeticoes();
		dump(data);

		send_to(socket_, Mensagem::pack(m), mc_endpoint);
	}

	void desliga(){
		Mensagem m(DESLIGA, origem_local, 0x13, 0x37);
		send_to(socket_, Mensagem::pack(m), mc_endpoint);
	}

	void fim_do_teste(){
		if (!teste_falhou){
			cerr << "Teste terminado com sucesso" << endl;
		} else {
			cerr << "Teste terminado devido à falha" << endl;
		}
		boost::lock_guard<boost::mutex> guard(*mutex);
		teste_acontecendo = false;
		cond->notify_one();
	}


	virtual void respond(ip::udp::endpoint origin,
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
			trata_mensagem(m, mc_endpoint);
		}
	}
	void trata_mensagem(const Mensagem& m,
			    ip::udp::endpoint origin) {
		switch (m.tipo()){
		case DISPARA:
		{
			switch (m.origem()){
			case SERVIDOR:
#ifdef DEBUG
				cerr << "Servidor respondeu (ack): ";
				dump(Mensagem::pack(m));
#endif
				ack_servidor = true;
				break;
			case CLIENTE:
#ifdef DEBUG
				cerr << "Cliente respondeu (ack): ";
				dump(Mensagem::pack(m));
#endif
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
			if (m.repeticoes() != 0 && teste_acontecendo){
				switch (m.origem()){
				case SERVIDOR:
					cerr << "Servidor respondeu notificando falha no teste: ";
					exito_servidor = true;
					break;
				case CLIENTE:
					cerr << "Servidor respondeu notificando falha no teste: ";
					exito_cliente = true;
					break;
				default:
					cerr << "Deveria ser inalcaçável" << endl;
					cerr.flush();
					abort();
					break;
				}
				dump(Mensagem::pack(m));
				teste_falhou = true;
				if (exito_servidor && exito_cliente){
					fim_do_teste();
				}
				break;
			}
			switch (m.origem()){
			case SERVIDOR:
				cerr << "Servidor respondeu com êxito: ";
				dump(Mensagem::pack(m));
				exito_servidor = true;
				if (exito_servidor && exito_cliente){
					fim_do_teste();
				}
				break;
			case CLIENTE:
				cerr << "Cliente respondeu com êxito: ";
				dump(Mensagem::pack(m));
				exito_cliente = true;
				if (exito_servidor && exito_cliente){
					fim_do_teste();
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
				dump(Mensagem::pack(m));
				desliga_servidor = true;
				if (desliga_servidor && desliga_cliente){
					cerr << "todo mundo ja foi embora. Tchau!" << endl;
					close();
				}
				break;
			case CLIENTE:
				cerr << "Cliente respondeu reconhecendo o desliga: ";
				dump(Mensagem::pack(m));
				desliga_cliente = true;
				if (desliga_servidor && desliga_cliente){
					cerr << "todo mundo ja foi embora. Tchau!" << endl;
					close();
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
//        boost::lock_guard<boost::mutex> lock(mut);

struct CLIData {
	Orquestrador* orquestrador;
	boost::mutex* mutex;
	boost::condition_variable* cond;
};

void int_handler_ignore(int s){

}

int yes_no(const char* a){
	if (strcmp(a, "sim") == 0
	    || strcmp(a, "s") == 0
	    || strcmp(a, "yes") == 0
	    || strcmp(a, "y") == 0 ){
		return 0;

	} else if (strcmp(a, "nao") == 0
		   || strcmp(a, "n") == 0
		   || strcmp(a, "no") == 0 ){
		return 1;
	}
	return -1;
}

static cli_parser_state* _int_parser_state = NULL;
static Orquestrador* _int_orquestrador = NULL;
//static boost::mutex* _mutex_for_int = NULL;
//static boost::condition_variable* _cond_for_int = NULL;
//static bool* _teste_acontecendo = NULL;
static bool _int_happened = false;

void int_handler_ask(int s){
	vector<char> buf(1024);
	cout << "Temos um teste em execução." << endl;
	cout << "Tem certeza que deseja fechar o orquestrador?";
	cout << "[sim/nao] (nao): ";
	cin.getline(buf.data(), buf.size());
//	cout.flush();
	if (yes_no(buf.data()) == 0){
		signal(SIGINT, SIG_DFL);
		*_int_parser_state = STATE_STOP;
		_int_orquestrador->close();
		_int_happened = true;
	}
}

int cli_exec_command(void *data_, int argc, const char **argv){
	CLIData* data = reinterpret_cast<CLIData*>(data_);

	if (strcmp(argv[0], "dispara") == 0){
		if( check_arg_count(argc, argv, 2) == 0) {
			int tamanho = atoi(argv[1]);
			if (tamanho < 4 || tamanho > 1 << 20){
				cerr << "Tamanho " << tamanho
				     << "inválido, entre um valor entre 4 e 65535." << endl;
				return 0;
			}
			int repeticoes = atoi(argv[2]);
			if (repeticoes < 1 || repeticoes > 1 << 16){
				cerr << "Número de repetições " << repeticoes
				     << "fora do escopo, entre um valor entre 1 e 65535." << endl;
				return 0;
			}
			signal(SIGINT, int_handler_ask);
			data->orquestrador->dispara(tamanho, repeticoes);
			boost::unique_lock<boost::mutex> lock(*data->mutex);
			while (data->orquestrador->teste_acontecendo){
				data->cond->wait(lock);
			}
			signal(SIGINT, int_handler_ignore);

		}
	} else if (strcmp(argv[0], "desliga") == 0){
		data->orquestrador->desliga();
	} else if (strcmp(argv[0], "exit") == 0) {
		return RET_QUIT;
	} else if (strcmp(argv[0], "help") == 0) {
		const char msg[] =
				"Comandos disponíveis:\n"
				"dispara\t\tRecebe como parâmetro o tamanho do pacote (em bytes) e \n"
				"\t\to numero de repetições e inicia um teste\n"
				"\t\tUsa a potência de 2 mais próxima do valor escolhido: round(log2(n))\n"
				"desliga\t\tEnvia mensagem para terminar os outros hosts e em seguida\n"
				"\t\tse desliga também\n"
				"exit\t\tTermina apenas o orquestrador\n";
		fputs(msg, stderr);
	} else if (argv[0][0] == 0){
	} else {
		fprintf(stderr, "Comando \"%s\" desconhecido, use o comando help para conhecer\n"
				"os comandos disponíveis.\n", argv[0]);
	}
	return 0;

}

void cli_handler(Orquestrador* orquestrador,
		 boost::mutex* mutex,
		 boost::condition_variable* cond){
	const size_t bufsize = 1024;
	char buffer[bufsize];
	CLIData data;
	data.orquestrador = orquestrador;
	data.mutex = mutex;
	data.cond = cond;
	cli_parser* parser = new_cli(buffer,
				     bufsize,
				     cli_exec_command,
				     reinterpret_cast<void*>(&data));
	_int_parser_state = &parser->state;
	_int_orquestrador = orquestrador;
	int ret = 0;
	prompt(parser);
	while (ret == 0 && !cin.fail() && parser->state != STATE_STOP){
		char minibuffer[2];
		cin.read(minibuffer, 1);
		ret = process_byte(parser, minibuffer[0]);
	}
	if (ret && ret != RET_QUIT){
		cerr << "cli_parser saiu com erro" << endl;
	}
	orquestrador->close();
}

class ThreadJoiner
{
public:
	ThreadJoiner(boost::thread& thread)
	: thread_(thread)
	{}

	~ThreadJoiner(){
		// Espera pela thread apenas caso não tenha
		// ocorrido uma interrupção
		if (thread_.joinable() && !_int_happened){
//			thread_.join();
		}
	}

protected:
	boost::thread& thread_;
};

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cerr << "Usage: orquestrador <multicast_address> <multicast_port>\n";
		cerr << "       or\n";
		cerr << "       orquestrador <multicast_address> <multicast_port>\n";
		cerr << "  For IPv4, try:\n";
		cerr << "    orquestrador 239.255.0.1 9900\n";
		cerr << "  For IPv6, try:\n";
		cerr << "    orquestrador ff31::8000:1234 9900\n";
		return 1;
	}

	io_service io_service;

	signal(SIGINT, int_handler_ignore);

	boost::mutex mutex;
	boost::condition_variable cond;

	Orquestrador orquestrador(io_service,
				  ip::address::from_string(argv[1]),
				  atoi(argv[2]),
				  &mutex,
				  &cond);
	boost::thread cli_worker(cli_handler,
				 &orquestrador,
				 &mutex,
				 &cond);
	cerr << io_service.run() << endl;

	return 0;
}

