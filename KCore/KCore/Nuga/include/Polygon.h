/*    
    Copyright 2013-2020 Onera.

    This file is part of Cassiopee.

    Cassiopee is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cassiopee is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cassiopee.  If not, see <http://www.gnu.org/licenses/>.
*/
//Author : Sâm Landier (sam.landier@onera.fr)

#ifndef __K_MESH_POLYGON_H__
#define __K_MESH_POLYGON_H__

#include "Nuga/include/Edge.h"
#include "Nuga/include/DynArray.h"
#include "Nuga/include/ArrayAccessor.h"
#include "Nuga/include/IdTool.h"
#include "Nuga/include/Triangle.h"
#include "Nuga/include/ngon_unit.h"
#include <deque>
#include "Nuga/include/subdiv_defs.h"
#include "Nuga/include/macros.h"

namespace K_MESH
{

class Polygon {
 
public:
  typedef K_MESH::NO_Edge boundary_type;
  typedef K_FLD::ArrayAccessor<K_FLD::FloatArray> aDynCrd_t;
    
public:
  ///
  //Polygon(E_Int shift=0):_shift(shift),_nodes(0){}
  ///
  Polygon(const E_Int* nodes, E_Int nb_nodes, E_Int shift=0):_nb_nodes(nb_nodes), _nodes(nodes), _shift(shift), _triangles(nullptr){}

  Polygon(const ngon_unit& ngu, E_Int ith):_nb_nodes(ngu.stride(ith)), _nodes(ngu.get_facets_ptr(ith)), _shift(-1), _triangles(nullptr){}
  ///
  ~Polygon(){if (_triangles != nullptr) delete [] _triangles;}

  // templated to use any polygon in the hierarchy (rather than upcasting when called from down)
  template <typename PolyG_t> Polygon(PolyG_t&& r): _nb_nodes(r._nb_nodes), _nodes(r._nodes), _shift(r._shift), _triangles(r._triangles)/* = default  old intel (15) reject it*/
  {
    r._triangles = nullptr;//stolen
  }

  ///
  //void setNodes(const E_Int* nodes, E_Int nb_nodes){NB_NODES=nb_nodes; for (size_t i=0; i < nb_nodes; ++i)_nodes[i]=nodes[i];}
  inline E_Int node(E_Int i) const {return _nodes[i]+_shift;}
  ///
  inline E_Int nb_nodes() const { return _nb_nodes;}
  
  inline E_Int shift() const { return _shift;}

  inline E_Int nb_tris() const { return (_triangles != nullptr) ? _nb_nodes - 2 : 0;}
  ///
  const E_Int* begin() const{ return &_nodes[0];}
  ///
  template <typename ConnectivityAcc>
  inline void set(const ConnectivityAcc& connect, E_Int K){connect.getEntry(K, _nodes);}
  
  template<typename box_t>
  void bbox(const K_FLD::FloatArray& crd, box_t&bb) const
  {
    bb.compute(crd, _nodes, _nb_nodes, -_shift);
  }
  ///
  //template <typename TriangulatorType>
  //inline E_Int triangulate(const TriangulatorType& t, const K_FLD::FloatArray& coord, K_FLD::IntArray& connectT3);//WARNING : connectT3 is Apended (not cleared upon entry)
  
  ///
  template <typename TriangulatorType>
  E_Int triangulate (const TriangulatorType& dt, const K_FLD::FloatArray&) const ;

  ///
  template <typename TriangulatorType>
  static E_Int triangulate
  (const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, K_FLD::IntArray& connectT3, bool do_no_shuffle = true, bool improve_qual = false);//WARNING : connectT3 is Apended (not cleared upon entry)
  
  ///
  template <typename TriangulatorType>
  static E_Int triangulate_inplace
  (const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Int* begin, bool do_no_shuffle = true, bool improve_qual = false);//WARNING : connectT3 is filled IN PLACE (not cleared upon entry)
  
  ///
  template <typename TriangulatorType>
  static E_Int triangulate_inplace
  (const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Int* begin, E_Int* neigh, bool do_no_shuffle = true, bool improve_qual = false);//WARNING : connectT3 is filled IN PLACE (not cleared upon entry) and neighbor is not sync with begin
  
  
  ///
  template <typename TriangulatorType>
  static E_Int triangulate
  (const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, K_FLD::IntArray& connectT3, K_FLD::IntArray& neighbors, bool do_no_shuffle = true, bool improve_qual = false);  
  
  inline void triangle(E_Int i, E_Int* target) const 
  {
    assert (_triangles != nullptr);
    const E_Int* p = &_triangles[i*3];
    target[0] = *(p++);
    target[1] = *(p++);
    target[2] = *p;
    
  }
  
