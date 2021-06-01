#include "mensagem.h"

#include <iostream>
#include <cmath>
#include <stdlib.h>

#include "dump.h"

using namespace std;

Mensagem::Mensagem(Tipo tipo,
		   Origem origem,
		   unsigned tamanho,
		   unsigned repeticoes)
	: tipo_(tipo)
	, origem_(origem)
	, tamanho_(tamanho)
	, repeticoes_(repeticoes)
{
}

Mensagem Mensagem::unpack(const vector<unsigned char>& data)
{
	if (data.size() != 4){
		cerr << "Erro tentando desempacotar mensagem "
			"de tamanho inválido "
		     << data.size() << endl;
		dump(data);
		cerr << "Abortando" << endl;
		cerr.flush();
		abort();
	}
	Mensagem m((Tipo)data[0],
		   (Origem)data[1],
		   // Impede numeros muito grandes
		   // 65536 bytes
		   1 << (data[2] > 16 ? 16 : data[2]),
		   // 1024 repetições
		   1 << (data[3] > 10 ? 10 : data[3]));
}

vector<unsigned char> Mensagem::pack(const Mensagem& msg)
{
	vector<unsigned char> data(4);
	data[0] = msg.tipo_;
	data[1] = msg.origem_;
	data[2] = log2(msg.tamanho_);
	data[3] = log2(msg.repeticoes_);
	return data;
}
