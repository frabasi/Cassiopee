/*



--------- NUGA v1.0



*/
//Authors : S�m Landier (sam.landier@onera.fr)

#ifndef __K_MESH_HEXAHEDRON_H__
#define __K_MESH_HEXAHEDRON_H__

#include "Nuga/include/Quadrangle.h"
#include "Nuga/include/DynArray.h"
#include "Nuga/include/ArrayAccessor.h"
#include "Nuga/include/ngon_t.hxx"
#include "Nuga/include/defs.h"
#include "Polygon.h"

namespace K_MESH
{

class Hexahedron {
  
public:
  static constexpr E_Int NB_NODES = 8;
  static constexpr E_Int NB_TRIS = 12;
  static constexpr E_Int NB_BOUNDS = 6;
  static constexpr E_Int NB_EDGES = 12;
 
  typedef K_MESH::Quadrangle boundary_type;
    
public:
  Hexahedron(){}
  Hexahedron(const E_Int* nodes, E_Int shift=0):_shift(shift){ for (size_t i = 0; i< 8; ++i)_nodes[i]=*(nodes++);}
 ~Hexahedron(){}
 
  template <typename ngunit_t>
  Hexahedron(const ngunit_t & PGs, const E_Int* first_pg){}

  template <typename ngunit_t>
  static bool is_of_type(const ngunit_t & PGs, const E_Int* first_pg, E_Int nb_pgs) { return K_MESH::Polyhedron<0>::is_HX8(PGs, first_pg, nb_pgs); }

  E_Int* nodes() { return _nodes;}
  const E_Int* nodes() const { return _nodes;}
  
  E_Int nb_nodes() const {return NB_NODES;}
  E_Int nb_tris() const {return NB_TRIS;}
  
  void setNodes(E_Int* nodes){for (size_t i = 0; i< 8; ++i)_nodes[i]=*(nodes++);}
  
  inline E_Int node(E_Int i){return _nodes[i]+_shift;}
  
  inline E_Int nbounds() { return 6;}
  
  template <typename Connectivity_t>
  inline void set(const K_FLD::ArrayAccessor<Connectivity_t>& connect, E_Int K)
  {connect.getEntry(K, _nodes);}
  
  E_Int volume(const K_FLD::FloatArray& crd, E_Float& v);
  
  void compact(const K_FLD::FloatArray& crdi, ngon_unit& pgs, K_FLD::FloatArray&crd);
  
  void triangulate(E_Int* target);//WARNING : connectT3 is Apended (not cleared upon entry)
  
  inline void triangle(E_Int i, E_Int* target)
  {
    assert (i >= 0 && i < NB_TRIS);
    
    switch (i)
    {
      case 0 : target[0] = _nodes[0]; target[1] = _nodes[3]; target[2] = _nodes[1]; break;  //031 BOTTOM
      case 1 : target[0] = _nodes[1]; target[1] = _nodes[3]; target[2] = _nodes[2]; break;  //132
      case 2 : target[0] = _nodes[4]; target[1] = _nodes[5]; target[2] = _nodes[7]; break;  //457 TOP
      case 3 : target[0] = _nodes[7]; target[1] = _nodes[5]; target[2] = _nodes[6]; break;  //756
      
      case 4 : target[0] = _nodes[3]; target[1] = _nodes[0]; target[2] = _nodes[7]; break;  //307 LEFT
      case 5 : target[0] = _nodes[7]; target[1] = _nodes[0]; target[2] = _nodes[4]; break;  //704
      case 6 : target[0] = _nodes[1]; target[1] = _nodes[2]; target[2] = _nodes[6]; break;  //126 RIGHT
      case 7 : target[0] = _nodes[1]; target[1] = _nodes[6]; target[2] = _nodes[5]; break;  //165
      case 8 : target[0] = _nodes[0]; target[1] = _nodes[1]; target[2] = _nodes[5]; break;  //015 FONT
      case 9 : target[0] = _nodes[0]; target[1] = _nodes[5]; target[2] = _nodes[4]; break;  //054
      case 10 : target[0] = _nodes[2]; target[1] = _nodes[3]; target[2] = _nodes[6]; break; //236 BACK
      case 11 : target[0] = _nodes[3]; target[1] = _nodes[7]; target[2] = _nodes[6]; break; //376
      default:break;
    }
  }
  