  ///
  static inline void cvx_triangulate(const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int ibest, E_Int index_start, K_FLD::IntArray& connectT3)
  {
    ibest = (ibest != E_IDX_NONE) ? ibest : 0;
    ibest = (ibest < nb_nodes && ibest > -1) ? ibest : 0;
    
    E_Int N0 = nodes[ibest] - index_start;
    E_Int I((ibest+1)%nb_nodes);
    E_Int nb_tris = nb_nodes - 2;
    E_Int T[3];
    T[0]=N0;
    for (E_Int i=0; i < nb_tris; ++i)
    {
      T[1] = nodes[I] - index_start;
      T[2] = nodes[(I + 1) % nb_nodes] - index_start;
      connectT3.pushBack(T, T + 3);
      I = (I + 1) % nb_nodes;
    }
  }
  
  ///
  E_Int shuffle_triangulation();

  ///
  template<NUGA::eSUBDIV_TYPE STYPE>
  static void split(const E_Int* refE, E_Int n, ngon_unit& PGs, E_Int posChild);

  ///
  static E_Int get_sharp_edges
    (const K_FLD::FloatArray& crd, const ngon_unit& PGS, const E_Int* first_pg, E_Int nb_pgs, const E_Int* orient, const ngon_unit& lneighbors,
     std::set<K_MESH::NO_Edge>& sharp_edges, E_Float angular_threshold, const E_Float** normals = 0);

  ///
  static E_Int update_neighbor_with_sharp_edges
    (const K_FLD::FloatArray& crd, const ngon_unit& PGS, const E_Int* first_pg, E_Int nb_pgs, const E_Int* orient, ngon_unit& lneighbors,
    E_Float angular_threshold, const E_Float** normals = 0);

  ///
  static E_Int full_agglomerate
    (const K_FLD::FloatArray& crd, const ngon_unit& PGS, const E_Int* first_pg, E_Int nb_pgs, E_Float angular_tol, 
     const E_Int* orient, ngon_unit& agglo_pgs, std::vector<E_Int>& nids, const E_Float** normals = 0);
  
  ///todo
  template <typename TriangulatorType>
  static E_Int convex_agglomerate(const ngon_unit& PGS, const E_Int* first_pg, E_Int nb_pgs, E_Float angular_tol, ngon_unit& agglo_pgs, std::vector<E_Int>& nids);
  
  ///
  static E_Int build_pg_neighborhood(const ngon_unit& PGS, ngon_unit& neighbor, const E_Int* first_pg = 0, E_Int nb_pgs = 0,
                                     const std::set<K_MESH::NO_Edge>* wall = 0/*extra specified walls*/);
  ///
  template <typename CoordAcc, E_Int DIM>
  inline void iso_barycenter(const CoordAcc& coord, E_Float* G);
  
  // Center of gravity of the summit : the polygon is seen as a cloud . INACURATE FOR IRREGULAR DISTRIBUTION OF NODES
  template <typename CoordAcc, E_Int DIM>
  static inline void iso_barycenter(const CoordAcc& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G);

  static inline void iso_barycenter(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G);

  // Center of gravity using the topology (weighted with triangles area)
  template <E_Int DIM>
  static inline void centroid(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G);
  
  // Center of gravity using the topology (weighted with edge length-based) TODO
  //template <E_Int DIM>
  //static inline void center_of_mass(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G);
  
  void edge_length_extrema(const K_FLD::FloatArray& crd, E_Float& Lmin2, E_Float& Lmax2) const
  {
    Lmin2 = K_CONST::E_MAX_FLOAT;
    Lmax2 = -1.;
  
    for (E_Int i=0; i < _nb_nodes; ++i)
    {
      const E_Float* Pi = crd.col(node(i));
      const E_Float* Pj = crd.col(node((i+1)%_nb_nodes));
      
      E_Float L2 = K_FUNC::sqrDistance(Pi,Pj, 3);
      Lmin2 = MIN(Lmin2, L2);
      Lmax2 = MAX(Lmax2, L2);
    }
  }

  double L2ref(const K_FLD::FloatArray& crd) const
  {
    E_Float Lmin2, Lmax2;
    edge_length_extrema(crd, Lmin2, Lmax2);
    return Lmin2;
  }

  double L2ref(const std::vector<E_Float>& nodal_tol2) const
  {
    return L2ref(_nodes, _nb_nodes, nodal_tol2, _shift);
  }

  static double L2ref(const E_Int* nodes, E_Int nb_nodes, const std::vector<E_Float>& nodal_tol2, E_Int shift)
  {
    double val = K_CONST::E_MAX_FLOAT;
    for (E_Int n = 0; n < nb_nodes; ++n)
    {
      E_Int Ni = *(nodes + n) + shift;
      val = std::min(val, nodal_tol2[Ni]);
    }
    return val;
  }
  
