//
//  FFTCorrection.cpp
//  c_version
//
//  Created by Philip Leindecker on 04.06.20.
//  Copyright © 2020 Philip Leindecker. All rights reserved.
//

#ifdef OLD

#include "FFTCorrection.hpp"
#include <math.h>
#include <cmath>

float FFTCorrection::CalculateCorrection(float pv) {

    float lastBatchSignalX = runningSignal[index];
    runningSignal[index] = pv;

    float e = pv - lastBatchSignalX;
    float pvOut = pv;

    for (int i = 0; i < runningFFT.size(); i++) {
        // Running FFT
        // X_k = exp(i*2*π*k/N) * (X_k - x_m + x_{N+m})
        // X_k = fourierCoeffs[k] * (previousFFT[k].fft + pv - lastBatchSignalX)
        // fourierCoeffs=(a+bi); previousFFT=(c+di); (pv - lastBatchSignalX)=e;
        // X_k = (a*c-b*d+a*e) + (b*c+a*d+b*e)i
        complex<float> coeff = fourierCoeffs[i];
        float real = coeff.real() * runningFFT[i].fft.real() - coeff.imag() * runningFFT[i].fft.imag() + coeff.real() * e;
        float imag = coeff.imag() * runningFFT[i].fft.real() +  coeff.real() * runningFFT[i].fft.imag() + coeff.imag() * e;
        complex<float> temp(real, imag);
        runningFFT[i].fft = temp;

        // Generate output signal
        // sin(x+y) = sin(x)*cos(y) + cos(x)*sin(y)
        // arg(a+bi) = atan2(b, a)
        // r = sqrt(x*x+y*y)
        // cos(atan2(y,x)) = x/r
        // sin(atan2(y,x)) = y/r
        float amplitude = 2.0 / batchSize;
        //pvOut -= amplitude * (runningFFT[i].supportSin * runningFFT[i].fft.real() + runningFFT[i].supportCos * runningFFT[i].fft.imag());
        float temp2 = amplitude * (runningFFT[i].supportSin * runningFFT[i].fft.real() + runningFFT[i].supportCos * runningFFT[i].fft.imag());
    }

    index += 1;
    if (index == batchSize) {
        index = 0;
    }

    return pv;
}

void FFTCorrection::GenerateFFTBasis() {
    vector<fftEntry> fftBasis;
    for (int i = 0; i < resonantPeaks.size(); i++) {
        int peak_k = int(resonantPeaks[i].freq / fftStep) + 1;
        for (int j = peak_k - int(supportingFreqRange / 2); j < peak_k + int(supportingFreqRange / 2); j++) {
            float freq = j * fftStep;
            float fft = 0.0;
            float x = 2 * M_PI * resonantPeaks[i].freq * batchEndTime + M_PI * (0.5 + batchEndTime * (freq-resonantPeaks[i].freq));
            float supportSin = sin(x);
            float supportCos = cos(x);

            fftBasis.push_back({freq, fft, supportSin, supportCos});
        }
    }
    runningFFT = fftBasis;
}

void FFTCorrection::GenerateFourierCoeffs() {
    vector<complex<float>> coeff;
    for (int i = 0; i < resonantPeaks.size(); i++) {
        int peak_k = int(resonantPeaks[i].freq / fftStep) + 1;
        for (int j = peak_k - int(supportingFreqRange / 2); j < peak_k + int(supportingFreqRange / 2); j++) {
            complex<float> temp(0, 2 * M_PI * j / batchSize);
            coeff.push_back(exp(temp));
        }
    }
    fourierCoeffs = coeff;
}

void FFTCorrection::Setup() {
    fftStep = int(sampleRate / batchSize);
    batchEndTime = float(batchSize) / float(sampleRate);
    GenerateFFTBasis();
    GenerateFourierCoeffs();
}

#endif
