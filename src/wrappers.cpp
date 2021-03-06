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

	// number of desired signal paths
	int nPaths = channelEstimation.delay_estimate.size();
	complex<double> cnPaths((double)nPaths, 0.0);
	for (int i = 0; i < outLength/2; i++)
	{
		// compute the average real and imaginary part
		complex<double> sum(0.0,0.0);
		for (int k = 0; k < GoldSeq.size(); k++)
		{
			complex<double> pmVal(GoldSeq[k],0.0);
			complex<double> phase(cos(phi*2*3.14159/360),sin(phi*2*3.14159/360));

			for (int b = 0; b < nPaths; b++)
			sum += symbolsIn[0][
				(i*GoldSeq.size()+k+channelEstimation.delay_estimate[b])
				% inLength]
				/ channelEstimation.beta_estimate[b] / cnPaths / phase * pmVal;	
		}
		double evenAv = sum.real() / GoldSeq.size();
		double oddAv = sum.imag() / GoldSeq.size();

		// cout << evenAv*evenAv+oddAv*oddAv << endl;

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
	vector<vector<complex<double> > > symbolsIn, vector<int> goldseq,
	int numberOfDesiredPaths, double phi)
{
	//function works only for single path and single antenna right now
	struct fChannelEstimationStruct channelEstimation;
	int length = symbolsIn[0].size();

	// estimate delay
	int estimatedDelay;
	double maxAutoCorr = 0.0;
	channelEstimation.beta_estimate.resize(numberOfDesiredPaths,
		complex<double>(0.0, 0.0));
	channelEstimation.delay_estimate.resize(numberOfDesiredPaths, 0);
	for (int delay = 0; delay < goldseq.size(); delay++)
	{
		complex<double> autoCorr(0.0, 0.0);
		for (int i = 0; i < goldseq.size(); i++)
		{
			complex<double> cChip((double)goldseq[i], 0.0);
			autoCorr += cChip * symbolsIn[0][(i + delay) % length];		
		}

		// keep a list of the abs-greatest beta_values and according delays
		if (abs(autoCorr) > abs(channelEstimation.beta_estimate[
			numberOfDesiredPaths-1]))
		{
			channelEstimation.beta_estimate[numberOfDesiredPaths-1] = autoCorr;
			channelEstimation.delay_estimate[numberOfDesiredPaths-1] = delay;
			for (int i = numberOfDesiredPaths-2; i >= 0; i--)
			{
				if (abs(channelEstimation.beta_estimate[i]) <
					abs(channelEstimation.beta_estimate[i+1]))
				{
					// swap
					complex<double> tmp = channelEstimation.beta_estimate[i];
					int tmp2 = channelEstimation.delay_estimate[i];

					channelEstimation.beta_estimate[i] =
						channelEstimation.beta_estimate[i+1];
					channelEstimation.delay_estimate[i] =
						channelEstimation.delay_estimate[i+1];

					channelEstimation.beta_estimate[i+1] = tmp;
					channelEstimation.delay_estimate[i+1] = tmp2;
				}
			}
		}
	}

	// estimate beta
	complex<double> divisor(goldseq.size(), goldseq.size());

	for (int i = 0; i < numberOfDesiredPaths; i++)
		channelEstimation.beta_estimate[i] /= divisor
			* complex<double>(cos(phi*2*3.14159/360),sin(phi*2*3.14159/360));
;
	// since the frist two bits are pilot bits set on 1, the complex number should be 1+i

	return channelEstimation;
}



vector<vector<complex<double> > > fChannel(
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



	// compute array manifold vector
	int nOutputs = array.size();
	int nInputs = symbolsIn.size(); // number of inputs
	vector<vector<complex<double> > > manifold(nOutputs,
		vector<complex<double> >(nInputs, complex<double>(0.0, 0.0)));
	for (int i = 0; i < nOutputs; i++)
	{
		for (int k = 0; k < nInputs; k++)
		{
			double scaled_phase = 0.0;
			scaled_phase += cos(DOA[k].azimuth) * cos(DOA[i].elevation) * array[i][0];
			scaled_phase += sin(DOA[k].azimuth) * cos(DOA[i].elevation) * array[i][1];
			scaled_phase += sin(DOA[k].elevation) * array[i][2];
			manifold[i][k] = complex<double> (cos(3.14159*scaled_phase),
				sin(3.14159*scaled_phase));
		}
	}

	int outLength = longestMsg;
	out.resize(nOutputs);
	for (int i = 0; i < nOutputs; i++)
		out[i].resize(outLength);

	for (int i = 0; i < outLength; i++)
	{
		for (int k = 0; k < nOutputs; k++)
		{
			complex<double> tmp(0.0, 0.0);
			for (int b = 0; b < nInputs; b++)
			{
				tmp += beta[b]*manifold[k][b]*symbolsIn[b][(i+symbolsIn[b].size()-delay[k]) % symbolsIn[b].size()];
			}
			tmp += whiteGaussianNoise(0.0, stdDev);
			out[k][i] = tmp;
		}
	}

	return out;
}



void fImageSink(vector<int> bitsIn, string path, int fileSize)
{
	ofstream file;
	char * filename = (char *)path.c_str();
	char * buffer = new char[fileSize];
	for (int i = 0; i < fileSize; i++)
	{
		char tmp = 0;
		for (int j = 0; j < 7; j++)
		{
			tmp += bitsIn[i*8+2+j]; // add 2 to the index in order go ignore the pilot bits
			tmp = tmp << 1;
		}
		tmp += bitsIn[i*8+2+7];
		buffer[i] = tmp;
	}


	file.open(filename);
	file.write(buffer, fileSize);
	file.close();
}



void fImageSinkNoPilot(vector<int> bitsIn, string path)
{
	ofstream file;
	int fileSize = bitsIn.size()/8;
	char * filename = (char *)path.c_str();
	char * buffer = new char[fileSize];
	for (int i = 0; i < fileSize; i++)
	{
		char tmp = 0;
		for (int j = 0; j < 7; j++)
		{
			tmp += bitsIn[i*8+j]; // add 2 to the index in order go ignore the pilot bits
			tmp = tmp << 1;
		}
		tmp += bitsIn[i*8+7];
		buffer[i] = tmp;
	}


	file.open(filename);
	file.write(buffer, fileSize);
	file.close();
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
	bitsOut.resize(bufferSize*8+2);
	bitsOut[0] = bitsOut[1] = 1; // add 2 pilot bits
	// see comment in fChannelEstimator
	for (int i = 0; i < bufferSize; i++)
	{
		char tmp = buffer[i];
		for (int j = 0; j < 8; j++)
		{
			if (tmp & (1 << (7-j))) bitsOut[i*8+2+j] = 1;
			else bitsOut[i*8+2+j] = 0;
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