  inline void getBoundary(E_Int n, boundary_type& b) const {b.setNodes(_nodes[n], _nodes[(n+1)%_nb_nodes]);}
  inline void getBoundary(E_Int n, K_MESH::Edge& b) const {b.setNodes(_nodes[n]-1, _nodes[(n+1)%_nb_nodes]-1);}
    
  static void getBoundary(const Polygon&  T1, const Polygon&  T2, E_Int& i1, E_Int& i2) ;
  
  static E_Int get_boundary
    (const ngon_unit& PGS, const std::vector<E_Int>& ids, std::deque<E_Int>& PGi, const std::vector<E_Int>& orient,
    std::set<K_MESH::Edge>& w_oe_set, std::map<E_Int, E_Int>& w_n_map);
  
  static E_Int getOrientation(const Polygon&  PG, 
                              const E_Int& Ni, const E_Int& Nj, 
                              E_Bool& same_orient);
  
  inline static E_Int getLocalNodeId(const E_Int* nodes, E_Int nb_nodes, E_Int Ni)
  {
    for (E_Int n = 0; n < nb_nodes; ++n)
    {
      //std::cout << *(nodes+n) << std::endl;
      if (*(nodes+n) == Ni) return n;
    }
    assert (false);// should never get here.
    return E_IDX_NONE;
  }
  
  /// Computes the surface vector
  template <typename CoordAcc, E_Int DIM>
  static inline void ndS(const CoordAcc& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* ndS);
  
  template <typename mesh_t>
  static inline void compute_dir(const mesh_t& mesh, const E_Float* origin, E_Float* dir);
    
  /// Compute the surface : norm of above
  template <typename Coord_t, E_Int DIM>
  static E_Float surface(const Coord_t& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start);

  // Computes the normal : normalized ndS
  template <typename CoordAcc, E_Int DIM>
  static inline void normal(const CoordAcc& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* W);

  // Computes the normal : normalized ndS
  template <typename CoordAcc, E_Int DIM>
  inline void normal(const CoordAcc& crd, E_Float* W) const
  {
    normal<CoordAcc, DIM>(crd, _nodes, _nb_nodes, -_shift, W);
  }

  //
  static E_Int get_oriented_normal(const K_FLD::FloatArray& crd, const ngon_unit& pgs, E_Int PGi/*glob id : sync with normals*/, bool reverse, E_Float* Normi, const E_Float** normals = 0);
  
  /// Tells if the input Polygon is star-shaped in regard with queryP.
  template <E_Int DIM>
  bool is_star_shaping(const E_Float* queryP, const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start);

  // returns the worst reflex info (K0, n0)
  static bool is_convex
    (const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start,
    const E_Float* normal, E_Float convexity_tol, E_Int& iworst, E_Int& ibest);

  // predicate
  static bool is_convex
    (const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start,
    const E_Float* normal, E_Float convexity_tol);
  
  static bool is_spiky
  (const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int idx_start, E_Int& is, E_Int& ie);

  // Polygon-Edge intersection
  template <typename TriangulatorType>
  bool intersect
  (const K_FLD::FloatArray&crd, const E_Float* Q0, const E_Float* Q1,
    E_Float tol, E_Bool tol_is_absolute, E_Float& u0, E_Float& u1, E_Bool& overlap);

