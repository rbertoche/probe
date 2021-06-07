#!/usr/bin/python

import sys

def main():
    if len(sys.argv) != 4:
        print("usage: {0} expoente_tamanho_maximo repeticoes arquivo_de_saida")

    f = "dispara {0} {1}\n"
    tamanho = int(sys.argv[1]) + 1
    repeticoes = int(sys.argv[2])
    out = open(sys.argv[3], "w")
    out.writelines((f.format(1 << i, repeticoes)
		    for i in xrange(2, tamanho)))
    out.write("desliga\n")
    out.close()

if __name__ == "__main__":
    main()


