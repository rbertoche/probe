#ifndef SOCKETOWNER_H
#define SOCKETOWNER_H

#include <boost/asio/io_service.hpp>

using namespace boost::asio;

template <typename Socket>
class SocketOwner // Provê um socket que permitiu desacoplar as outras classes
		  // Sem essa classe não seria possível inicializar um membro
		  // socket antes de chamar o construtor de outras classes
		  // base em BaseMulticastServer::BaseMulticastServer()
		  //
		  // Cada objeto deve possuir no máximo 1 recurso?
		  // Sim mas não por isso...
				  {
private:
	Socket socket__;
protected:
	SocketOwner(io_service &io_service)
		: socket__(io_service)
		, socket_(socket__)
	{}
public:
	Socket& socket_;
};

#endif // SOCKETOWNER_H
