
#include <iostream>
#include <iomanip>

#include <vector>

using namespace std;


void dump(const vector<unsigned char>& data){
	ios_base::fmtflags f(cerr.flags());
	char fill = cerr.fill();
	cerr << "\t" << setw(5) << data.size() << " bytes: ";
	cerr << setfill('0') << hex;
	cerr << "0x ";
	for (vector<unsigned char>::const_iterator it = data.begin();
	     it < data.end();
	     it++){
		cerr << setw(2) << unsigned(*it) << " ";
	}
	cerr << "FIM" << endl;
	cerr.fill(fill);
	cerr.flags(f);
}
