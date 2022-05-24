

class PIDnew {

	private:
		float integral;
		float error;
		float oldError;
		float Kp;
		float Ki;
		float Kd;
		float dt;

	public:
		PIDnew(float,float,float);

		void setPIDParameters(float p,float i ,float d) { Kp = p; Ki = i; Kd = d; };
		float calcControlOutput(float,float,float);
};
