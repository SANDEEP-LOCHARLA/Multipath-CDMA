#include "wrappers.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cmath>
//#include <random>
#include "utils.h"

vector<int> fDSQPSKDemodulator(
	vector<vector<complex<double> > > symbolsIn,
	struct fChannelEstimationStruct channelEstimation,
	vector<int> GoldSeq,
	double phi)
{
	vector<int> out;
	int inLength = symbolsIn[0].size();
	int outLength = 2 * symbolsIn[0].size() / GoldSeq.size();
	out.resize(outLength);

	for (int i = 0; i < outLength/2; i++)
	{
		// compute the average real and imaginary part
		complex<double> sum(0.0,0.0);
		for (int k = 0; k < GoldSeq.size(); k++)
		{
			complex<double> pmVal(GoldSeq[k],0.0);
			complex<double> phase(cos(phi*2*3.14159/360),sin(phi*2*3.14159/360));

			sum += symbolsIn[0][
				(i*GoldSeq.size()+k+channelEstimation.delay_estimate[0])
				% inLength] / phase * pmVal;		
		}
		double evenAv = sum.real() / GoldSeq.size();
		double oddAv = sum.imag() / GoldSeq.size();

		// decide for the one with the lowest distance
		if ((evenAv-1.0)*(evenAv-1.0) < (evenAv+1.0)*(evenAv+1.0))
			out[2*i] = 1;
		else
			out[2*i] = 0;

		if ((oddAv-1.0)*(oddAv-1.0) < (oddAv+1.0)*(oddAv+1.0))
			out[2*i+1] = 1;
		else
			out[2*i+1] = 0;
	}

	return out;
}



struct fChannelEstimationStruct fChannelEstimation(
	vector<vector<complex<double> > > symbolsIn, vector<int> goldseq)
{
	//function works only for single path and single antenna right now
	struct fChannelEstimationStruct channelEstimation;
	int length = symbolsIn[0].size();

	// estimate delay
	int estimatedDelay;
	double maxAutoCorr = 0.0;
	for (int delay = 0; delay < goldseq.size(); delay++)
	{
		complex<double> autoCorr(0.0, 0.0);
		for (int i = 0; i < goldseq.size(); i++)
		{
			complex<double> cChip((double)goldseq[i], 0.0);
			autoCorr += cChip * symbolsIn[0][(i + delay) % length];		
		}

		if (abs(autoCorr) > maxAutoCorr)
		{
			maxAutoCorr = abs(autoCorr);
			estimatedDelay = delay;
		}
	}
	channelEstimation.delay_estimate.push_back(estimatedDelay);

	// estimate beta
	channelEstimation.beta_estimate.push_back(maxAutoCorr / goldseq.size() / sqrt(2.0));

	return channelEstimation;
}



vector<vector<complex<double> > > fChannel(vector<int> paths,
	vector<vector<complex<double> > > symbolsIn, vector<int> delay,
	vector<complex<double> > beta, vector<struct DOAStruct> DOA, double SNR,
	vector<vector<double> > array)
{
	vector<vector<complex<double> > > out;

	// compute the standard deviation for gaussian random numbers
	double stdDev = sqrt(2/ pow(10.0,0.1*SNR) );

	// find length of longest message
	int longestMsg = 0;
	for (int i = 0; i < symbolsIn.size(); i++)
		if (symbolsIn[i].size() > longestMsg) longestMsg = symbolsIn[i].size();

	// for now, we'll be operating with one output
	int outLength = longestMsg;
	int nInputs = symbolsIn.size(); // number of inputs
	out.resize(1);
	out[0].resize(outLength);

	for (int i = 0; i < outLength; i++)
	{
		out[0][i] = complex<double>(0.0,0.0);
		for (int k = 0; k < nInputs; k++)
		{
			out[0][i] += beta[k]*symbolsIn[k][(i+symbolsIn[k].size()-delay[k]) % symbolsIn[k].size()];
		}

		double noise;

		out[0][i] += whiteGaussianNoise(0.0, stdDev);
	}

	return out;
}