  static void sync_join(const K_FLD::FloatArray&crd1, const E_Int* nodes1, E_Int idx_strt1, const K_FLD::FloatArray&crd2, E_Int* nodes2, E_Int idx_strt2, E_Int nb_nodes, bool do_reverse = false);
  static void shift_geom(const K_FLD::FloatArray&crd, E_Int* nodes, E_Int nnodes, E_Int idx_strt);

private: 
  Polygon(const Polygon& orig);
 
protected:
    E_Int _nb_nodes;
    const E_Int*_nodes;
    E_Int _shift;
    mutable E_Int* _triangles;

};

///
template <typename TriangulatorType>
E_Int Polygon::triangulate
(const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, K_FLD::IntArray& connectT3,
 bool do_not_shuffle, bool improve_quality)
{
  K_FLD::IntArray cM, neighbors;
  E_Int err = Polygon::triangulate(t, coord, nodes, nb_nodes, index_start, cM, neighbors, do_not_shuffle, improve_quality);
  if (!err) connectT3.pushBack(cM);
  return err;
}

///
template <typename TriangulatorType>
E_Int Polygon::triangulate_inplace
(const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Int* begin,
 bool do_not_shuffle, bool improve_quality)
{
  K_FLD::IntArray cM, neighbors;
  E_Int err = Polygon::triangulate(t, coord, nodes, nb_nodes, index_start, cM, neighbors, do_not_shuffle, improve_quality);
  if (!err)
  {
    for (E_Int i=0; i < cM.cols(); ++i)
      std::copy(cM.col(i), cM.col(i)+3, begin + 3*i);
  }
  return err;
}

///
template <typename TriangulatorType>
E_Int Polygon::triangulate_inplace
(const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Int* begin, E_Int* neigh,
 bool do_not_shuffle, bool improve_quality)
{
  K_FLD::IntArray cM, neighbors;
  E_Int err = Polygon::triangulate(t, coord, nodes, nb_nodes, index_start, cM, neighbors, do_not_shuffle, improve_quality);
  if (!err)
  {
    for (E_Int i=0; i < cM.cols(); ++i){
      std::copy(cM.col(i), cM.col(i)+3, begin + 3*i);
      std::copy(neighbors.col(i), neighbors.col(i)+3, neigh + 3*i);
    }
  }
  return err;
}

///
template <typename TriangulatorType>
E_Int Polygon::triangulate
(const TriangulatorType& t, const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, K_FLD::IntArray& connectT3, K_FLD::IntArray& neighbors,
 bool do_not_shuffle, bool improve_quality)
{
  // OVERWRITTEN : append connectT3. not cleared

  E_Int err(0);
  K_MESH::Triangle T;
   
  E_Int Ei[4], shft (index_start);
  //E_Float n1[3], n2[3], NjNk[3], NjNi[3];
  assert (nb_nodes > 2);//should have been cleaned before
      
  switch (nb_nodes)
  {
    case 3:
    {
      Ei[0]=nodes[0]-shft;Ei[1]=nodes[1]-shft;Ei[2]=nodes[2]-shft;
      connectT3.pushBack(Ei, Ei+3);
      //neighbors.resize(3, /*neighbors.cols()+*/1, E_IDX_NONE);
      break;
    }
    /*case 4:fixme !!
    {
      const E_Float* Ni = coord.col(nodes[0]-shft);
      const E_Float* Nj = coord.col(nodes[1]-shft);
      const E_Float* Nk = coord.col(nodes[2]-shft);
      
      K_FUNC::diff<3>(Nk, Nj, NjNk);
      K_FUNC::diff<3>(Ni, Nj, NjNi);
      
      K_FUNC::crossProduct<3>(NjNk, NjNi, n1);
      E_Int k=1;
      bool convex=true;
      E_Float s1(0.), s2;
      for (; (k < 4); ++k)
      {
        const E_Float* Ni = coord.col(nodes[k]-shft);
        const E_Float* Nj = coord.col(nodes[(k+1)%4]-shft);
        const E_Float* Nk = coord.col(nodes[(k+2)%4]-shft);
        
        K_FUNC::diff<3>(Nk, Nj, NjNk);
        K_FUNC::diff<3>(Ni, Nj, NjNi);
        
        K_FUNC::crossProduct<3>(NjNk, NjNi, n2);
        s2 = K_FUNC::dot<3>(n1, n2);
        if (s2*s1 < 0.)
        {
          if (s1 > 0.)
          {
            // k is concave
            convex = false; break;
          }
          else // if (s1 < 0.)
          {
            // k -1 is concave
            --k;
            convex = false; break;
          }
        }
        
        s1 = s2; 
        n1[0]=n2[0];n1[1]=n2[1];n1[2]=n2[2];
      }
        
      if (convex)
      {
        Ei[0]=nodes[0]-shft;Ei[1]=nodes[1]-shft;Ei[2]=nodes[3]-shft;
        connectT3.pushBack(Ei, Ei+3);
        Ei[0]=Ei[1];Ei[1]=nodes[2]-shft;
        connectT3.pushBack(Ei, Ei+3);
        break;
      }
      else //split on concave point
      {
        Ei[0]=nodes[k]-shft;Ei[1]=nodes[(k+1)%4]-shft;Ei[2]=nodes[(k+2)%4]-shft;
        connectT3.pushBack(Ei, Ei+3);
        //K_FLD::IntArray tmp;
        //tmp.pushBack(Ei, Ei + 3);
        Ei[1]=Ei[2];Ei[2]=nodes[(k+3)%4]-shft;
        connectT3.pushBack(Ei, Ei+3);
        //tmp.pushBack(Ei, Ei + 3);
        //MIO::write("concaveq4.mesh", coord, tmp, "TRI");
        break;
      }
   }*/
    default: err = t.run(coord, nodes, nb_nodes, shft, connectT3, neighbors, do_not_shuffle, improve_quality);break; //indexing to -1 is done inside
  }
  
  return err;
}

