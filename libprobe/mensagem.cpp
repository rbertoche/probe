#include "mensagem.h"

#include <iostream>
#include <cmath>
#include <stdlib.h>

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

Mensagem Mensagem::unpack(const vector<char>& data)
{
	if (data.size() != 4){
		cerr << "Erro tentando desempacotar mensagem "
			"de tamanho inválido "
		     << data.size() << endl;
		cerr << "Abortando" << endl;
		cerr.flush();
		abort();
	}
	return Mensagem(Tipo(data[0]),
			Origem(data[1]),
			1 << data[2],
			1 << data[3]);
}

vector<char> Mensagem::pack(const Mensagem& msg)
{
	vector<char> data(4);
	data[0] = msg.tipo_;
	data[1] = msg.origem_;
	data[2] = log2(msg.tamanho_);
	data[3] = log2(msg.repeticoes_);
	return data;
}
