/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

// source code for Fourier-family of functions
#include "dy4.h"
#include "fourier.h"

// just DFT function (no FFT yet)
void DFT(const std::vector<float> &x, std::vector<std::complex<float>> &Xf) {
	Xf.resize(x.size(), static_cast<std::complex<float>>(0));
	for (unsigned int m = 0; m < Xf.size(); m++) {
		for (unsigned int k = 0; k < x.size(); k++) {
				std::complex<float> expval(0, -2*PI*(k*m) / x.size());
				Xf[m] += x[k] * std::exp(expval);
		}
	}
}

// function to compute the magnitude values in a complex vector
void computeVectorMagnitude(const std::vector<std::complex<float>> &Xf, std::vector<float> &Xmag)
{
	// only the positive frequencies
	Xmag.resize(Xf.size(), static_cast<float>(0));
 	for (unsigned int i = 0; i < Xf.size(); i++) {
		Xmag[i] = std::abs(Xf[i])/Xf.size();
	}
}

// add your own code to estimate the PSD

void estimatePSD(const std::vector<float> &samples, const int nfft, const float Fs, std::vector<float> &freq, std::vector<float> &psd_est){
  unsigned int freq_bins = nfft;
  float df = Fs/freq_bins;

  unsigned int freq_size = (int)(Fs/2/df);
  freq.clear();
  freq.resize(freq_size, 0.0);
  for (unsigned int i=0; i<freq_size; i++){
    freq[i] = i*df;
  }

  std::vector<float> hann;
  hann.clear();
  hann.resize(freq_bins, 0.0);
  for (unsigned int i=0; i<freq_bins; i++){
    hann[i] = std::pow(std::sin(i*PI/freq_bins), 2.0);
  }

  std::vector<float> psd_list;
  psd_list.clear();

  unsigned int no_segments = (int)(samples.size()*1.0/freq_bins);

  std::vector<float> windowed_samples;
  windowed_samples.clear();
  windowed_samples.resize(freq_bins, 0.0);

  for (unsigned int k=0; k<no_segments; k++){
    for (unsigned int j=0; j<windowed_samples.size(); j++){
      windowed_samples[j] = samples[k*freq_bins+j] * hann[j];
    }

    std::vector<std::complex<float>> Xf;
    Xf.clear();
    DFT(windowed_samples, Xf);
    Xf.resize(freq_bins/2, 0.0);

    std::vector<float> psd_seg;
    psd_seg.clear();
    psd_seg.resize(freq_bins/2, 0.0);

    for(unsigned int m=0; m<psd_seg.size(); m++){
      psd_seg[m] = (1.0/(Fs*freq_bins/2.0)) * (std::pow(std::abs(Xf[m]), 2.0));
      psd_seg[m] = 2.0*psd_seg[m];
      psd_seg[m] = 10.0*std::log10(psd_seg[m]);
      psd_list.push_back(psd_seg[m]);
    }

    psd_est.clear();
    psd_est.resize(freq_bins/2, 0.0);

    for (unsigned int k=0; k<psd_est.size(); k++){
      for (unsigned int l=0; l<no_segments; l++){
        psd_est[k] += psd_list[k + l*(freq_bins/2)];
      }
      psd_est[k] = psd_est[k]/no_segments;
    }

  }

}

//////////////////////////////////////////////////////

// added IDFT

void IDFT(const std::vector<std::complex<float>> &Xf, std::vector<std::complex<float>> &x) {
	x.resize(Xf.size(), static_cast<std::complex<float>>(0));
	for (unsigned int k = 0; k < x.size(); k++) {
		for (unsigned int m = 0; m < x.size(); m++) {
			std::complex<float> expval(0, 2*PI*(k*m) / Xf.size());
			x[k] += Xf[m] * std::exp(expval);
		}
		x[k] /= Xf.size();
	}
}

// added FFT

unsigned int swap_bits(unsigned int x, unsigned char i, unsigned char j) {

  unsigned char bit_i = (x >> i) & 0x1L;
  unsigned char bit_j = (x >> j) & 0x1L;

  unsigned int val = x;
  val = bit_i ? (val | (0x1L << j)) : (val & ~(0x1L << j));
  val = bit_j ? (val | (0x1L << i)) : (val & ~(0x1L << i));

  return val;
}

