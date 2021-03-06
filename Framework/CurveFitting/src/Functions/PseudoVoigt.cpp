// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/PseudoVoigt.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidKernel/make_unique.h"

#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Constraints;

using namespace API;

DECLARE_FUNCTION(PseudoVoigt)

void PseudoVoigt::functionLocal(double *out, const double *xValues,
                                const size_t nData) const {
  double h = getParameter("Height");
  double x0 = getParameter("PeakCentre");
  double f = getParameter("FWHM");

  double gFraction = getParameter("Mixing");
  double lFraction = 1.0 - gFraction;

  // Lorentzian parameter gamma...fwhm/2
  double g = f / 2.0;
  double gSquared = g * g;

  // Gaussian parameter sigma...fwhm/(2*sqrt(2*ln(2)))...gamma/sqrt(2*ln(2))
  double sSquared = gSquared / (2.0 * M_LN2);

  for (size_t i = 0; i < nData; ++i) {
    double xDiffSquared = (xValues[i] - x0) * (xValues[i] - x0);

    out[i] = h * (gFraction * exp(-0.5 * xDiffSquared / sSquared) +
                  (lFraction * gSquared / (xDiffSquared + gSquared)));
  }
}

void PseudoVoigt::functionDerivLocal(Jacobian *out, const double *xValues,
                                     const size_t nData) {

  double h = getParameter("Height");
  double x0 = getParameter("PeakCentre");
  double f = getParameter("FWHM");

  double gFraction = getParameter("Mixing");
  double lFraction = 1.0 - gFraction;

  // Lorentzian parameter gamma...fwhm/2
  double g = f / 2.0;
  double gSquared = g * g;

  // Gaussian parameter sigma...fwhm/(2*sqrt(2*ln(2)))...gamma/sqrt(2*ln(2))
  double sSquared = gSquared / (2.0 * M_LN2);

  for (size_t i = 0; i < nData; ++i) {
    double xDiff = (xValues[i] - x0);
    double xDiffSquared = xDiff * xDiff;

    double expTerm = exp(-0.5 * xDiffSquared / sSquared);
    double lorentzTerm = gSquared / (xDiffSquared + gSquared);

    out->set(i, 0, h * (expTerm - lorentzTerm));
    out->set(i, 1, gFraction * expTerm + lFraction * lorentzTerm);
    out->set(i, 2,
             h * xDiff *
                 (gFraction * expTerm / sSquared +
                  lFraction * lorentzTerm * 2.0 / (xDiffSquared + gSquared)));
    out->set(i, 3,
             h * (gFraction * expTerm * xDiffSquared / sSquared / f +
                  lFraction * lorentzTerm *
                      (1.0 / g - g / (xDiffSquared + gSquared))));
  }
}

void PseudoVoigt::init() {
  declareParameter("Mixing", 1.0);
  declareParameter("Height");
  declareParameter("PeakCentre");
  declareParameter("FWHM");

  auto mixingConstraint =
      Kernel::make_unique<BoundaryConstraint>(this, "Mixing", 0.0, 1.0, true);
  mixingConstraint->setPenaltyFactor(1e9);

  addConstraint(std::move(mixingConstraint));
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
