#include "wrappers.h"
#include <iostream>
#include <time.h>
#include <fstream>
#include <cmath>
#include <cstdlib>

int main(void)
{
	cout << "Task 2\n";

	// initialize the random seed
	srand(time(NULL));

	// read source file
	double phi = 0.0;//1*19 + 2*19; // S is the nineteenth letter in the alphabet
	int fileSize;
	int garbage;
	
	/*vector<int> source = fImageSource("../data/t0.txt", fileSize);
	vector<int> spam1 = fImageSource("../data/t1.txt", garbage);
	vector<int> spam2 = fImageSource("../data/t2.txt", garbage);*/

	vector<int> source = fImageSource("../data/im0.jpg", fileSize);
	vector<int> spam1 = fImageSource("../data/im1.jpg", garbage);
	vector<int> spam2 = fImageSource("../data/im2.jpg", garbage);

	// generate gold sequences
	vector<int> pol1(5, 1);
	vector<int> pol2(5, 1);
	pol1[1] = 0;
	pol1[2] = 0;
	pol2[2] = 0;
	pol2[3] = 0;
	vector<int> m1 = fMSeqGen(pol1);
	vector<int> m2 = fMSeqGen(pol2);
	vector<int> goldSeq = fGoldSeq(m1, m2, 2);

	// d >= 1 + (19+19) mod 12 -> d >= 3
	vector<int> goldSeqMAI1 = fGoldSeq(m1, m2, 6);
	vector<int> goldSeqMAI2 = fGoldSeq(m1, m2, 9);
	//vector<int> pilotGoldSeq = fGoldSeq(m1, m2, 10);

	// perform the DS-QPSK modulation
	vector<vector<complex<double> > > channelIn;

// dbg
cout << "sourcelength: " << source.size() << endl;
cout << source[0] << source[1] << source[3] << endl;


	// since the signal s1 arrives via three paths, we push it times
	vector<complex<double> > s1 = fDSQPSKModulator(source, goldSeq, phi);
	for (int i = 0; i < 3; i++)
		channelIn.push_back(s1);

	/* since the receiver does not know, which bits are transmitted right
	the start, we send a pilot signal from the same transmitter with a
	different pn-code, sending only 1s. The estimator will then be able
	to estimate the complex betas.
	*/
	/*vector<int> ones(10, 1);
	vector<complex<double> > pilotSignal = fDSQPSKModulator(ones, pilotGoldSeq, phi);
	for (int i = 0; i < 3; i++)
		channelIn.push_back(pilotSignal);*/

	// push the MAI-signals
	channelIn.push_back(fDSQPSKModulator(spam1, goldSeqMAI1, phi));
	channelIn.push_back(fDSQPSKModulator(spam2, goldSeqMAI2, phi));

	// set up channel parameters
	vector<int> delay(8, 0);
	delay[0] = (19 + 19) % 4;
	delay[1] = 4 + (19 + 19) % 5;
	delay[2] = 9 + (19 + 19) % 6;
	delay[3] = 8;
	delay[4] = 11;

	vector<complex<double> > beta(8, complex<double>(0.9, 0.0));
	beta[1] = complex<double>(0.8*cos(-3.14159/6), 0.8*sin(-3.14159/6));
	beta[2] = complex<double>(0.7*cos(3.14159/4), 0.7*sin(3.14159/4));
	beta[3] = complex<double>(0.06, 0.0);
	beta[4] = complex<double>(0.05, 0.0);


for (int i = 0; i < 8; i++)
	cout << "delay " << i << " = " << delay[i] << endl;

for (int i = 0; i < 8; i++)
	cout << "beta " << i << " = " << beta[i] << endl;

	vector<struct DOAStruct> DOA;

	double SNR = 40.0;

	vector<vector<double> > array;

	// apply signals to the channel
	vector<vector<complex<double> > > channelOut = fChannel(
		channelIn, delay, beta, DOA, SNR, array);

	// point T^_1, system simulation up to the receivers input is completed
	// channel output will therefore be stored in a file ("channelOut.txt")
	ofstream file;
	file.open("channelOut.txt");
	for (int i = 0; i < channelOut.size(); i++)
	{
		file << "SIGNAL VECTOR " << i << endl;
		for (int j = 0; j < channelOut[i].size(); j++)
		{
			file << channelOut[i][j] << endl;
		}
	}

	struct fChannelEstimationStruct est = fChannelEstimation(channelOut, goldSeq, 3);
	for (int i = 0; i < 3; i++)
	{
		cout << "delay " << i << " = " << est.delay_estimate[i] << endl;
		cout << "beta " << i << " = " << abs(est.beta_estimate[i]) << endl;
	}

	// demodulate signal
	vector<int> sinkBits = fDSQPSKDemodulator(channelOut, est, goldSeq, phi);

	// save file
	fImageSink(sinkBits, "out.jpg", fileSize);


	/*cout << "crosscorrlations: " << endl;

	int sum;
	for (int i = 0; i < goldSeq.size(); i++)
	{
		sum = 0;
		for (int j = 0; j < goldSeq.size(); j++)
		{
			sum += goldSeq[(j+i)%goldSeq.size()]*pilotGoldSeq[j];
			//cout << "adding " << goldSeq[(j+i)%goldSeq.size()]*pilotGoldSeq[j] << endl;
		}
		cout << sum << endl;
	}

	cout << "autocorrlations: " << endl;

	int sum1;
	for (int i = 0; i < goldSeq.size(); i++)
	{
		sum1 = 0;
		for (int j = 0; j < goldSeq.size(); j++)
		{
			sum1 += pilotGoldSeq[(j+i)%goldSeq.size()]*pilotGoldSeq[j];
			//cout << "adding " << goldSeq[(j+i)%goldSeq.size()]*pilotGoldSeq[j] << endl;
		}
		cout << sum1 << endl;
	}*/

	return 0;
}