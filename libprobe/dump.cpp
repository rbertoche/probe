
#include <iostream>
#include <iomanip>

#include <vector>

using namespace std;


void dump(const vector<unsigned char>& data){
	for (vector<unsigned char>::const_iterator it = data.begin();
	     it < data.end();
	     it++){
		cerr << setw(2) << hex << unsigned(*it) << " ";
	}
	cerr << "FIM" << endl;
}
