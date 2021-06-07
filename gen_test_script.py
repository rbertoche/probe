#!/usr/bin/python

def main():
    f = "dispara {0} {1}\n"
    repeticoes = 16
    out = open("test_script.orq", "w")
    out.writelines((f.format(1 << i, repeticoes)
		    for i in xrange(2, 31)))
    out.write("desliga\n")
    out.close()

if __name__ == "__main__":
    main()


