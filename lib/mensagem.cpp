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
	if (data.size() < 4){
		cerr << "Erro tentando desempacotar mensagem "
			"de tamanho inválido "
		     << data.size() << endl;
		dump(data);
		cerr << "Abortando" << endl;
		cerr.flush();
		abort();
	}
#ifdef DEBUG_1
	cerr << "desempacotando mensagem: ";
	vector<unsigned char> v(4);
	copy(data.begin(), data.begin()+4, v.begin());
	dump(v);
#endif
	unsigned repeticoes;
	if (data[0] != EXITO) {
		// 1024 repetições;
//		repeticoes = 1 << (data[3] > 31 ? 31 : data[3]);
	} else {
		repeticoes = data[3];
	}
	return Mensagem((Tipo)data[0],
			(Origem)data[1],
			// Impede numeros muito grandes
			// 65536 bytes
			1 << (data[2] > 20 ? 20 : data[2]),
			repeticoes);
}

void Mensagem::pack(vector<unsigned char>& dest, const Mensagem& msg)
{
	dest[0] = msg.tipo_;
	dest[1] = msg.origem_;
	dest[2] = round(log2(msg.tamanho_));
	if (msg.tipo_ != EXITO){
		dest[3] = round(log2(msg.repeticoes_));
	} else {
		dest[3] = msg.repeticoes_;
	}
}
vector<unsigned char> Mensagem::pack(const Mensagem& msg)
{
	vector<unsigned char> data(4);
	pack(data,msg);
	return data;
}
