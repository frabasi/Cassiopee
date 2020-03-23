/*
 
 
 
              NUGA 
 
 
 
 */

#ifndef NUGA_XCELLN_HXX
#define NUGA_XCELLN_HXX

#include "Nuga/include/classifyer.hxx"

namespace NUGA
{
  ///
  template<typename zmesh_t, typename bound_mesh_t = typename NUGA::boundary_t<zmesh_t>>
  class xcellnv : public classifyer<XCELLN_VAL, zmesh_t, bound_mesh_t>
  {
  public:
    using parent_t = classifyer<XCELLN_VAL, zmesh_t, bound_mesh_t>;
    using wdata_t = typename parent_t::wdata_t;
    using outdata_t = typename parent_t::outdata_t;

    xcellnv(double RTOL) : parent_t(RTOL){}

    outdata_t __process_X_cells(zmesh_t const & z_mesh, std::vector< bound_mesh_t*> const & mask_bits, wdata_t & wdata)
    {
      //todo
      return outdata_t();
    };

    
  };
}
#endif