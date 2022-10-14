/*



--------- NUGA v1.0



*/
//Authors : Sâm Landier (sam.landier@onera.fr)

#include "Nuga/include/Triangulator.h"
#include "Nuga/include/T3Mesher.h"
#include "Nuga/include/MeshTool.h"
#include "Nuga/include/IdTool.h"
#include "Nuga/include/FittingBox.h"
#include "Nuga/include/GeomAlgo.h"

#ifdef DEBUG_TRIANGULATOR
#include "IO/io.h"
#endif

#ifdef FLAG_STEP
#include "chrono.h"
#endif

#ifdef DEBUG_TRIANGULATOR
    bool DELAUNAY::Triangulator::dbg_enabled = false;
#endif

using namespace NUGA;

#define Vector_t std::vector
namespace DELAUNAY
{
  Triangulator::Triangulator()
  {
#ifdef FLAG_STEP
    tcreate = tconnect = trun = ttra = 0;
#endif
#ifdef DEBUG_TRIANGULATOR
    dbg_enabled = false;
#endif

    auto& mode = _mesher.mode;
    mode.mesh_mode = mode.TRIANGULATION_MODE;
    mode.silent_errors = true;

  }
  
#ifdef NETBEANSZ
inline 
#endif
E_Int Triangulator::run
(const K_FLD::FloatArray& coord, const E_Int* pNodes, E_Int nb_nodes, E_Int index_start, 
       K_FLD::IntArray& connectM, K_FLD::IntArray& neighbors, bool do_not_shuffle, bool improve_quality) const 
{
   //OVERWRITE mesh upon exit.
#ifdef FLAG_STEP
  NUGA::chrono c;
  c.start();
#endif
  
  connectE2.clear();
  connectE2b.clear();
  __set_connectE2(pNodes, nb_nodes, connectE2, index_start);
  
#ifdef FLAG_STEP
  tconnect +=c.elapsed();
  c.start();
#endif
  
  Wpos.clear();
  oldIds.clear();
  NUGA::MeshTool::compact_to_mesh(coord, connectE2, Wpos, connectE2b, oldIds);
  
#ifdef FLAG_STEP
  tcompact +=c.elapsed();
  c.start();
#endif

  if (Wpos.rows() >= 3) // fixme : if more than 3, it contains soluiton fields : are we sure the first 3 are x,y,z ?
  {
    // Computes the fitting box coordinate system optimizing the view over the contour
    K_FLD::FloatArray P(3, 3), iP(3, 3);
    E_Float W[3];
    FittingBox::computeNormalToContour(Wpos, connectE2b, W);
    NUGA::computeAFrame(W, P);
    iP = P;
    K_FLD::FloatArray::inverse3(iP);
    NUGA::transform(Wpos, iP);// Now we are in the fitting coordinate system.

    Wpos.resize(2, Wpos.cols()); // Wpos is 2D now.
  }
  
#ifdef FLAG_STEP
  ttra +=c.elapsed();
  c.start();
#endif
  
  _data.set(Wpos, connectE2b);
  
#ifdef FLAG_STEP
  tcreate +=c.elapsed();
  c.start();
#endif
  
#if defined(DEBUG_TRIANGULATOR) && defined(DEBUG_MESHER)
  if (dbg_enabled)
    mesher.dbg_flag=true;
#endif

  _mesher.mode.do_not_shuffle = do_not_shuffle;
  _mesher.seed_random(1);
  E_Int err = _mesher.run(_data);
  
  if (!err && (_data.connectM.cols() == 0) && (_data.connectB->cols() != 0))
    err = 1;

  if (err)
  {
#ifdef DEBUG_TRIANGULATOR
      medith::write("data0", coord, connectE2, "BAR");
      medith::write("dataT", Wpos, connectE2b, "BAR");
#endif
      return err;
  }
  
  // In case we need to improve the output quality by swapping.
  if (improve_quality) // fixme : means specified (what to do for relative tol is given as negative ?)
  {
#ifdef DEBUG_TRIANGULATOR
    if (dbg_enabled)
    {
      K_FLD::FloatArray crd(*data.pos);
      crd.resize(3,crd.cols(), 0.);
      medith::write("before_swap", crd, data.connectM, "TRI");
    }
#endif
    E_Float quality_tol = 1.e-4;// EPSILON;
    E_Int railing =_data.connectM.cols();
    while (--railing)
    {
      _swapE.clear();
      
      // also swap poor quality triangles
      NUGA::GeomAlgo<K_MESH::Triangle>::get_swapE(*_data.pos, _data.connectM, _data.neighbors, _data.hardEdges, quality_tol, _swapE); //warning : quality<2> doesnt give the same result as quality<3>. need to convert to tolerance criterium.

      if (!_swapE.empty())
        NUGA::EltAlgo<K_MESH::Triangle>::fast_swap_edges(_swapE, _data.connectM, _data.neighbors);
      else
        break;
    }

#ifdef DEBUG_TRIANGULATOR
    if (dbg_enabled)
    {
      {
      K_FLD::FloatArray crd(*data.pos);
      crd.resize(3,crd.cols(), 0.);
      medith::write("after_swap", crd, data.connectM, "TRI");
      }
    }
#endif
  }
  
#ifdef FLAG_STEP
  trun +=c.elapsed();
  c.start();
#endif
  
  K_FLD::IntArray::changeIndices(_data.connectM, oldIds);//Back to original ids
  
  connectM  = _data.connectM;
  neighbors = _data.neighbors;

  return 0;
}
  


#ifdef NETBEANSZ
inline 
#endif
E_Int Triangulator::run
(const K_FLD::FloatArray& coord, const E_Int* pNodes, E_Int nb_nodes, E_Int index_start, K_FLD::IntArray& connectM, bool do_not_shuffle, bool improve_quality) const 
{
  //APPENDING mesh upon exit.
  K_FLD::IntArray cM, neighbors;
  E_Int err = run(coord, pNodes, nb_nodes, index_start, cM, neighbors, do_not_shuffle, improve_quality);
  
  if (!err)
    connectM.pushBack(cM);
  
  return err;
}

///
#ifdef NETBEANSZ
inline 
#endif
E_Int Triangulator::__set_connectE2
(const E_Int* pNodes, E_Int nb_nodes, K_FLD::IntArray& connectE2, E_Int index_start)
{
  connectE2.clear();
  connectE2.reserve(2, nb_nodes);
  
  E_Int E[2];
  const E_Int* p = pNodes;
  for (E_Int i = 0; i < nb_nodes-1; ++i, ++p)
  {
    E[0]=(*p)-index_start;
    E[1]=*(p+1)-index_start;
    connectE2.pushBack(&E[0], &E[0]+2);
  }
  
  E[0]=(*p)-index_start;
  E[1]=(*pNodes)-index_start;
  connectE2.pushBack(&E[0], &E[0]+2);
  
  return 0;
}

}
