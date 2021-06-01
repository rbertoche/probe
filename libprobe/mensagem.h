#ifndef MENSAGEM_H
#define MENSAGEM_H

#include <vector>

using namespace std;

enum Tipo {
	DISPARA=0,
	DESLIGA=1,
	EXITO=2,
	ECO=3
};

enum Origem {
	ORQUESTRADOR=0,
	CLIENTE=1,
	SERVIDOR=2
};

class Mensagem
{
protected:
	Tipo tipo_;
	Origem origem_;
	unsigned tamanho_;
	unsigned repeticoes_;
public:
	Mensagem(Tipo tipo,
		 Origem origem,
		 unsigned tamanho,
		 unsigned repeticoes);

	static Mensagem unpack(const vector<char>& data);

	static vector<char> pack(const Mensagem& msg);

	Tipo tipo(){
		return tipo_;
	}

	Origem origem(){
		return origem_;
	}

	unsigned tamanho(){
		return tamanho_;
	}

	unsigned repeticoes(){
		return repeticoes_;
	}


};

#endif // MENSAGEM_H
