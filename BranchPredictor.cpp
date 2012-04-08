#include <iostream>
#include <fstream>
#include <sstream>
#include "string.h"
#include <iomanip>
#include "stdlib.h"
#include <algorithm>

//#define DEBUG 1
//#define VERBOSE 1
using namespace std;
class Predictor {
	int m;
	int n;
	int *predTable;
	unsigned int predTableSize;
	unsigned int bhr;
	int numBranches;
	int numMispredict;
	int highLimit;
	int lowLimit;

public:
	Predictor(int mVal,int nVal, int initVal=2,int highVal=3,int lowVal = 0 );
	int incrPredTable(int index);
	int decrPredTable(int index);
	unsigned int getIndex(unsigned int pc);
	int updateBHR(int outcome);
	int run(char *file);
	int predict(unsigned int pc);
	int update(unsigned int pc,int outcome);
	int print(char *type);

};

Predictor::Predictor(int mVal,int nVal, int initVal ,int highVal,int lowVal  ) {

	m = mVal;
	n = nVal;
	highLimit = highVal;
	lowLimit = lowVal;

	numBranches = 0;
	numMispredict = 0;

	predTableSize = 1 << m;
#ifdef DEBUG
	cout << "Pred table size : " << predTableSize << " High limit : " << highLimit << " Low limit :" << lowLimit << endl;
#endif
	predTable = new int[predTableSize];
	for(int i=0;i<predTableSize;i++) {
		predTable[i] = initVal;
	}

	bhr = 0;	
}


int Predictor::incrPredTable(int index) {
	if (predTable[index] < highLimit)
		predTable[index]++;

	return 0;

}

int Predictor::decrPredTable(int index) {
	if ( predTable[index] > lowLimit)
		predTable[index]--;

	return 0; 

}

unsigned int Predictor::getIndex(unsigned int pc) {

	unsigned int mask = 0;
	for(int i=0;i<m;i++) {
		mask = mask << 1;
		mask = mask|1;
	}		

	mask = mask << 2;

	int tmp = pc & mask;
	tmp = tmp >> 2;

	int tmpBhr = bhr << (m-n);
	int index = tmpBhr ^ tmp;

	return index;

}

int Predictor::updateBHR(int outcome) {

	bhr = bhr >> 1;
	outcome = outcome << (n-1);
	bhr = bhr | outcome;

#ifdef DEBUG
	cout << "BHR value : " << bhr << endl;
#endif

	return 0;
}

int Predictor::run(char *fileName ) {

	string line;
	ifstream myfile(fileName);
	char actualOutcome;
	char address[9];
	unsigned int addr;
	int outcome;

	if ( myfile.is_open() ) {

		while ( getline(myfile , line)) {
			actualOutcome = line.at(line.size() -1 );
			line.copy(address,line.size()-2,0);

                        istringstream iss(address);
			numBranches++;
			iss >> hex >> addr;

#ifdef VERBOSE
			cout << hex << addr << " " << actualOutcome << endl;
#endif

			if (actualOutcome == 't' )
				outcome = 1;
			else
				outcome = 0;

			if( predict(addr) != outcome )
				numMispredict++;

			update(addr,outcome);	

		}

	}

}

int Predictor::predict(unsigned int pc) {
	int index;
	index = getIndex(pc);

#ifdef DEBUG
	cout << "Index : " << index << " cond : " << highLimit/2 << endl;
#endif

	if (predTable[index] > highLimit/2 ) {
		return 1;
	} else { 
		return 0;
	}
}

int Predictor::update( unsigned int pc,int outcome ) {

	int index = getIndex(pc);

	if ( outcome == 1  ) {
		incrPredTable(index);
	} else {
		decrPredTable(index);
	}

	if ( n > 0)
		updateBHR(outcome);
}

int Predictor::print( char *type) {

	double misPredictRate;
	misPredictRate = (double)numMispredict/(double)numBranches * 100 ;
	cout << "number of predictions:\t" << numBranches << endl;
	cout << "number of mispredictions:\t" << numMispredict << endl;
	cout << "misprediction rate:\t" << fixed << setprecision(2) << misPredictRate << "%" << endl;


	string str(type);
	transform(str.begin(),str.end(),str.begin(),::toupper);	
	//string type = "BIMODAL";
	cout << "FINAL " << str << " CONTENTS" << endl;

	for(int i=0;i<predTableSize;i++) {
		cout << i << "\t" << predTable[i] << endl;
	}
}

int main(int argc,char *argv[]) {

	int m,n;

	char *type = argv[1];
	char *file;
	cout << "COMMAND" << endl;
	cout << "./sim " << type << " "; 
	if (strcmp(type,"bimodal") == 0 ) {
		m=atoi(argv[2]);
		n=0;
		file = argv[3];
		cout << m << " " ;
	} else if(strcmp(type,"gshare") ==0) {
		m=atoi(argv[2]);
		n = atoi(argv[3]);
		file = argv[4];
		cout << m << " " << n << " " ;
	}

	cout << file << endl;
	
	Predictor p1(m,n);

	cout << "OUTPUT" << endl;
	p1.run( file );

	p1.print( type );
/*
	cout << "index: " << p1.getIndex(0x2F) << endl;
//	p1.updateBHR(1);
	

	p1.decrPredTable( p1.getIndex(0x2F) );
	p1.print();
	cout << "Result : " << p1.predict(0x2F) << endl; 
*/
}
