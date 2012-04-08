#include <iostream>
#include <fstream>
#include <sstream>
#include "string.h"
#include <iomanip>
#include "stdlib.h"
#include <algorithm>

//#define DEBUG 1
//#define VERBOSE 1
char file[3][20] = {"gcc_trace.txt","jpeg_trace.txt" , "perl_trace.txt"};
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
	char fName[20];

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

	strcpy(fName,fileName);
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

	double misPredictRate;
	misPredictRate = (double)numMispredict/(double)numBranches * 100 ;
	cout << "number of predictions:\t" << numBranches << endl;
	cout << "number of mispredictions:\t" << numMispredict << endl;
	cout << fName << " " << m << " " << n << " " <<  "misprediction rate:\t" << fixed << setprecision(2) << misPredictRate << "%" << endl;



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


	string str(type);
	transform(str.begin(),str.end(),str.begin(),::toupper);	
	//string type = "BIMODAL";
	cout << "FINAL " << str << " CONTENTS" << endl;

	for(int i=0;i<predTableSize;i++) {
		cout << i << "\t" << predTable[i] << endl;
	}
}

class HybridPredictor {
	Predictor *bimodal;
	Predictor *gshare;
	int k;
	int *chooserTable;
	int chooserTableSize;
	int numBranches;
	int numMispredict;
	int highLimit;
	int lowLimit;
public:
	HybridPredictor(int kVal,int m1Val,int nVal,int m2Val );
	int getIndex(unsigned int pc);
	int incrChooserTable(int index);
	int decrChooserTable(int index);
	int predict(unsigned int pc,int outcome);
	int run(char *fileName);
	int print();
};

HybridPredictor::HybridPredictor(int kVal,int m1Val,int nVal,int m2Val ) {

	k = kVal;
	chooserTableSize = 1 << k;
	chooserTable = new int[chooserTableSize];
	gshare = new Predictor(m1Val,nVal);
	bimodal = new Predictor(m2Val,0);
	numBranches = 0;
	numMispredict = 0;

	highLimit  =3;
	lowLimit = 0;
	for(int i=0;i<chooserTableSize;i++)
		chooserTable[i] = 1;

}

int HybridPredictor::getIndex(unsigned int pc) {

        unsigned int mask = 0;
        for(int i=0;i<k;i++) {
                mask = mask << 1;
                mask = mask|1;
        }

        mask = mask << 2;

        int index = pc & mask;
        index = index >> 2;

	return index;

}

int HybridPredictor::incrChooserTable(int index) {
        if (chooserTable[index] < highLimit)
                chooserTable[index]++;

        return 0;


}

int HybridPredictor::decrChooserTable(int index) {

        if ( chooserTable[index] > lowLimit)
                chooserTable[index]--;

        return 0;

}

int HybridPredictor::predict(unsigned int pc,int outcome) {

	int index = getIndex(pc);

	int bimodalPred = bimodal->predict(pc);
	int gsharePred = gshare->predict(pc);

	int finalPred;
	
	if(chooserTable[index] > 1) {
		finalPred = gsharePred;
		gshare->update(pc,outcome);
	
	} else {
		finalPred = bimodalPred;
		bimodal->update(pc,outcome);
		gshare->updateBHR(outcome);
	}

	if ( bimodalPred == outcome && gsharePred != outcome )
		decrChooserTable(index);
	else if( bimodalPred != outcome && gsharePred == outcome)
		incrChooserTable(index);

	return finalPred;
}

int HybridPredictor::run(char *fileName) {

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

                        if( predict(addr,outcome) != outcome )
                                numMispredict++;

                        //update(addr,outcome);

                }

        }

}

int HybridPredictor::print() {
	double misPredictRate;
	misPredictRate = (double)numMispredict/(double)numBranches * 100 ;
	cout << "number of predictions:\t" << numBranches << endl;
	cout << "number of mispredictions:\t" << numMispredict << endl;
	cout << "misprediction rate:\t" << fixed << setprecision(2) << misPredictRate << "%" << endl;


	cout << "FINAL CHOOSER CONTENTS" << endl;
	for(int i=0;i<chooserTableSize ; i++) {
		cout << i << "\t" << chooserTable[i] << endl;
	}

	gshare->print("gshare");
	bimodal->print("bimodal");
}


int exp1() {
	for(int i=0;i<3;i++) { // for each of the three file
	
		for(int m=7;m<=12;m++) {
			Predictor p1(m,0);
			p1.run(file[i]);
		}
	}

}
int exp2() {
	for(int i=0;i<3;i++) { // for each of the three file
	
		for(int m=7;m<=12;m++) {
			for(int n=2;n<=m;n+=2) {
				Predictor p1(m,n);
				p1.run(file[i]);
			}
		}
	}

}
int main(int argc,char *argv[]) {

//	exp1();

	exp2();
/*
	char *type = argv[1];
	char *file;
	cout << "COMMAND" << endl;
	cout << "./sim " << type << " ";
 
	if (strcmp(type,"bimodal") == 0 ) {
		m=atoi(argv[2]);
		n=0;
		file = argv[3];
		cout << m << " " << file << endl ;
	} else if(strcmp(type,"gshare") ==0) {
		m=atoi(argv[2]);
		n = atoi(argv[3]);
		file = argv[4];
		cout << m << " " << n << " " << file << endl ;
	} else if(strcmp(type,"hybrid") == 0) {
		k = atoi(argv[2]);
		m = atoi(argv[3]);
		n = atoi(argv[4]);
		mBimodal = atoi(argv[5]);
		file = argv[6];
		cout << k << " " << m << " " << n << " " << mBimodal << " " << file << endl;
	} else {
		cout << "Invalid type of predictor" << endl;
		exit(1);
	}

	if ( strcmp(type,"hybrid") == 0 ) {
		HybridPredictor hp(k,m,n,mBimodal);
		hp.run(file);
	
		cout << "OUTPUT" << endl;
		hp.print();

	} else {
		Predictor p1(m,n);

		cout << "OUTPUT" << endl;
		p1.run( file );

		p1.print( type );
	}

	cout << "index: " << p1.getIndex(0x2F) << endl;
//	p1.updateBHR(1);
	

	p1.decrPredTable( p1.getIndex(0x2F) );
	p1.print();
	cout << "Result : " << p1.predict(0x2F) << endl; 
*/

}
