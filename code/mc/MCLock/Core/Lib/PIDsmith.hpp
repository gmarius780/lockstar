

class PIDsmith {

	public:
		PIDsmith(float,float,float,int,float,float);
		void setPIDParameter(float p,float i ,float d) { Kp=p; Ki=i; Kd=d; };
		float calcControlOutput(float,float,float);
		void calcModelOutput(float);
		int setModelParameter(const float*,const float*,const int);
		float getModelOutput(int);
		void setModelOffset(float o) { modelOffset = o; };
		float getModelOffset() { return modelOffset; };

	private:

		// PID variables
		float integral;
		float error;
		float oldError;
		float dError;
		float Kp;
		float Ki;
		float Kd;
		float dt;
		float controlOutput;
		float modelOffset;

		// Smith Predictor variables
		int deadtime;
		int modelOrder;
		int inputBufferLength;
		int outputBufferLength;
		int inputBufferPointer;
		int outputBufferPointer;

		float* inputBuffer;
		float* outputBuffer;

		float* A;
		float* B;

		bool debug;

};
