//
//  FFTCorrection.cpp
//  c_version
//
//  Created by Philip Leindecker on 04.06.20.
//  Copyright © 2020 Philip Leindecker. All rights reserved.
//

#include "FFTCorrection.hpp"
//#include <iostream>

//__attribute__((section("sram_func")))
float FFTCorrection::CalculateCorrection(float pv) {

    pvOut = pv;
    diffSignalX = pv - runningSignal[runningIndex];

    for (int i = 0; i < runningFFTSize; i++) {
        // Running FFT
        // X_k = exp(i*2*π*k/N) * (X_k - x_m + x_{N+m})
        (*activeFFT)[i].fft = fourierCoeffs[i] * ((*activeFFT)[i].fft + diffSignalX);
        (*passiveFFT)[i].fft = fourierCoeffs[i] * ((*passiveFFT)[i].fft + pv);

        tempResult = (*activeFFT)[i].fft * amplitudeCorr[i];

        pvOut -= amplitudeNorm * ((*activeFFT)[i].supportSin * tempResult.real() + (*activeFFT)[i].supportCos * tempResult.imag());
        pvOut += amplitudeNorm * ((*activeFFT)[i].shiftSin * tempResult.real() + (*activeFFT)[i].shiftCos * tempResult.imag());

        // Generate output signal
        //pvOut -= amplitudeNorm * (*activeFFT)[i].scaling * ((*activeFFT)[i].supportSin * (*activeFFT)[i].fft.real() + (*activeFFT)[i].supportCos * (*activeFFT)[i].fft.imag());
        //pvOut += amplitudeNorm * (*activeFFT)[i].scaling * ((*activeFFT)[i].ShiftSin * (*activeFFT)[i].fft.real() + (*activeFFT)[i].ShiftCos * (*activeFFT)[i].fft.imag());
    }

    runningSignal[runningIndex] = pv;
    runningIndex += 1;
    if (runningIndex == batchSize) {
        runningIndex = 0;
        // reset active FFT to zero
        ResetActiveFFT();
        // swap active and passiveFFT
        SwapFFTs();
    }

    return pvOut;
}

//__attribute__((section("sram_func")))
void FFTCorrection::SwapFFTs() {
    vector<fftEntry> *temp = activeFFT;
    activeFFT = passiveFFT;
    passiveFFT = temp;
}

//__attribute__((section("sram_func")))
void FFTCorrection::ResetActiveFFT() {
    for (int i = 0; i < runningFFTSize; i++)
    (*activeFFT)[i].fft = 0;
}

void FFTCorrection::GenerateFFTBasis() {
    for (int i = 0; i < resonantPeaks.size(); i++) {
        int peak_k = int(resonantPeaks[i].freq / fftStep) + 1;
        int start_k = resonantPeaks[i].fftCoeffs == 1 ? peak_k - 1 : peak_k - int(resonantPeaks[i].fftCoeffs / 2);
        int end_k = resonantPeaks[i].fftCoeffs == 1 ? peak_k : peak_k + int(resonantPeaks[i].fftCoeffs / 2);
        for (int j = start_k; j < end_k; j++) {
            float freq = j * fftStep;
            float fft = 0.0;
            //float freqDiff = freq-resonantPeaks[i].freq;
            float x = 2 * m_pi * resonantPeaks[i].freq * batchEndTime + m_pi * 0.5;// + m_pi* batchEndTime * freqDiff;
            float shiftSin = resonantPeaks[i].amplitudeShift * sin(x+resonantPeaks[i].phaseShift);
            float shiftCos = resonantPeaks[i].amplitudeShift * sin(x+resonantPeaks[i].phaseShift);
            fftEntry entry = {freq, fft, sin(x), cos(x), shiftSin, shiftCos};
            (*activeFFT).push_back(entry);
            (*passiveFFT).push_back(entry);
        }
    }
    runningFFTSize = (*activeFFT).size();
}

float FFTCorrection::sinc(float x) {
    if (x==0)
        return 1;
    return sin(x)/(x);
}

void FFTCorrection::GenerateFourierCoeffs() {
    for (int i = 0; i < resonantPeaks.size(); i++) {
        int peak_k = int(resonantPeaks[i].freq / fftStep) + 1;
        int start_k = resonantPeaks[i].fftCoeffs == 1 ? peak_k - 1 : peak_k - int(resonantPeaks[i].fftCoeffs / 2);
        int end_k = resonantPeaks[i].fftCoeffs == 1 ? peak_k : peak_k + int(resonantPeaks[i].fftCoeffs / 2);
        for (int j = start_k; j < end_k; j++) {
            complex<float> temp(0, 2 * m_pi * j / batchSize);
            fourierCoeffs.push_back(exp(temp));
        }
    }
}

void FFTCorrection::GenerateAmplitudeCorrection() {
    complex<float> c_i = complex <float> (0.0f, 1.0f);
    for (int i = 0; i < resonantPeaks.size(); i++) {
        float r = resonantPeaks[i].freq / fftStep;
        int r_r = int(resonantPeaks[i].freq / fftStep) + 1;
        int start_k = resonantPeaks[i].fftCoeffs == 1 ? r_r - 1 : r_r - int(resonantPeaks[i].fftCoeffs / 2);
        int end_k = resonantPeaks[i].fftCoeffs == 1 ? r_r : r_r + int(resonantPeaks[i].fftCoeffs / 2);
        for (int k = start_k; k < end_k; k++) {
            float diff_k = r-k;
            complex<float> corr = exp(- c_i * m_pi * diff_k) * sinc(m_pi * diff_k);
            amplitudeCorr.push_back(corr);
        }
    }
}

void FFTCorrection::Setup() {
	runningSignal = vector<float>(batchSize, 0.0);
    fftStep = sampleRate / batchSize;
    batchEndTime = (batchSize-1) / sampleRate;
    amplitudeNorm = 2.0 / batchSize;
    GenerateFFTBasis();
    GenerateFourierCoeffs();
    GenerateAmplitudeCorrection();
}

