#ifndef SoluteStorage_h__
#define SoluteStorage_h__

#include "Solute.h"
#include "../math/StateVariable.h"
#include "../math/real.h"

//#include "Reaction.h"
namespace cmf {
	namespace water {
		class WaterStorage;
		//class SoluteStorageMap;
		
		/// A class for the storage of any tracer. The state is the amount (mol, kg etc. see cmf::water) 
		/// of the tracer in the storage.
		/// \f{eqnarray*}
		/// \frac{dX}{dt}&=&\sum_{f=1}^{F}\left( q_f [X]_f\right) + X_{in} + [X]_{in}V - r^-X \left[\frac{mol}{day}\right]\\
		/// F&=& \mbox{Number of fluxes in water storage} \\
		/// q_f&=& \mbox{Water flux in } \frac{m^3}{day}	\\
		/// \left[X\right]_f &=& \mbox{Concentration of solute X in flux }q_f \mbox{ in } \frac{mol}{m^3} \\
		/// X_{in} &=& \mbox{Absolute source or sink term} \frac{mol}{day} \\
		/// [X]_{in} &=& \mbox{Concentration source or sink term} \frac{mol}{m^3 day} \\
		/// r^- &=& \mbox{Decay rate} \frac 1{day} }}
		/// V &=& \mbox{Volume of water in water storage }\left[m^3\right]
		/// \f}
		class SoluteStorage : public cmf::math::StateVariable
		{
			SoluteStorage(std::tr1::weak_ptr<WaterStorage> _water,const cmf::water::solute& solute, double InitialState=0) 
				: cmf::math::StateVariable(InitialState),m_water(_water), Solute(solute),decay(0),source(0),source_concentration(0) {}
			std::tr1::weak_ptr<WaterStorage> m_water;
			
		public:
			friend class WaterStorage;
			/// Rate of decay of the solute (in 1/day)
			real decay;
			/// A source or sink term of the solute as a concentration in state unit/(m3H2O day)
			real source_concentration;
			/// A source or sink term of the solute as an absolute matter flux in state unit/day
			real source;
			/// The water storage to which the concentration storage belongs
			std::tr1::shared_ptr<cmf::water::WaterStorage> get_water() const {return m_water.lock();}
			/// The solute, which is stored in this
			const cmf::water::solute& Solute;
			/// Returns the concentration of the solute
			real conc() const;
			virtual real Derivate(const cmf::math::Time& time);
		};
	}
}

#endif // SoluteStorage_h__