  ///
  template <typename TriangulatorType, typename acrd_t>
  void triangulate (const TriangulatorType& dt, const acrd_t& acrd) {} //dummy : for genericity
  ///
  template <typename acrd_t>
  E_Int cvx_triangulate (const acrd_t& acrd) {return 0;}
  
  static void get_edges(const E_Int* nodes, Vector_t<K_MESH::NO_Edge>& edges);
  
  static bool cross(const ngon_t<K_FLD::IntArray>& ng, const K_FLD::FloatArray& crd, const E_Int* face, E_Int nb_faces, K_FLD::FloatArray& data, E_Float* P0, E_Float* P1, E_Float& lambda0, E_Float& lambda1, E_Float tolerance);
  
  inline void getBoundary(E_Int n, boundary_type& b) const {
    
    switch (n)
    {
      case 0: b.setNodes(_nodes[0], _nodes[1], _nodes[2], _nodes[3]);break;
      case 1: b.setNodes(_nodes[4], _nodes[5], _nodes[6], _nodes[7]);break;
      case 2: b.setNodes(_nodes[0], _nodes[4], _nodes[7], _nodes[3]);break;
      case 3: b.setNodes(_nodes[1], _nodes[2], _nodes[6], _nodes[5]);break;
      case 4: b.setNodes(_nodes[0], _nodes[1], _nodes[5], _nodes[4]);break;
      case 5: b.setNodes(_nodes[2], _nodes[3], _nodes[7], _nodes[6]);break;
      default : break;
    }
  }
  
  inline void getBoundary(E_Int n, E_Int* nodes) const {
    
    switch (n)
    {
      case 0: nodes[0]=_nodes[0]; nodes[1]=_nodes[1]; nodes[2]=_nodes[2]; nodes[3]=_nodes[3];break;
      case 1: nodes[0]=_nodes[4]; nodes[1]=_nodes[5]; nodes[2]=_nodes[6]; nodes[3]=_nodes[7];break;
      case 2: nodes[0]=_nodes[0]; nodes[1]=_nodes[4]; nodes[2]=_nodes[7]; nodes[3]=_nodes[3];break;
      case 3: nodes[0]=_nodes[1]; nodes[1]=_nodes[2]; nodes[2]=_nodes[6]; nodes[3]=_nodes[5];break;
      case 4: nodes[0]=_nodes[0]; nodes[1]=_nodes[1]; nodes[2]=_nodes[5]; nodes[3]=_nodes[4];break;
      case 5: nodes[0]=_nodes[2]; nodes[1]=_nodes[3]; nodes[2]=_nodes[7]; nodes[3]=_nodes[6];break;
      default : break;
    }
  }
  
  template<typename box_t, typename CoordAcc>
  void bbox(const CoordAcc& acrd, box_t&bb) const
  {
    for (E_Int i = 0; i < 3; ++i)
      {bb.minB[i] = NUGA::FLOAT_MAX; bb.maxB[i] = -NUGA::FLOAT_MAX;}

    bb.compute(acrd, _nodes, NB_NODES, 0/*idx start*/);
  }
  
  template< typename ngo_t>
  static void reorder_pgs(ngo_t& ng, const K_FLD::IntArray& F2E, E_Int i);

  template <typename ngunit_t>
  static int get_opposite(const ngunit_t & PGs, const E_Int* first_pg, E_Int k);

  template <typename ngunit_t>
  static void get_local(const ngunit_t & PGs, const E_Int* first_pg, E_Int*& local);
  
  
  ///
  template <typename CoordAcc>
  inline void iso_barycenter(const CoordAcc& coord, E_Float* G);
  
  template <typename ngunit_t>
  static inline void iso_barycenter(const K_FLD::FloatArray& crd, const ngunit_t & PGs, const E_Int* first_pg, E_Int nb_pgs, E_Int index_start, E_Float* G);
  