unsigned int bit_reversal(unsigned int x, unsigned char bit_size) {

  unsigned int val = x;

  for (int i=0; i < int(bit_size/2); i++)
    val = swap_bits(val, i, bit_size-1-i);

  return val;
}

void compute_twiddles(std::vector<std::complex<float>> &twiddles) {
  for (int k=0; k<(int)twiddles.size(); k++) {
      std::complex<float> expval(0.0, -2*PI*float(k)/ NFFT);
      twiddles[k] = std::exp(expval);
  }
}

void FFT_recursive(const std::vector<std::complex<float>> &x, \
  std::vector<std::complex<float>> &Xf) {

  if (x.size() > 1) {
    // declare vectors and allocate space for the even and odd halves
    std::vector<std::complex<float>> xe(int(x.size()/2)), xo(int(x.size()/2));
    std::vector<std::complex<float>> Xfe(int(x.size()/2)), Xfo(int(x.size()/2));

    // split into even and odd halves
    for (int k=0; k<(int)x.size(); k++)
      if ((k%2) == 0) xe[k/2] = x[k];
      else xo[k/2] = x[k];

    // call recursively FFT of half size for even and odd halves respectively
    FFT_recursive(xe, Xfe);
    FFT_recursive(xo, Xfo);

    // merge the results from the odd/even FFTs (each of half the size)
    for (int k=0; k<(int)xe.size(); k++) {
        std::complex<float> expval(0.0, -2*PI*float(k)/ x.size());
        std::complex<float> twiddle = std::exp(expval);
        Xf[k]           = Xfe[k] + twiddle * Xfo[k];
        Xf[k+xe.size()] = Xfe[k] - twiddle * Xfo[k];
    }
  } else {
    // end of recursion - copy time domain samples to frequency bins (default values)
    Xf[0] = x[0];
  }
}

void FFT_improved(const std::vector<std::complex<float>> &x, \
  std::vector<std::complex<float>> &Xf, \
  const std::vector<std::complex<float>> &twiddles, \
  const unsigned char recursion_level) {

  if (x.size() > 1) {
    int half_size = int(x.size()/2);
    std::vector<std::complex<float>> xe(half_size), xo(half_size);
    std::vector<std::complex<float>> Xfe(half_size), Xfo(half_size);

    for (int k=0; k<half_size; k++) {
      xe[k] = x[k*2];
      xo[k] = x[k*2+1];
    }

    FFT_improved(xe, Xfe, twiddles, recursion_level+1);
    FFT_improved(xo, Xfo, twiddles, recursion_level+1);

    for (int k=0; k<half_size; k++) {
        Xf[k]           = Xfe[k] + twiddles[k*(1<<(recursion_level-1))] * Xfo[k];
        Xf[k+half_size] = Xfe[k] - twiddles[k*(1<<(recursion_level-1))] * Xfo[k];
    }
  } else {
    Xf[0] = x[0];
  }
}

void FFT_optimized(const std::vector<std::complex<float>> &x, \
  std::vector<std::complex<float>> &Xf, \
  const std::vector<std::complex<float>> &twiddles) {

  unsigned char no_levels = (unsigned char)std::log2((float)x.size());
  for (unsigned int i=0; i<x.size(); i++) {
    Xf[i] = x[bit_reversal(i, no_levels)];
  }

  unsigned int step_size = 1;

  std::complex<float> tmp;
  for (unsigned char l=0; l<no_levels; l++) {
    for (unsigned int p=0; p<x.size(); p+=2*step_size) {
      for (unsigned int k=p; k<p+step_size; k++) {
        tmp             = Xf[k] + twiddles[(k-p)*(1<<(no_levels-1-l))] * Xf[k+step_size];
        Xf[k+step_size] = Xf[k] - twiddles[(k-p)*(1<<(no_levels-1-l))] * Xf[k+step_size];
        Xf[k]           = tmp;
      }
    }
    step_size *= 2;
  }
}

