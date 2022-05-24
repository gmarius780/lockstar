

class PIDsmith {

	private:

		// PID variables
		float integral;
		float error;
		float oldError;
		float Kp;
		float Ki;
		float Kd;
		float dt;

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


	public:
		PIDsmith(float,float,float,int,float,float);
		void setPIDParameter(float p,float i ,float d) { Kp=p; Ki=i; Kd=d; };
		float calcControlOutput(float,float,float);
		float calcModelOutput(float);
		int setModelParameter(float*,float*,int);
		float getLatestModelOutput() { return outputBuffer[(outputBufferPointer+outputBufferLength-1)%outputBufferLength]; };
};