  static inline void iso_barycenter(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int idx_start, E_Float* G);

  E_Float quality(const K_FLD::FloatArray& crd, E_Float* Vol){return 1;}

private:
  
  Hexahedron(const Hexahedron& orig);
 
private:
    E_Int _shift;
    E_Int _nodes[8];

};

template< typename ngo_t>
void Hexahedron::reorder_pgs(ngo_t& ng, const K_FLD::IntArray& F2E, E_Int i) // bot, top, left, right, front, back
{
  std::map<E_Int,E_Int> glmap; // crd1 to 0-26 indexes
  E_Int nb_faces = ng.PHs.stride(i); 
  E_Int* faces = ng.PHs.get_facets_ptr(i);
  E_Int PGi = faces[0] - 1;
  E_Int* pN = ng.PGs.get_facets_ptr(PGi);
  
  // by convention, first face is bottom, first node is 0 in local numbering (0 to 26)

  glmap[*pN] = 0; // PHi(0,0) -> 0  
  glmap[*(pN+1)] = 1;
  glmap[*(pN+2)] = 2;
  glmap[*(pN+3)] = 3;

  if (F2E(1,PGi) != i) // for BOT, PH is the right element. if not, wrong orientation => swap of 1 and 3
  { 
    glmap[*(pN+3)] = 1;
    glmap[*(pN+1)] = 3;
  }
  E_Int TopId(IDX_NONE),LeftId(IDX_NONE),RightId(IDX_NONE),FrontId(IDX_NONE),BackId(IDX_NONE);

  bool commonNodes[4];

  for (int k = 1; k < 6; ++k)
  {
    int count = 0;
    commonNodes[0] = commonNodes[1] = commonNodes[2] = commonNodes[3] = false;
    E_Int testedPG = faces[k]-1;
    E_Int* pNode = ng.PGs.get_facets_ptr(testedPG);

    for (int j = 0; j < 4; ++j)
    {
      auto it = glmap.find(pNode[j]);
      if (it != glmap.end())
      {
        // found
        count++;
        commonNodes[it->second] = true;
      }
    }
    if (count == 0) // no common point, the ith PG is the TOP
      TopId = k;
    else if (commonNodes[0] && commonNodes[1])
      FrontId = k;
    else if (commonNodes[1] && commonNodes[2])
      RightId = k;
    else if (commonNodes[2] && commonNodes[3])
      BackId = k;
    else if (commonNodes[0] && commonNodes[3])
      LeftId = k;
  }
  
  E_Int mol[6];

  mol[0] = faces[0];
  mol[1] = faces[TopId];
  mol[2] = faces[LeftId];
  mol[3] = faces[RightId];
  mol[4] = faces[FrontId];
  mol[5] = faces[BackId];

  assert (TopId != IDX_NONE && TopId != LeftId && TopId != RightId && TopId != FrontId && TopId != BackId);
  assert (LeftId != IDX_NONE && LeftId != TopId && LeftId != RightId && LeftId != FrontId && LeftId != BackId);
  assert (RightId != IDX_NONE && RightId != LeftId && RightId != TopId && RightId != FrontId && RightId != BackId);
  assert (FrontId != IDX_NONE && FrontId != LeftId && FrontId != RightId && FrontId != TopId && FrontId != BackId);
  assert (BackId != IDX_NONE && BackId != LeftId && BackId != RightId && BackId != FrontId && BackId != TopId);

  for (int i = 0; i < nb_faces; ++i)
    faces[i] = mol[i];
}

///
template <typename ngunit_t>
int Hexahedron::get_opposite(const ngunit_t & PGs, const E_Int* first_pg, E_Int k)
{
  E_Int PGk = first_pg[k]-1;
  const int* nodes = PGs.get_facets_ptr(PGk);
  std::set<int> bnodes(nodes, nodes+4), tmp;

  for (size_t i=0; i < 6; ++i)
  {
    if (i == k) continue;

    const int* inodes = PGs.get_facets_ptr(first_pg[i]-1);
    tmp = bnodes;

    int nb_common = 0;
    for (size_t j = 0; j < 4; ++j)
    {
      if (!tmp.insert(inodes[j]).second) //already in
      {
        ++nb_common;
        break;
      }
    }

    if (nb_common == 0) return i;

  }
  return IDX_NONE;
}


///
template <typename ngunit_t>
  static void get_local(const ngunit_t & PGs, const E_Int* first_pg, E_Int*& local)
{ /*alexis : todo : de taille 6, signe+ si l'orientation est bonne, 
   * -sinon, indice one-based du i0*/
}                                                                        

template <typename CoordAcc> inline
void Hexahedron::iso_barycenter(const CoordAcc& coord, E_Float* G)
{ 
  //
  for (size_t d=0; d < 3; ++d) G[d]=0.;
  
  for (E_Int i=0; i < NB_NODES; ++i)
  {
    for (size_t d=0; d < 3; ++d)
    {
      //std::cout << "v : " << coord.getVal(node(i), d) << std::endl;
      G[d] += coord.getVal(node(i), d);
    }
  }
  
  E_Float k = 1./(E_Float)NB_NODES;
  
  for (size_t i = 0; i < 3; ++i) G[i] *= k;
  //std::cout << "G : " << G[0] << "/" << G[1] << "/" << G[2] << std::endl;
  
}

