//
//  FFTCorrection.hpp
//  c_version
//
//  Created by Philip Leindecker on 04.06.20.
//  Copyright © 2020 Philip Leindecker. All rights reserved.
//

#ifndef FFTCorrection_hpp
#define FFTCorrection_hpp

#include <stdio.h>
#include <vector>
#include <complex>
using namespace std;

struct fftEntry {
    float freq;
    complex<float> fft;
    float supportSin;
    float supportCos;
    float shiftSin;
    float shiftCos;
};

struct peak {
    float freq;
    float fftCoeffs; //Must be even or 1
    float amplitudeShift;
    float phaseShift;
};

class FFTCorrection {
private:
    int runningIndex;
    float batchEndTime, fftStep, amplitudeNorm, pvOut, diffSignalX, m_pi;
    vector<float> runningSignal;
    vector<fftEntry> runningFFT1, runningFFT2;
    vector<fftEntry> *activeFFT, *passiveFFT;
    unsigned int runningFFTSize;
    vector<complex<float>> fourierCoeffs;
    vector<complex<float>> amplitudeCorr;
    complex<float> tempResult;
    void SwapFFTs();
    void ResetActiveFFT();
    void GenerateFFTBasis();
    void GenerateFourierCoeffs();
    float sinc(float x);
    void GenerateAmplitudeCorrection();
public:
    float sampleRate, batchSize;
    vector<peak> resonantPeaks;
    FFTCorrection(vector<peak> resonantPeaks) {
        runningIndex = 0;
        sampleRate = 10000.0; // Hz
        batchSize =  1000.0; // Data points
        this->resonantPeaks = resonantPeaks;
        m_pi = 3.14159265;

        activeFFT = &runningFFT1;
        passiveFFT = &runningFFT2;
    };
    void SetParameters(float sampleRate, float batchSize) {
        this->sampleRate = sampleRate;
        this->batchSize = batchSize;
    };
    float CalculateCorrection(float pv);
    void Setup();
};

#endif /* FFTCorrection_hpp */