void fImageSink(vector<int> bitsIn, string path, int fileSize)
{
	ofstream file;
	char * filename = (char *)path.c_str();
	//int fileSize = bitsIn.size()/8;
cout << "fileSize = " << fileSize << endl;

	char * buffer = new char[fileSize];
	for (int i = 0; i < fileSize; i++)
	{
		char tmp = 0;
		for (int j = 0; j < 7; j++)
		{
			tmp += bitsIn[i*8+j];
			tmp = tmp << 1;
		}
		tmp += bitsIn[i*8+7];
		buffer[i] = tmp;
//cout << "wrote bit " << i << endl;
	}


	file.open(filename);

	for (int i = 0; i < fileSize; i++)
	{
		file.write(buffer+i, 1);
		cout << "written into file: " << file.tellp() << " after " << i << endl;
	}
	file.close();
	cout << "fileSizeAfter = " << filesize((char*)path.c_str()) << endl;
}



vector<int> fImageSource(string filename, int &fileSize)
{
	vector<int> bitsOut;

	ifstream file;
	file.open((char *)filename.c_str());
	int bufferSize = filesize((char *)filename.c_str());
	fileSize = bufferSize;

	char * buffer = new char[bufferSize];

	// read file
	file.read(buffer, bufferSize);
	file.close();

	// decompose into single bits
	bitsOut.resize(bufferSize*8);
	for (int i = 0; i < bufferSize; i++)
	{
		char tmp = buffer[i];
		for (int j = 0; j < 8; j++)
		{
			if (tmp & (1 << (7-j))) bitsOut[i*8+j] = 1;
			else bitsOut[i*8+j] = 0;
		}
	}

	return bitsOut;
}



vector<complex<double> > fDSQPSKModulator(vector<int> bitsIn,
	vector<int> goldseq, double phi)
{
	// imaginary unit
	complex<double> j(0.0,1.0);

	int inLength = bitsIn.size();
	vector<complex<double> > tmp(inLength/2 + inLength%2, 0);

	int outLength = tmp.size() * goldseq.size();
	vector<complex<double> > out(outLength, 0);

	// create new bit vector
	// move odd bits to imaginary part, even bits to real part
	for (int i = 0; i < inLength; i++)
	{
		if (!bitsIn[i]) bitsIn[i] = -1;
		complex<double> bit(bitsIn[i],0.0);
		if (i%2) tmp[i/2] += j*bit;
		else tmp[i/2] += bit;
	}

	complex<double> phase(cos(phi*2*3.14159/360),sin(phi*2*3.14159/360));

	// spread with pn-code and multiply with exp(j*phi)
	for (int i = 0; i < outLength; i++)
	{
		complex<double> pnVal(goldseq[i%goldseq.size()], 0.0);
		out[i] = phase * tmp[i/goldseq.size()] * pnVal;
	}
	return out;
}



vector<int> fGoldSeq(vector<int> mseq1, vector<int> mseq2, int shift)
{
	vector<int> out;
	int N_c = mseq1.size();
	out.resize(N_c);

	for(int i = 0; i < N_c; i++)
		out[i] = 1 - 2*((mseq1[i] + mseq2[(i+shift) % N_c]) % 2);
	return out;
}



vector<int> fMSeqGen(vector<int> coeffs)
{
	vector<int> out;
	int m = coeffs.size() - 1;

	// Compute N_c = 2^m - 1
	int N_c = 1;
	for (int i = 0;  i < m; i++)
		N_c *= 2;
	N_c--;

	out.resize(N_c);

	// init shift registers with 1s
	vector<int> shiftRegs;
	shiftRegs.resize(m);
	for (int i = 0; i < m; i++)
		shiftRegs[i] = 1;

	int adderOut;

	// enter the loop
	for (int i = 0; i < N_c; i++)
	{
		out[i] = shiftRegs[m-1];
		adderOut = 0;
		for (int j = m-1; j > 0; j--)
		{
			if (coeffs[j+1]) adderOut += shiftRegs[j];
			shiftRegs[j] = shiftRegs[j-1];
		}
		if (coeffs[1]) adderOut+= shiftRegs[0];
		adderOut %= 2;
		shiftRegs[0] = adderOut;
	}
	return out;
}