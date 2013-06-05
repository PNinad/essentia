/*
 * Copyright (C) 2006-2008 Music Technology Group (MTG)
 *                         Universitat Pompeu Fabra
 *
 */

#include "lpc.h"
#include "essentiamath.h"
#include "algorithmfactory.h"

using namespace std;
using namespace essentia;
using namespace standard;

const char* LPC::name = "LPC";
const char* LPC::description = DOC("This algorithm computes the Linear Predictive Coefficients of a signal and the associated Reflection coefficients.\n"
"\n"
"An exception is thrown if the \"order\" provided is larger than the size of the input signal.\n"
"\n"
"References:\n"
"  [1] Linear predictive coding - Wikipedia, the free encyclopedia,\n"
"      http://en.wikipedia.org/wiki/Linear_predictive_coding\n"
"  [2] J. Makhoul, Spectral analysis of speech by linear prediction, IEEE Transactions on Audio and Electroacoustics, vol. 21, no. 3, pp. 140–148, 1973.\n");


void LPC::configure() {
  _P = parameter("order").toInt();

  delete _correlation;
  if (parameter("type").toString() == "warped") {
    _correlation = AlgorithmFactory::create("WarpedAutoCorrelation",
                                            "maxLag", _P+1);
    _correlation->output("warpedAutoCorrelation").set(_r);
  }
  else {
    _correlation = AlgorithmFactory::create("AutoCorrelation");
    _correlation->output("autoCorrelation").set(_r);
  }
}

void LPC::compute() {

  const vector<Real>& signal = _signal.get();
  vector<Real>& lpc = _lpc.get();
  vector<Real>& reflection = _reflection.get();

  if (_P > (int)signal.size()) {
    throw EssentiaException("LPC: you can't compute more coefficients than the size of your input");
  }

  if (isSilent(signal)) {
    lpc = vector<Real>(_P+1, 0.0);
    reflection = vector<Real>(_P, 0.0);
    return;
  }

  lpc.resize(_P+1);
  reflection.resize(_P);

  _correlation->input("array").set(signal);
  _correlation->compute();

  // Levinson-Durbin algorithm
  vector<Real> temp(_P);

  Real k;
  Real E = _r[0];
  lpc[0] = 1;

  for (int i=1; i<(_P+1); i++) {
    k = _r[i];

    for (int j=1; j<i; j++) {
      k += _r[i-j] * lpc[j];
    }

    k /= E;

    reflection[i-1] = k;
    lpc[i] = -k;

    for (int j=1; j<i; j++) {
      temp[j] = lpc[j] - k*lpc[i-j];
    }

    for (int j=1; j<i; j++) {
      lpc[j] = temp[j];
    }

    E *= (1-k*k);
  }
}