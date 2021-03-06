#pragma once

#include <vector>
#include <complex>

using namespace std;

vector<int> fDSQPSKDemodulator(
	vector<vector<complex<double> > > symbolsIn,
	struct fChannelEstimationStruct fChannelEstimation,
	vector<int> GoldSeq, double phi);


struct DOAStruct
{
	double azimuth;
	double elevation;
};

struct fChannelEstimationStruct
{
	vector<int> delay_estimate;
	vector<struct DOAStruct> DOA_estimate;
	vector<complex<double> > beta_estimate;
};

struct fChannelEstimationStruct fChannelEstimation(
	vector<vector<complex<double> > > symbolsIn, vector<int> goldseq,
	int numberOfDesiredPaths, double phi);




vector<vector<complex<double> > > fChannel(
	vector<vector<complex<double> > > symbolsIn, vector<int> delay,
	vector<complex<double> > beta, vector<struct DOAStruct> DOA, double SNR,
	vector<vector<double> > array);

void fImageSink(vector<int> bitsIn, string filname, int fileSize);

void fImageSinkNoPilot(vector<int> bitsIn, string path);

vector<int> fImageSource(string filename, int &fileSize);

vector<complex<double> > fDSQPSKModulator(vector<int> bitsIn,
	vector<int> goldseq, double phi);

vector<int> fGoldSeq(vector<int> mseq1, vector<int> mseq2, int shift);

vector<int> fMSeqGen(vector<int> coeffs);