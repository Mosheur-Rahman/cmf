

// Copyright 2010 by Philipp Kraft
// This file is part of cmf.
//
//   cmf is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   cmf is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with cmf.  If not, see <http://www.gnu.org/licenses/>.
//   
#ifndef real_h__
#define real_h__

#include <string>
#include <limits>
typedef double real;
#ifndef SWIG
const real REAL_MAX = std::numeric_limits<real>::max();
#endif

const std::string __compiledate__ = std::string("cmf compiled ") + std::string(__DATE__) + " - " + std::string(__TIME__);

// Some helper functions
/// Returns the minimum of two values
real minimum(real a,real b);
real maximum(real a,real b);
real minmax(real x,real min,real max);
real mean(real a,real b);
real geo_mean(real a,real b);
real harmonic_mean(real a,real b);
real piecewise_linear(real x,real xmin,real xmax,real ymin=0,real ymax=1);

/// The boltzmann function, used in cmf at several places where a s-shaped curve is needed
///
/// \f[f(x,x_{1/2},\tau)=\frac{1}{1+e^{-\frac{x-x_{1/2}}{tau}}}\f]
real boltzmann(real x,real x_half,real tau);
real sign(real x);
real square(real x);

const real Pi=3.141592654;

namespace cmf {
    typedef std::string bytestring;

    /// @brief Protects diffusive St. Venant equation from numerical problems for slope ≃ 0
    ///
    /// For diffusive St. Venant equations, near to zero, the driving force is sqrt(|h1-h2|).
    /// For h1 ≃ h2, the sensitivity of the flow has a singularity. To avoid this
    /// near to the slope zero the driver is overrriden by |h1-h2|. "Near" is defined by this constant.
    extern real diffusive_slope_singularity_protection;

    /// @brief Allows the cmf::upslope::connections::Richards_lateral connection for faster flow in lower regions.
    extern bool richards_lateral_base_flow;


}


#endif // real_h__
