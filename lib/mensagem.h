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

	static Mensagem unpack(const vector<unsigned char>& data);

	static void pack(vector<unsigned char>& dest, const Mensagem& msg);

	static vector<unsigned char> pack(const Mensagem& msg);

	Tipo tipo() const{
		return tipo_;
	}

	Origem origem() const{
		return origem_;
	}

	unsigned tamanho() const{
		return tamanho_;
	}

	unsigned repeticoes() const{
		return repeticoes_;
	}


};

#endif // MENSAGEM_H
