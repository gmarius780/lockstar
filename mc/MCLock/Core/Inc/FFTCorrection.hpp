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
};

struct peak {
    float freq;
    float amp;
    float phase;
};

class FFTCorrection {
private:
    int index, fftStep;
    float batchEndTime;
    vector<float> runningSignal;
    vector<fftEntry> runningFFT;
    vector<complex<float>> fourierCoeffs;
    void GenerateFFTBasis();
    void GenerateFourierCoeffs();
public:
    int sampleRate, batchSize, supportingFreqRange;
    vector<peak> resonantPeaks;
    FFTCorrection() {
        index = 0;
        sampleRate = 10000;//pow(2, 18);
        batchSize =  pow(2, 10);
        runningSignal = vector<float>(batchSize, 0.0);
        resonantPeaks = {{3000.0, 3.0, 0}, {7000.0, 3.0, 0}};
        supportingFreqRange = 4; //Must be even
    };
    void SetParameters(float sampleRate, float batchSize, float supportingFreqRange, vector<peak> resonantPeaks) {
        this->sampleRate = sampleRate;
        this->batchSize = batchSize;
        this->supportingFreqRange = supportingFreqRange;
        this->resonantPeaks = resonantPeaks;
    };
    float CalculateCorrection(float pv);
    void Setup();
};

#endif /* FFTCorrection_hpp */