 ///
template <typename TriangulatorType>
E_Int Polygon::triangulate
  (const TriangulatorType& dt, const K_FLD::FloatArray& crd) const 
{
  if (_triangles != nullptr) return 0;
  
  E_Int ntris = _nb_nodes -2;
  _triangles = new E_Int[ntris*3];
    
  E_Int err = K_MESH::Polygon::triangulate_inplace(dt, crd, _nodes, _nb_nodes, -_shift/*index start*/, _triangles, false/*do_not_shuffle*/, false/*improve_quality*/);

  return err;
  }

  template<>
  inline void Polygon::split<NUGA::ISO_HEX>(const E_Int* refE, E_Int n, ngon_unit& PGs, E_Int posChild)
  {
    //todo JP : refE => (n-1)/2 QUADS
    // exemple : voir Q9::split dans Q9.hxx
  }


///
template <> inline
void Polygon::iso_barycenter<K_FLD::FloatArray, 3>(const K_FLD::FloatArray& coord, E_Float* G)
{ 
  //
  for (size_t d=0; d < 3; ++d) G[d]=0.;
  
  for (E_Int i=0; i < _nb_nodes; ++i)
  {
    for (size_t d=0; d < 3; ++d)
    {
      //std::cout << "v : " << coord.getVal(node(i), d) << std::endl;
      G[d] += coord(d, node(i));
    }
  }
  
  E_Float k = 1./(E_Float)_nb_nodes;
  
  for (size_t i = 0; i < 3; ++i) G[i] *= k;
  //std::cout << "G : " << G[0] << "/" << G[1] << "/" << G[2] << std::endl;
  
}

template <typename CoordAcc, E_Int DIM> inline
void Polygon::iso_barycenter(const CoordAcc& coord, E_Float* G)
{ 
  //
  for (size_t d=0; d < DIM; ++d) G[d]=0.;
  
  for (E_Int i=0; i < _nb_nodes; ++i)
  {
    for (size_t d=0; d < DIM; ++d)
    {
      //std::cout << "v : " << coord.getVal(node(i), d) << std::endl;
      G[d] += coord.getVal(node(i), d);
    }
  }
  
  E_Float k = 1./(E_Float)_nb_nodes;
  
  for (size_t i = 0; i < DIM; ++i) G[i] *= k;
  //std::cout << "G : " << G[0] << "/" << G[1] << "/" << G[2] << std::endl;
  
}

template <typename CoordAcc, E_Int DIM> inline
void Polygon::iso_barycenter(const CoordAcc& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G) 
{ 
  //
  for (size_t d=0; d < DIM; ++d) G[d]=0.;
  
  for (E_Int i=0; i < nb_nodes; ++i)
  {
    for (size_t d=0; d < DIM; ++d)
    {
      G[d] += coord.getVal(nodes[i]-index_start, d);
    }
  }
  
  E_Float k = 1./(E_Float)nb_nodes;
  
  for (size_t i = 0; i < DIM; ++i) G[i] *= k;
  //std::cout << "G : " << G[0] << "/" << G[1] << "/" << G[2] << std::endl;
  
}

template <> inline
void Polygon::iso_barycenter<K_FLD::FloatArray, 3>(const K_FLD::FloatArray& coord, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G) 
{ 
  //
  G[0]=G[1]=G[2]=0.;
  
  for (E_Int i=0; i < nb_nodes; ++i)
  {
    const E_Float* P = coord.col(nodes[i]-index_start);
    G[0]+=P[0];
    G[1]+=P[1];
    G[2]+=P[2];
  }
  
  E_Float k = 1./(E_Float)nb_nodes;
  
  G[0] *= k;
  G[1] *= k;
  G[2] *= k;
}

void Polygon::iso_barycenter(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* G)
{
  K_MESH::Polygon::iso_barycenter<K_FLD::FloatArray, 3>(crd, nodes, nb_nodes, index_start, G);
}

///
template <typename CoordAcc, E_Int DIM> inline
void Polygon::ndS(const CoordAcc& acrd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* ndS)
{
  E_Float G[3], Pi[3], Pj[3], V1[3], V2[3], w[3];

  K_MESH::Polygon::iso_barycenter<CoordAcc, DIM >(acrd, nodes, nb_nodes, index_start, G);
  
  // Compute an approximate normal W to the contour's surface (oriented toward outside).
  ndS[0] = ndS[1] = ndS[2] = 0.;

  E_Int Ni, Nj;
  for (E_Int n = 0; n < nb_nodes; ++n)
  {
    Ni = nodes[n] - index_start;
    Nj = nodes[(n+1)%nb_nodes] - index_start;
    
    acrd.getEntry(Ni, Pi);
    acrd.getEntry(Nj, Pj);

    K_FUNC::diff<DIM>(Pi, G, V1);
    K_FUNC::diff<DIM>(Pj, G, V2);
    
    // prevent numerical error when computing cross product. fixme : should be done evrywhere a cross product or determinant is done ?
    for (size_t i=0; i < DIM; ++i)
    {
      V1[i]=ROUND(V1[i]);
      V2[i]=ROUND(V2[i]);
    }

    K_FUNC::crossProduct<DIM>(V1, V2, w);
    K_FUNC::sum<DIM>(0.5, w, ndS, ndS);
  }
}

///
template <> inline
void Polygon::ndS<K_FLD::FloatArray,3>(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* ndS)
{
  E_Float G[3], V1[3], V2[3], w[3];
  
  K_MESH::Polygon::iso_barycenter<K_FLD::FloatArray, 3 >(crd, nodes, nb_nodes, index_start, G);
  
  // Compute an approximate normal W to the contour's surface (oriented toward outside).
  ndS[0] = ndS[1] = ndS[2] = 0.;

  E_Int Ni, Nj;
  for (E_Int n = 0; n < nb_nodes; ++n)
  {
    Ni = nodes[n] - index_start;
    Nj = nodes[(n+1)%nb_nodes] - index_start;
    
    const E_Float* Pi = crd.col(Ni);
    const E_Float* Pj = crd.col(Nj);

    K_FUNC::diff<3>(Pi, G, V1);
    K_FUNC::diff<3>(Pj, G, V2);
    
    // prevent numerical error when computing cross product. fixme : should be done evrywhere a cross product or determinant is done ?
    for (size_t i=0; i < 3; ++i)
    {
      V1[i]=ROUND(V1[i]);
      V2[i]=ROUND(V2[i]);
    }

    K_FUNC::crossProduct<3>(V1, V2, w);
    K_FUNC::sum<3>(0.5, w, ndS, ndS);
  }
}

///
template <typename mesh_t> inline
void Polygon::compute_dir(const mesh_t& poly_line/*0-based*/, const E_Float* origin, E_Float* dir)
{
  E_Float V1[3], V2[3], w[3];
  
  // Compute an approximate normal W to the contour's surface (oriented toward outside).
  dir[0] = dir[1] = dir[2] = 0.;

  //
  E_Int nedges = poly_line.ncells();
  for (E_Int n = 0; n < nedges; ++n)
  {
    E_Int Ni = poly_line.cnt(0,n); 
    E_Int Nj = poly_line.cnt(1,n);
    
    const E_Float* Pi = poly_line.crd.col(Ni);
    const E_Float* Pj = poly_line.crd.col(Nj);

    K_FUNC::diff<3>(Pi, origin, V1);
    K_FUNC::diff<3>(Pj, origin, V2);
    
    // prevent numerical error when computing cross product. fixme : should be done evrywhere a cross product or determinant is done ?
    for (size_t i=0; i < 3; ++i)
    {
      V1[i]=ROUND(V1[i]);
      V2[i]=ROUND(V2[i]);
    }

    K_FUNC::crossProduct<3>(V1, V2, w);
    K_FUNC::sum<3>(1., w, dir, dir);
  }
}

///
template <E_Int DIM> inline
void Polygon::centroid(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* cM)
{
  // A TESTER
  
  // More accurate than iso_barycenter which see the polygon as a cloud a point.
  // Here the topology is taken into account
  // WARNING : inacurate for non planar cases
  
  cM[0] = cM[1] = cM[2] = 0.;
  
  E_Float G[DIM]/*iso_bar*/, n[DIM]/*normal*/, S/*surface*/;
  iso_barycenter<K_FLD::FloatArray, 3>(crd, nodes, nb_nodes, index_start, G);
  K_MESH::Polygon::ndS<K_FLD::FloatArray, 3>(crd, nodes, nb_nodes, index_start, n);
  S = K_FUNC::normalize<DIM>(n);

  E_Float vsi[DIM], si, gi[DIM], w;
  
  for (E_Int i = 0; i < nb_nodes; ++i)
  {
    E_Int Ni = nodes[i] - index_start;
    E_Int Nip1 = nodes[(i + 1) % nb_nodes] - index_start;
    
    K_MESH::Triangle::ndS<3>(G, crd.col(Ni), crd.col(Nip1), vsi);
    si = ::sqrt(K_FUNC::sqrNorm<DIM>(vsi));
    K_MESH::Triangle::isoG(G, crd.col(Ni), crd.col(Nip1), gi);
    
    w = SIGN(K_FUNC::dot<DIM>(vsi, n)) * si;
    
    cM[0] += w * gi[0];
    cM[1] += w * gi[1];
    cM[2] += w * gi[2];
  }

  w = 1. / S;

  cM[0] *= w;
  cM[1] *= w;
  cM[2] *= w;
}

template <> inline 
E_Float Polygon::surface<K_FLD::FloatArray,2>(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start)
{
  E_Float O[]={0.,0.}, s(0.);
//  E_Float /*O[]={0.,0.},*/ s(0.); using iso_barycenter doesn not seem better in term of precision...
//  E_Float O[2];
//  aDynCrd_t acrd(crd);
//  iso_barycenter<aDynCrd_t, 2>(acrd, nodes, nb_nodes, index_start, O);
  for (E_Int n=0; n < nb_nodes; ++n)
    s += K_MESH::Triangle::surface<2>(&O[0], crd.col(*(nodes+n)-index_start), crd.col(*(nodes+(n+1)%nb_nodes)-index_start));
  return s;
}

///
template <> inline 
E_Float Polygon::surface<K_FLD::FloatArray,3>(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start)
{
  E_Float ndS[3];
  K_MESH::Polygon::ndS<K_FLD::FloatArray, 3>(crd, nodes, nb_nodes, index_start, ndS);
  return ::sqrt(K_FUNC::sqrNorm<3>(ndS));
}

template <> inline 
E_Float Polygon::surface<K_FLD::ArrayAccessor<K_FLD::FldArrayF>,2>(const K_FLD::ArrayAccessor<K_FLD::FldArrayF>& acrd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start)
{
  E_Float O[]={0.,0.}, s(0.), Pn[2], Pnp1[2];
  for (E_Int n=0; n < nb_nodes; ++n)
  {
    acrd.getEntry(*(nodes+n)-index_start, Pn);
    acrd.getEntry(*(nodes+(n+1)%nb_nodes)-index_start, Pnp1);
    //std::cout << Pn[0] << " " << Pn[1]<< std::endl;
    //std::cout << Pnp1[0] << " " << Pnp1[1]<< std::endl;
    s += K_MESH::Triangle::surface<2>(&O[0], Pn, Pnp1);
  }
  return s;
}

template <> inline 
E_Float Polygon::surface<K_FLD::ArrayAccessor<K_FLD::FloatArray>,2>(const K_FLD::ArrayAccessor<K_FLD::FloatArray>& acrd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start)
{
  E_Float O[]={0.,0.}, s(0.), Pn[2], Pnp1[2];
  for (E_Int n=0; n < nb_nodes; ++n)
  {
    acrd.getEntry(*(nodes+n)-index_start, Pn);
    acrd.getEntry(*(nodes+(n+1)%nb_nodes)-index_start, Pnp1);
    //std::cout << Pn[0] << " " << Pn[1]<< std::endl;
    //std::cout << Pnp1[0] << " " << Pnp1[1]<< std::endl;
    s += K_MESH::Triangle::surface<2>(&O[0], Pn, Pnp1);
  }
  return s;
}

template <typename CoordAcc, E_Int DIM> inline
void Polygon::normal(const CoordAcc& acrd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* n)
{
  //
  K_MESH::Polygon::ndS<CoordAcc, DIM>(acrd, nodes, nb_nodes, index_start, n);
  K_FUNC::normalize<DIM>(n);
}

template <> inline
void Polygon::normal<K_FLD::FloatArray, 3>(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start, E_Float* n)
{
  K_MESH::Polygon::ndS<K_FLD::FloatArray, 3>(crd, nodes, nb_nodes, index_start, n);
  K_FUNC::normalize<3>(n);
}

///
template <E_Int DIM> inline
bool Polygon::is_star_shaping
(const E_Float* queryP, const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int index_start)
{
  E_Float ndS[DIM], vsi[DIM];
  K_MESH::Polygon::ndS<DIM>(crd, nodes, nb_nodes, index_start, ndS);
  
  for (E_Int i = 0; i < nb_nodes; ++i)
  {
    E_Int Ni = nodes[i] - index_start;
    E_Int Nip1 = nodes[(i + 1) % nb_nodes] - index_start;
    
    K_MESH::Triangle::ndS<3>(queryP, crd.col(Ni), crd.col(Nip1), vsi);
    
    if (SIGN(K_FUNC::dot<3>(vsi, ndS)) < 0)
      return false;
  }
  
  return true;
}

template <typename TriangulatorType>
bool Polygon::intersect
(const K_FLD::FloatArray&crd, const E_Float* Q0, const E_Float* Q1, 
  E_Float tol, E_Bool tol_is_absolute, E_Float& u0, E_Float& u1, E_Bool& overlap)
{
  TriangulatorType dt;
  this->triangulate(dt, crd);
  E_Int ntris = nb_tris();

  E_Int T[3];
  for (E_Int i = 0; i < ntris; ++i)
  {
    this->triangle(i, T);
    const E_Float* P0 = crd.col(T[0]);
    const E_Float* P1 = crd.col(T[1]);
    const E_Float* P2 = crd.col(T[2]);
    E_Int tx;
    if (K_MESH::Triangle::intersect<3>(P0, P1, P2, Q0, Q1, tol, tol_is_absolute, u0, u1, tx, overlap))
      return true;
  }

  return false;
}

///
inline void Polygon::sync_join(const K_FLD::FloatArray&crd1, const E_Int* nodes1, E_Int idx_strt1, const K_FLD::FloatArray&crd2, E_Int* nodes2, E_Int idx_strt2, E_Int nb_nodes, bool do_reverse)
{
  E_Float n1[3], n2[3];
  normal<K_FLD::FloatArray, 3>(crd1, nodes1, nb_nodes, 1, n1);
  normal<K_FLD::FloatArray, 3>(crd2, nodes2, nb_nodes, 1, n2);

  if (do_reverse) {
    bool reversed = (SIGN(K_FUNC::dot<3>(n1, n2)) == -1);
    if (reversed)
      std::reverse(nodes2, nodes2 + nb_nodes);
  }

  //look for nodes1[0] in nodes2
  const E_Float* P0{ crd1.col(nodes1[0]-idx_strt1) };
  E_Float zerom2{ ZERO_M*ZERO_M };
  E_Int i0{ E_IDX_NONE };
  for (E_Int i=0; i < nb_nodes; ++i)
  {
    const E_Float* Pi = crd2.col(nodes2[i] - idx_strt2);
    if (K_FUNC::sqrDistance(P0, Pi, 3) < zerom2)
    {
      i0 = i;
      break;
    }
  }

  assert(i0 != E_IDX_NONE);

  K_CONNECT::IdTool::right_shift(nodes2, nb_nodes, i0);

}

inline void Polygon::shift_geom(const K_FLD::FloatArray&crd, E_Int* nodes, E_Int nnodes, E_Int idx_strt)
{
  using palma_t = std::vector<std::pair<E_Float, E_Int>>;

  std::vector<E_Int> cands;
  K_CONNECT::IdTool::init_inc(cands, nnodes);

  palma_t palma;
  palma.reserve(nnodes);

  // check min X
  for (E_Int i = 0; i < nnodes; ++i)
    palma.push_back(std::make_pair(crd(0, nodes[i] - idx_strt), i));

  std::sort(ALL(palma));

  if (palma[0].first < palma[1].first - ZERO_M)
  {
    K_CONNECT::IdTool::right_shift(nodes, nnodes, palma[0].second);
    return;
  }

  if (palma[1].first < palma[2].first - ZERO_M)      // keep first 2
  {
    cands[0] = palma[0].second;
    cands[1] = palma[1].second;
    cands.resize(2);
  }
  else if (palma[2].first < palma[3].first - ZERO_M) // keep first 3
  { 
    cands[0] = palma[0].second;
    cands[1] = palma[1].second;
    cands[2] = palma[2].second;
    cands.resize(3);
  }

  // else check min Y on remaining candidates

  palma.clear();
  for (size_t i = 0; i < cands.size(); ++i)
    palma.push_back(std::make_pair(crd(1, nodes[cands[i]] - idx_strt), i));

  std::sort(ALL(palma));

  if (palma[0].first < palma[1].first - ZERO_M)
  {
    E_Int locid = cands[palma[0].second];
    K_CONNECT::IdTool::right_shift(nodes, nnodes, locid);
    return;
  }

  if ((cands.size() > 2) && (palma[1].first < palma[2].first - ZERO_M))      // keep first 2
  {
    cands[0] = palma[0].second;
    cands[1] = palma[1].second;
    cands.resize(2);
  }
  else if ((cands.size() > 3) && (palma[2].first < palma[3].first - ZERO_M)) // keep first 3
  {
    cands[0] = palma[0].second;
    cands[1] = palma[1].second;
    cands[2] = palma[2].second;
    cands.resize(3);
  }

  // else check min Z

  palma.clear();
  for (size_t i = 0; i < cands.size(); ++i)
    palma.push_back(std::make_pair(crd(2, nodes[cands[i]] - idx_strt), i));

  std::sort(ALL(palma));

  assert(palma[0].first < palma[1].first - ZERO_M);

  E_Int locid = cands[palma[0].second];
  K_CONNECT::IdTool::right_shift(nodes, nnodes, locid);
  return;
}

}
#endif	/* __K_MESH_POLYGON_H__ */
