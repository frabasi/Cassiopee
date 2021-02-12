/*



--------- NUGA v1.0



*/
//Authors : S�m Landier (sam.landier@onera.fr)

#include "Nuga/include/adapt_cells.h"
#include "Nuga/include/Basic.h"
#include "Nuga/include/hierarchical_mesh.hxx"
#include "Nuga/include/geom_sensor.hxx"

namespace NUGA
{


  int adapt_cells(c_phmesh_t& m, const c_crd3D_t& src_pts)
  {
    using ELT_t = K_MESH::Basic;
    using hmesh_t = hierarchical_mesh<ELT_t, ISO, ngon_type>;
    using sensor_t = NUGA::geom_sensor<hmesh_t, c_phmesh_t::crd_t>;

    K_FLD::FloatArray crd(m.crd.p, 3, m.crd.n, (m.CALLOC == 1));

    ngon_type ng(
      ngon_unit(m.pgs.elts, m.pgs.range, m.pgs.nrange), // PGS is moved
      ngon_unit(m.phs.elts, m.phs.range, m.phs.nrange)  // PHs is moved
    );

    hmesh_t hmesh(crd, ng);

    sensor_t gsensor(hmesh, eSmoother::SHELL, 1/*max_pts_per_cell*/, 10/*itermax*/);

    int err = gsensor.assign_data(src_pts);

    adaptor<hmesh_t, sensor_t>::run(hmesh, gsensor);

    std::vector<E_Int> oids;
    ngon_type ngo;

    hmesh.conformize(ngo, oids);

    int dim{ 3 };
    bool calloc{ false };
    hmesh._crd.relay_mem(m.crd.p, dim, m.crd.n, calloc);
    assert((E_Int)calloc == m.crd.CALLOC);
    ngo.PHs.relay_mem(m.phs);
    ngo.PGs.relay_mem(m.pgs);

    return 0;
  }

}
