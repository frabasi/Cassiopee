/*
 
 
 
              NUGA 
 
 
 
 */
//Authors : Sâm Landier (sam.landier@onera.fr), Alexis Rouil (alexis.rouil@onera.fr)

#ifndef __K_MESH_BASIC_H
#define __K_MESH_BASIC_H

#include "MeshElement/Quadrangle.h"
#include "MeshElement/Hexahedron.h"
#include "MeshElement/Tetrahedron.h"
#include "MeshElement/Pyramid.h"
#include "MeshElement/Prism.h"


#include "Fld/DynArray.h"
#include "Fld/ArrayAccessor.h"
#include "Fld/ngon_t.hxx"
#include "Def/DefTypes.h"

namespace K_MESH
{

class Basic {
public: 
    static constexpr E_Int NB_BOUNDS=6;//fixme

    //
    template< typename ngo_t>
    static void reorder_pgs(ngo_t& ng, const K_FLD::IntArray& F2E, E_Int i);
    //
    template <typename ngunit_t>
    static inline void iso_barycenter(const K_FLD::FloatArray& crd, const ngunit_t & PGs, const E_Int* first_pg, E_Int nb_pgs, E_Int index_start, E_Float* G);
    //
    E_Float quality(const K_FLD::FloatArray& crd, E_Float* Vol){return 1;}
};

///
template< typename ngo_t>
void Basic::reorder_pgs(ngo_t& ng, const K_FLD::IntArray& F2E, E_Int i)
{
  if (Polyhedron<0>::is_HX8(ng.PGs, ng.PHs.get_facets_ptr(i), ng.PHs.stride(i))){
    Hexahedron::reorder_pgs(ng, F2E, i);
  }
  else if (Polyhedron<0>::is_TH4(ng.PGs, ng.PHs.get_facets_ptr(i), ng.PHs.stride(i))){
    Tetrahedron::reorder_pgs(ng, F2E, i);
  }
  else if (Polyhedron<0>::is_PY5(ng.PGs, ng.PHs.get_facets_ptr(i), ng.PHs.stride(i))){
    Pyramid::reorder_pgs(ng, F2E, i);
  }
  else if (Polyhedron<0>::is_PR6(ng.PGs, ng.PHs.get_facets_ptr(i), ng.PHs.stride(i))){
    Prism::reorder_pgs(ng, F2E, i);
  }           
}

///
template <typename ngunit_t>
void Basic::iso_barycenter(const K_FLD::FloatArray& crd, const ngunit_t & PGs, const E_Int* first_pg, E_Int nb_pgs, E_Int index_start, E_Float* G)
{    
  if (Polyhedron<0>::is_HX8(PGs, first_pg, nb_pgs)){
    Hexahedron::iso_barycenter(crd, PGs, first_pg, nb_pgs, index_start, G);
  }
  else if (Polyhedron<0>::is_TH4(PGs, first_pg, nb_pgs)){
    Tetrahedron::iso_barycenter(crd, PGs, first_pg, nb_pgs, index_start, G);
  }
  else if (Polyhedron<0>::is_PY5(PGs, first_pg, nb_pgs)){
    Pyramid::iso_barycenter(crd, PGs, first_pg, nb_pgs, index_start, G);
  }
  else if (Polyhedron<0>::is_PR6(PGs, first_pg, nb_pgs)){
    Prism::iso_barycenter(crd, PGs, first_pg, nb_pgs, index_start, G);
  }
}

}
#endif /* BASIC_H */

