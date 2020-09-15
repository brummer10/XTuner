// generated from file '../src/LV2/faust/low_high_cut.dsp' by dsp2cc:
// Code generated with Faust (https://faust.grame.fr)

#include <algorithm>

template<class T> inline T mydsp_faustpower2_f(T x) {return (x * x);}
#define always_inline inline __attribute__((always_inline))

namespace low_high_cut {

class Dsp {
private:
	uint32_t fSampleRate;
	double fConst0;
	double fConst1;
	double fConst2;
	double fConst3;
	double fConst4;
	double fConst5;
	double fConst6;
	int iVec0[2];
	double fRec4[2];
	double fVec1[2];
	double fConst7;
	double fRec3[2];
	double fRec2[2];
	double fConst8;
	double fConst9;
	double fRec1[3];
	double fConst10;
	double fRec0[3];

	void clear_state_f();
	void init(uint32_t sample_rate);
	void compute(int count, float *input0, float *output0);

public:
	Dsp();
	~Dsp();
	static void clear_state_f_static(Dsp*);
	static void init_static(uint32_t sample_rate, Dsp*);
	static void compute_static(int count, float *input0, float *output0, Dsp*);
	static void del_instance(Dsp *p);
};



Dsp::Dsp() {
}

Dsp::~Dsp() {
}

inline void Dsp::clear_state_f()
{
	for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) iVec0[l0] = 0;
	for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) fRec4[l1] = 0.0;
	for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) fVec1[l2] = 0.0;
	for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) fRec3[l3] = 0.0;
	for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) fRec2[l4] = 0.0;
	for (int l5 = 0; (l5 < 3); l5 = (l5 + 1)) fRec1[l5] = 0.0;
	for (int l6 = 0; (l6 < 3); l6 = (l6 + 1)) fRec0[l6] = 0.0;
}

void Dsp::clear_state_f_static(Dsp *p)
{
	p->clear_state_f();
}

inline void Dsp::init(uint32_t sample_rate)
{
	fSampleRate = sample_rate;
	fConst0 = std::min<double>(192000.0, std::max<double>(1.0, double(fSampleRate)));
	fConst1 = std::tan((3138.4510609362032 / fConst0));
	fConst2 = (1.0 / fConst1);
	fConst3 = (1.0 / (((fConst2 + 0.76536686473017945) / fConst1) + 1.0));
	fConst4 = (1.0 / (((fConst2 + 1.8477590650225735) / fConst1) + 1.0));
	fConst5 = (72.256631032565238 / fConst0);
	fConst6 = (1.0 / (fConst5 + 1.0));
	fConst7 = (1.0 - fConst5);
	fConst8 = (((fConst2 + -1.8477590650225735) / fConst1) + 1.0);
	fConst9 = (2.0 * (1.0 - (1.0 / mydsp_faustpower2_f(fConst1))));
	fConst10 = (((fConst2 + -0.76536686473017945) / fConst1) + 1.0);
	clear_state_f();
}

void Dsp::init_static(uint32_t sample_rate, Dsp *p)
{
	p->init(sample_rate);
}

void always_inline Dsp::compute(int count, float *input0, float *output0)
{
	for (int i = 0; (i < count); i = (i + 1)) {
		iVec0[0] = 1;
		fRec4[0] = ((9.9999999999999995e-21 * double((1 - iVec0[1]))) - fRec4[1]);
		double fTemp0 = (double(input0[i]) + fRec4[0]);
		fVec1[0] = fTemp0;
		fRec3[0] = (fConst6 * ((fTemp0 - fVec1[1]) + (fConst7 * fRec3[1])));
		fRec2[0] = (fConst6 * ((fRec3[0] - fRec3[1]) + (fConst7 * fRec2[1])));
		fRec1[0] = (fRec2[0] - (fConst4 * ((fConst8 * fRec1[2]) + (fConst9 * fRec1[1]))));
		fRec0[0] = ((fConst4 * (fRec1[2] + (fRec1[0] + (2.0 * fRec1[1])))) - (fConst3 * ((fConst10 * fRec0[2]) + (fConst9 * fRec0[1]))));
		output0[i] = float((fConst3 * (fRec0[2] + (fRec0[0] + (2.0 * fRec0[1])))));
		iVec0[1] = iVec0[0];
		fRec4[1] = fRec4[0];
		fVec1[1] = fVec1[0];
		fRec3[1] = fRec3[0];
		fRec2[1] = fRec2[0];
		fRec1[2] = fRec1[1];
		fRec1[1] = fRec1[0];
		fRec0[2] = fRec0[1];
		fRec0[1] = fRec0[0];
	}
}

void Dsp::compute_static(int count, float *input0, float *output0, Dsp *p)
{
	p->compute(count, input0, output0);
}


void Dsp::del_instance(Dsp *p)
{
	delete p;
}

}

