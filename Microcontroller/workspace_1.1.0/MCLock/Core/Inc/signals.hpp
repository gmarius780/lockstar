// Signals for Output, returns Pointer to first element of the array created with 'new'
//#include <arm_math.h>
#include <math.h>

float* SineSignal(float sinemin, float sinemax, uint32_t length){
	float* sine_signal = new float[length];
	for(unsigned int i=0;i<length;i++){
		sine_signal[i] = 0.5 * (sinemin + sinemax) + 0.5*(sinemax-sinemin)*sin(float(i*6.28318530718)/float(length));
	}
	return sine_signal;
}

float* TriangleSignal(float trianglemin, float trianglemax, uint32_t length){
	float* triangle_signal = new float[length];
	for(unsigned int i=0;i<length;i++){
		triangle_signal[i] = (i<(length/2)) ? (trianglemin+i*(trianglemax-trianglemin)/(float((length/2)-1))) : (trianglemax+(i-length/2)*(trianglemin-trianglemax)/(float((length/2)-1))) ;
	}
	return triangle_signal;
}

float* RampSignal(float Rampmin, float Rampmax, uint32_t length){
	float* Ramp_signal = new float[length];
	for(unsigned int i=0;i<length;i++){
		Ramp_signal[i] = (Rampmin+i*(Rampmax-Rampmin)/(float((length)-1)));
	}
	return Ramp_signal;
}

// Return a single step of the signal, indexed at 0

float RampStep(uint32_t step, float Rampmin, float Rampmax, uint32_t length){
	return (Rampmin+step*(Rampmax-Rampmin)/(float((length)-1)));
}

float TriangleStep(uint32_t step, float trianglemin, float trianglemax, uint32_t length){
	return (step<(length/2)) ? (trianglemin+step*(trianglemax-trianglemin)/(float((length/2)-1))) : (trianglemax+(step-length/2)*(trianglemin-trianglemax)/(float((length/2)-1))) ;
}

float SineStep(uint32_t step, float sinemin, float sinemax, uint32_t length){
	return 0.5 * (sinemin + sinemax) + 0.5*(sinemax-sinemin)*sin(float(step*6.28318530718)/float(length));
}