  template <typename ngunit_t>
  inline void Hexahedron::iso_barycenter(const K_FLD::FloatArray& crd, const ngunit_t & PGs, const E_Int* first_pg, E_Int nb_pgs, E_Int index_start, E_Float* G)
  {
    //WARNING : assuming reodrederd pgs : first is bottom, second is top
    
    E_Int new_bary[8];

    for (int i = 0; i < 2; ++i) // 8 points : bot and top nodes
    {
      const E_Int* nodes = PGs.get_facets_ptr(first_pg[i]-index_start);
      E_Int nb_nodes = PGs.stride(first_pg[i]-index_start);
      
      for (int k = 0; k  < nb_nodes; ++k)
        new_bary[nb_nodes*i+k] = nodes[k];   
    }
    
    K_MESH::Polyhedron<STAR_SHAPED>::iso_barycenter(crd, new_bary, 8, 1, G);
  }

  inline void Hexahedron::iso_barycenter(const K_FLD::FloatArray& crd, const E_Int* nodes, E_Int nb_nodes, E_Int idx_start, E_Float* G)
  {
    K_MESH::Polyhedron<STAR_SHAPED>::iso_barycenter(crd, nodes, 8, idx_start, G);
  }
  
  ///
  inline E_Int Hexahedron::volume(const K_FLD::FloatArray& crd, E_Float& v)
  {    
    K_FLD::IntArray cT3(3, 12);
    for (E_Int i=0; i < 12; ++i)
      this->triangle(i, cT3.col(i));
    E_Float G[3];
    K_MESH::Polyhedron<UNKNOWN>::metrics(crd, cT3, v, G);
    return 0;
    
  }
  
  inline void Hexahedron::compact(const K_FLD::FloatArray& crdi, ngon_unit& pgs, K_FLD::FloatArray&crd) //1-based
  {
    assert (false); // not tested
    pgs.clear();
    crd.clear();
    
    E_Int F1[] = {1,4,3,2};
    E_Int F2[] = {5,6,7,8};
    E_Int F3[] = {1,5,8,4};
    E_Int F4[] = {2,3,7,6};
    E_Int F5[] = {1,2,6,5};
    E_Int F6[] = {3,4,8,7};
    
    pgs.add(4, F1);
    pgs.add(4, F2);
    pgs.add(4, F3);
    pgs.add(4, F4);
    pgs.add(4, F5);
    pgs.add(4, F6);
    
    pgs.updateFacets();
    
    for (size_t i=0; i < 8; ++i)
    {
      //E_Int id = node(i);
      crd.pushBack(crdi.col(node(i)), crdi.col(node(i))+3);
    }
  }

}
#endif	/* __K_MESH_HEXAHEDRON_H__ */
