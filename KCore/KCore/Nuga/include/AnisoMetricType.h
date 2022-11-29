/*



--------- NUGA v1.0



*/
//Authors : S�m Landier (sam.landier@onera.fr)

#ifndef __ANISO_METRIC_TYPE_H__
#define __ANISO_METRIC_TYPE_H__

#include "Nuga/include/defs.h"

namespace DELAUNAY{

template <short DIM>
class AnisoMetricType
{
public:
  typedef NUGA::size_type            size_type;
  typedef AnisoMetricType<DIM>       self_type;
  #define                            DIMANISO DIM*(DIM+1) / 2

public:

  explicit AnisoMetricType(void)               {for (E_Int k = 0; k < DIMANISO; ++k)_mij[k] = 0.;}
  explicit AnisoMetricType(E_Float h){ _mij[0] = _mij[2] = 1./(h*h);_mij[1] = 0.;} // fixme : 2D only
  explicit AnisoMetricType(const E_Float* m) { _mij[0] = m[0]; _mij[1] = m[1]; _mij[2] = m[2];} // fixme : 2D only

  ~AnisoMetricType(void){}

  inline E_Float operator[](size_type i) const {assert (i < DIMANISO); return _mij[i];}
  inline E_Float& operator[](size_type i) {assert (i < DIMANISO); return _mij[i];}
  inline self_type& operator=(const self_type& m){for (E_Int k = 0; k < DIMANISO; ++k)_mij[k] = m[k]; return *this;}
  inline self_type& operator=(const E_Float* mvals){for (E_Int k = 0; k < DIMANISO; ++k)_mij[k] = mvals[k]; return *this;}

  inline self_type operator*(const E_Float& a) const;
  //inline self_type operator*(E_Float* v) const;


  AnisoMetricType operator+(const AnisoMetricType&) const;

 void eigen_values(E_Float &lmax, E_Float & lmin) const;

#ifndef DEBUG_METRIC
  private:
#else
  public:
#endif
  E_Float                 _mij[DIM*(DIM+1) / 2]; // 2D -> 3, 3D -> 6

};


using Aniso2D = AnisoMetricType<2>;
using Aniso3D = AnisoMetricType<3>;

///
template <short DIM>
AnisoMetricType<DIM>
AnisoMetricType<DIM>::operator*(const E_Float& a) const {

  self_type res(*this);
  for (E_Int i = 0; i < DIMANISO; ++i)
    res._mij[i] *= a;

  return res;
}

///
template <short DIM>
AnisoMetricType<DIM>
AnisoMetricType<DIM>::operator+(const AnisoMetricType& rhs) const
{
  self_type res(*this);
   for (E_Int i = 0; i < DIMANISO; ++i)
     res._mij[i] += rhs._mij[i];

   return res;
}

template <> inline
void AnisoMetricType<2>::eigen_values(E_Float &lmax, E_Float & lmin) const
{
  E_Float a = _mij[0] + _mij[2];                 //trace
  E_Float b = _mij[0]*_mij[2] - _mij[1]*_mij[1]; //det
  E_Float d = a*a - 4.*b;

  d = (d > 0.) ? ::sqrt(d) : 0.;
  lmin = 0.5*(a - d);
  lmax = lmin+d;
}

template <> inline
void AnisoMetricType<3>::eigen_values(E_Float &lmax, E_Float & lmin) const
{
  //todo Imad : fixme : asssume here a diagonal matrix!
  lmin = std::min(_mij[0], std::min(_mij[3], _mij[5]));
  lmax = std::max(_mij[0], std::max(_mij[3], _mij[5]));
}

#ifdef DEBUG_METRIC
inline std::ostream&
 operator<<(std::ostream& os, const DELAUNAY::AnisoMetricType<2>& m)  
{  
  os << m._mij[0] << '/' << m._mij[1] << '/' << m._mij[2];  
  return os;  
}  
#endif

}

#endif
