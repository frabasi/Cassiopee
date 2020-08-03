/*
 
 
 
              NUGA 
 
 
 
*/

#include "Nuga/include/DynArray.h"
#include "Nuga/include/ngon_t.hxx"
#include "Nuga/include/localizer.hxx"
# include "Nuga/include/BbTree.h"
#include "Nuga/include/polygon.hxx"
#include "Nuga/include/polyhedron.hxx"

#ifndef NUGA_MESH_T_HXX
#define NUGA_MESH_T_HXX

using ngon_type = ngon_t<K_FLD::IntArray>;

namespace NUGA
{

enum eGEODIM { LINEIC = 1, SURFACIC = 2, VOLUMIC = 3};

// : default impl : fixed stride (Basic element in any DIM)
template <eGEODIM GEODIM, bool fixed_stride>
struct connect_trait;
//{
//  using cnt_t = K_FLD::IntArray;
//  // missing elt_t : need a polytop<GEODIM, N> => polyhedron<N> and a polygon<N>
//  static const eGEODIM BOUND_GEODIM = eGEODIM(GEODIM-1);
//  static const bool BOUND_STRIDE = fixed_stride;
//
//  static int ncells(const cnt_t& c) {return c.cols();}
//  static void shift(cnt_t& c, int v) { c.shift(v);}
//  static void unique_indices(const cnt_t&c, std::vector<E_Int>& uinds) { c.uniqueVals(uinds);}
//
//  //static void compact(cnt_t&c, const std::vector<int>& keep){ std::vector<E_Int> nids; c.compact(keep, nids);}
//};

// LINEIC : BAR
template <>
struct connect_trait<LINEIC, true>
{
  using cnt_t = K_FLD::IntArray;
  using elt_t = K_MESH::Edge; using aelt_t = K_MESH::aEdge;
  
  using neighbor_t = cnt_t;
  static const E_Int index_start=0;
  static const bool BOUND_STRIDE = true;

  static int ncells(const cnt_t& c) {return c.cols();}
  static void unique_indices(const cnt_t&c, std::vector<E_Int>& uinds) { c.uniqueVals(uinds);}
  
  static void compress(cnt_t&c, const std::vector<bool>& keep)
  {
    std::vector<E_Int> nids;
    K_CONNECT::keep<bool> pred_keep(keep);
    K_CONNECT::IdTool::compress(c, pred_keep, nids);
  }
  static void compress(cnt_t&c, const std::vector<int>& keepids, int index_start)
  {
    K_CONNECT::IdTool::compress(c, keepids, index_start);
  }

  static cnt_t compress_(cnt_t const& c, const std::vector<int>& keepids,int index_start)
  {
    return K_CONNECT::IdTool::compress_(c, keepids, index_start);
  }

  static void compact(cnt_t&c, const std::vector<bool>& keep){ std::vector<E_Int> nids; K_FLD::IntArray::compact(c, keep, nids);}
  static void compact_to_used_nodes(cnt_t& c, K_FLD::FloatArray& crd, std::vector<E_Int>& nids)
  {
    NUGA::MeshTool::compact_to_mesh(crd, c, nids);
  }

  static K_FLD::FloatArray compact_to_used_nodes(cnt_t&c, K_FLD::FloatArray const & crdi, std::vector<E_Int>& nids)
  {
    std::vector<E_Int> oids;
    K_FLD::FloatArray lcrd;
    K_FLD::IntArray lcnt;
    NUGA::MeshTool::compact_to_mesh(crdi, c, lcrd, lcnt, oids); //use this version to minimize mem print
    c = std::move(lcnt);

    // build nids, sized as crdi
    K_CONNECT::IdTool::reverse_indirection(oids, nids);
    nids.resize(crdi.cols(), E_IDX_NONE);

    return lcrd;
  }

  static void compute_nodal_tolerance(const K_FLD::FloatArray& crd, const cnt_t& cnt, std::vector<E_Float>& nodal_tolerance)
  {
    // should be factorized in a behaviour class
    K_FLD::FloatArray L;
    NUGA::MeshTool::computeIncidentEdgesSqrLengths(crd, cnt, L);
    L.extract_field(0, nodal_tolerance); // 0: extract the min
  }
  
  static void reverse_orient(cnt_t&c)
  {
    for (E_Int i=0; i<c.cols(); ++i)std::swap(c(0,i), c(1,i));
  }
  
};

// SURF
template <>
struct connect_trait<SURFACIC, false>
{
  using cnt_t = ngon_unit;
  using elt_t = K_MESH::Polygon;  using aelt_t = NUGA::aPolygon;
  using neighbor_t = cnt_t;

  static const eGEODIM BOUND_GEODIM = LINEIC;
  static const bool BOUND_STRIDE = true;
  static const E_Int index_start=1;

  static int ncells(const cnt_t& c) {return c.size();}
  static void shift(cnt_t& c, int v) { c.shift(v);}
  static void unique_indices(const cnt_t& c, std::vector<E_Int>& uinds) { c.unique_indices(uinds);}

  template <typename ELT_t> static void add(K_FLD::FloatArray& crd, cnt_t& c, ELT_t const& e)
  {
    E_Int shift = crd.cols() + index_start - e.shift();
    crd.pushBack(e.m_crd);
    c.add(e.nb_nodes(), e.begin(), shift);
  }
  
  static void build_neighbors(const cnt_t& c, neighbor_t& neighbors)
  {
    K_MESH::Polygon::build_pg_neighborhood(c, neighbors);
  }

  static void get_boundary(const cnt_t& c, K_FLD::IntArray& bc)
  {
    NUGA::MeshTool::getBoundary(c, bc);
  }
  static void get_boundary(const cnt_t& c, K_FLD::IntArray& bc, std::vector<E_Int>& ancestors)
  {
    NUGA::MeshTool::getBoundary(c, bc, &ancestors);
    bc.shift(-1); //NGON store 1-based, IntArray is 0-based
  }
  
  static void get_boundaries_by_color(const cnt_t& c, const neighbor_t& neighborz, std::map<E_Int, K_FLD::IntArray>& col_to_bound)
  {
    // WARNING : neighborz must contain some colors : one for the inner faces, and some other for the boundary
    for (E_Int i=0; i < c.size(); ++i)
    {
      E_Int nneigh = neighborz.stride(i);
      const E_Int* pneigh = neighborz.get_facets_ptr(i); //0-based
      
      E_Int nnodes = c.stride(i);
      const E_Int* pnodes = c.get_facets_ptr(i); //1-based
      
      assert (nnodes == nneigh);
      
      for (E_Int j=0; j < nneigh; ++j)
      {
        E_Int const & col = pneigh[j];
        E_Int E[] = {pnodes[j]-1, pnodes[(j+1)%nnodes]-1};
        
        col_to_bound[col].pushBack(E, E+2);
      }
    }
  }
    
  static cnt_t compress_(cnt_t const& c, const std::vector<int>& keepids, int index_start)
  {
    std::vector<E_Int> oids;
    ngon_unit pgs;
    c.extract(keepids, pgs, oids, index_start);
    return pgs;
  }

  static void compress(cnt_t&c, const std::vector<bool>& ikeep)
  {
    K_CONNECT::keep<bool> pred(ikeep);
    Vector_t<E_Int> pgnids, pgoids;
    ngon_unit pgs;
    c.extract_by_predicate(pred, pgs, pgoids, pgnids);
    c = pgs;
  }
  
  static void compact_to_used_nodes(cnt_t& c, K_FLD::FloatArray& crd, std::vector<E_Int>& nids)
  {
    nids.clear();
    ngon_type::compact_to_used_nodes(c, crd);
  }

  static K_FLD::FloatArray compact_to_used_nodes(cnt_t&c, K_FLD::FloatArray const & crdi, std::vector<E_Int>& nids)
  {
    return ngon_type::compress_to_used_nodes(c, crdi, nids);
  }

  static void compute_nodal_tolerance(const K_FLD::FloatArray& crd, const cnt_t& cnt, std::vector<E_Float>& nodal_tolerance)
  {
    K_FLD::FloatArray L;
    NUGA::MeshTool::computeIncidentEdgesSqrLengths(crd, cnt, L);
    L.extract_field(0, nodal_tolerance); // 0: extract the min
  }

  static void reverse_orient(cnt_t&c)
  {
    for (E_Int PGi = 0; PGi < c.size(); ++PGi)
    {
      E_Int s = c.stride(PGi);
      E_Int* p = c.get_facets_ptr(PGi);
      std::reverse(p, p + s);
    }
  }
};

// VOL 
template <>
struct connect_trait<VOLUMIC, false>
{
  using cnt_t = ngon_type;
  using elt_t = K_MESH::Polyhedron<UNKNOWN>; using aelt_t = NUGA::aPolyhedron<UNKNOWN>;
  using neighbor_t = ngon_unit;

  static const eGEODIM BOUND_GEODIM = SURFACIC;
  static const bool BOUND_STRIDE = false;
  static const E_Int index_start=1;

  static int ncells(const cnt_t& c) {return c.PHs.size();}
  static void shift(cnt_t& c, int v) { c.PGs.shift(v);}
  static void unique_indices(const cnt_t& c, std::vector<E_Int>& uinds) { c.PGs.unique_indices(uinds);}

  template <typename ELT_t> static void add(K_FLD::FloatArray& crd, cnt_t& c, ELT_t const& e)
  {
    E_Int ptshift = crd.cols();
    E_Int npgs0 = c.PGs.size();
    //E_Int nphs0 = c.PHs.size();

    crd.pushBack(e.m_crd);
    c.PGs.append(e.m_pgs);
    c.PGs.shift(ptshift, npgs0);
    c.PHs.add(e.m_faces.size(), &e.m_faces[0], npgs0);
  }

  static void build_neighbors(const cnt_t& c, neighbor_t& neighbors)
  {
    c.build_ph_neighborhood(neighbors);
  }

  static void get_boundary(cnt_t& c, ngon_unit& bc)
  {
    c.flag_externals(INITIAL_SKIN);
    Vector_t<E_Int> oids;
    c.PGs.extract_of_type(INITIAL_SKIN, bc, oids);
  }
  static void get_boundary(cnt_t& c, ngon_unit& bc, std::vector<E_Int>& ancestors)
  {
    c.flag_externals(INITIAL_SKIN);
    Vector_t<E_Int> oids;
    c.PGs.extract_of_type(INITIAL_SKIN, bc, oids);

    ancestors.clear();
    ancestors.resize(c.PGs.size(), E_IDX_NONE);
    for (E_Int i = 0; i < c.PHs.size(); ++i)
    {
      E_Int s = c.PHs.stride(i);
      const E_Int* f = c.PHs.get_facets_ptr(i);

      for (E_Int j = 0; j < s; ++j)
        ancestors[f[j] - 1] = i;
    }

    std::vector<E_Int> new_anc(bc.size(), E_IDX_NONE);
    for (size_t i = 0; i < new_anc.size(); ++i) new_anc[i] = ancestors[oids[i]];
    ancestors = new_anc;
  }

  static cnt_t compress_(cnt_t const& c, const std::vector<int>& keepids, int index_start)
  {
    std::vector<E_Int> oids;
    cnt_t ng;
    c.PHs.extract(keepids, ng.PHs, oids, index_start);

    ng.PGs = c.PGs;

    Vector_t<E_Int> pgnids, phnids;
    ng.remove_unreferenced_pgs(pgnids, phnids);

    return ng;
  }

  static void compress(cnt_t&c, const std::vector<bool>& ikeep)
  {
    K_CONNECT::keep<bool> pred(ikeep);
    Vector_t<E_Int> pgnids, pgoids;
    ngon_unit phs;
    c.PHs.extract_by_predicate(pred, phs, pgoids, pgnids);
    c.PHs = phs;

    Vector_t<E_Int> phnids;
    c.remove_unreferenced_pgs(pgnids, phnids);
  }

  static void compress(ngon_unit& neigh, const std::vector<bool>& ikeep)
  {
    connect_trait<SURFACIC, false>::compress(neigh, ikeep);
  }

  static void compact_to_used_nodes(cnt_t& c, K_FLD::FloatArray& crd, std::vector<E_Int>& nids)
  {
    nids.clear();
    ngon_type::compact_to_used_nodes(c.PGs, crd);
  }

  static K_FLD::FloatArray compact_to_used_nodes(cnt_t&c, K_FLD::FloatArray const & crdi, std::vector<E_Int>& nids)
  {
    return ngon_type::compress_to_used_nodes(c.PGs, crdi, nids);
  }

  static void compute_nodal_tolerance(const K_FLD::FloatArray& crd, const cnt_t& c, std::vector<E_Float>& nodal_tolerance)
  {
    // should be factorized in a behaviour class
    K_FLD::FloatArray L;
    NUGA::MeshTool::computeIncidentEdgesSqrLengths(crd, c.PGs, L);
    L.extract_field(0, nodal_tolerance); // 0: extract the min
  }
};


///
template <eGEODIM GEODIM, bool FIXSTRIDE>
struct mesh_t
{
  static const  eGEODIM ARG1 = GEODIM;
  static const  bool ARG2 = FIXSTRIDE;
  
  using trait = connect_trait<GEODIM, FIXSTRIDE>;
  template <bool USTRIDE> using bound_mesh_t = mesh_t<eGEODIM(GEODIM-1), USTRIDE>;
  template <bool USTRIDE> using bound_trait  = connect_trait<eGEODIM(GEODIM-1), USTRIDE>;
  template <bool USTRIDE> using bound_elt_t  = typename bound_mesh_t<USTRIDE>::elt_t;
  
  static const E_Int BOUND_STRIDE = trait::BOUND_STRIDE; //cannot use directly it in above definitions (instead of USTRIDE) as it does a infinite recursion => avoid it delcaring template
  static const E_Int index_start  = trait::index_start;

  using cnt_t = typename trait::cnt_t;
  
  using elt_t = typename trait::elt_t;
  using aelt_t = typename trait::aelt_t;
  
  using loc_t = localizer<K_SEARCH::BbTree3D>;
  using neighbor_t = typename trait::neighbor_t;
  
  K_FLD::FloatArray   crd;
  cnt_t               cnt;
  mutable std::vector<E_Int>   e_type, flag;
  mutable std::vector<E_Float> nodal_tolerance; // square dist
  mutable loc_t*               localiz;
  mutable neighbor_t*          neighbors;
  int                         oriented;

  // CONSTRUCTORS / DESTRUCTOR //

  mesh_t():localiz(nullptr), neighbors(nullptr), oriented(0){}

  mesh_t(const K_FLD::FloatArray &crd, const K_FLD::IntArray& cnt, int orient=0):crd(crd),cnt(cnt), localiz(nullptr), neighbors(nullptr), oriented(orient){}
  
  mesh_t& operator=(const mesh_t&m)
  {
    crd = m.crd; cnt = m.cnt;
    oriented = m.oriented;
    
    // these attribute are rferring to a previous state
    if (localiz != nullptr) delete localiz;
    if (neighbors != nullptr) delete neighbors;
    localiz   = nullptr;
    neighbors = nullptr;

    if (m.neighbors != nullptr)
    {
      neighbors  = new neighbor_t;
      *neighbors = *m.neighbors;
    }
    
    e_type = m.e_type;
    flag = m.flag;
    nodal_tolerance = m.nodal_tolerance;

    return *this;
  }

  ///
  mesh_t& operator=(const mesh_t &&m)
  {
    crd = std::move(m.crd);
    cnt = std::move(m.cnt);
    oriented = m.oriented;

    // these attribute are rferring to a previous state
    if (localiz != nullptr) delete localiz;
    if (neighbors != nullptr) delete neighbors;
    // steal new ones
    localiz = m.localiz;
    neighbors = m.neighbors;
    
    e_type = std::move(m.e_type);
    flag = std::move(m.flag);
    nodal_tolerance = std::move(m.nodal_tolerance);

    m.localiz = nullptr;
    m.neighbors = nullptr;

    return *this;
  }
  
  mesh_t(const mesh_t& m) :localiz(nullptr), neighbors(nullptr) { *this = m; }
  mesh_t(const mesh_t&& m):localiz(nullptr), neighbors(nullptr) { *this = m; }

  mesh_t(const mesh_t& m, std::vector<E_Int>& ids, int idx_start) :cnt(trait::compress_(m.cnt, ids, idx_start)), localiz(nullptr), neighbors(nullptr)
  {
    // fixme : hpc issue for more than lineic
    std::vector<E_Int> nids;
    crd = trait::compact_to_used_nodes(cnt, m.crd, nids);
    oriented = m.oriented;

    nodal_tolerance = K_CONNECT::IdTool::compact_(m.nodal_tolerance, nids); //sync the tolerance 

    //sync the neighborhood
    if (m.neighbors != nullptr)
    {
      neighbors = new neighbor_t;
      *neighbors = trait::compress_(*m.neighbors, ids, idx_start);
    }
  }

  // to create a bound mesh from a "parent" mesh
  template <bool USTRIDE>
  mesh_t(const mesh_t<eGEODIM(GEODIM+1), USTRIDE>& parent_mesh):crd(crd), localiz(nullptr), neighbors(nullptr), oriented(parent_mesh.oriented)
  {
    parent_mesh.get_boundary<FIXSTRIDE>(*this);
  }

  ~mesh_t()
  {
    if (localiz != nullptr) { delete localiz; localiz = nullptr; }
    if (neighbors != nullptr) { delete neighbors; neighbors = nullptr; }
  }

  // end constructors / destructor //

  // ACCESSORS //

  elt_t element(int i) const { return elt_t(cnt,i);}
  aelt_t aelement(int i) const 
  {  
    elt_t e(cnt, i);
    E_Float L2r = e.L2ref(nodal_tolerance);
    return aelt_t(e, crd, L2r);
  }
    
  template <bool BSTRIDE = BOUND_STRIDE> void get_boundary(int i, int j, bound_elt_t<BSTRIDE>& b) const
  {
    elt_t e(cnt, i);
    e.getBoundary(j, b);
  }
  
  int ncells() const {return trait::ncells(cnt);}

  // end accessors

  //

  template <bool BSTRIDE>
  void get_boundary(bound_mesh_t<BSTRIDE>& bound_mesh) 
  {
    trait::get_boundary(cnt, bound_mesh.cnt);
    bound_mesh.crd = crd;
    //compact crd to boundary only
    std::vector<E_Int> nids;
    bound_trait<BSTRIDE>::compact_to_used_nodes(bound_mesh.cnt, bound_mesh.crd, nids);
  }

  ///
  template <bool BSTRIDE>
  void get_boundary(bound_mesh_t<BSTRIDE>& bound_mesh, std::vector<int>& ancestors) 
  {
    trait::get_boundary(cnt, bound_mesh.cnt, ancestors);
    bound_mesh.crd = crd;
    //compact crd to boundary only
    std::vector<E_Int> nids;
    bound_trait<BSTRIDE>::compact_to_used_nodes(bound_mesh.cnt, bound_mesh.crd, nids);
  }
  
  ///
  template <bool BSTRIDE>
  void get_boundaries_by_color(std::map<E_Int, mesh_t<eGEODIM(GEODIM-1), BSTRIDE>>& col_to_boundmesh) const
  {
    using bound_t = mesh_t<eGEODIM(GEODIM-1), BSTRIDE>;
    using bcnt_t = typename bound_t::cnt_t;
    
    const neighbor_t* neighborz = get_neighbors(); // should be computed and set with colors before to have more than E_IDX_NONE
    
    std::map<E_Int, bcnt_t> col_to_boundcnt;
    trait::get_boundaries_by_color(cnt, *neighborz, col_to_boundcnt);
    
    for (auto it : col_to_boundcnt)
    {
      bcnt_t& bcn = it.second;
      bound_t& bmesh = col_to_boundmesh[it.first] = bound_t(crd, bcn);
      
      std::vector<E_Int> nids;
      bound_trait<BSTRIDE>::compact_to_used_nodes(bmesh.cnt, bmesh.crd, nids);
    }
    
  }
  
  /// 
  void reverse_orient()
  {
    trait::reverse_orient(cnt);
    if (oriented != 0) oriented = -oriented;
  }
  
  ///
  void append(const mesh_t<GEODIM, FIXSTRIDE>& m)
  {
    cnt_t mcnt = m.cnt;//fixme copy just because of the non-constness of the shift : shift should also have an option to start from an inex to make it at the end
    trait::shift(mcnt, crd.cols());
    
    crd.pushBack(m.crd);
    cnt.append(mcnt);
  }

  ///
  void add(aelt_t const & e)
  {
    trait::add(crd, cnt, e);
  }

  void clear()
  {
    crd.clear();
    cnt.clear();
  }

  template <typename T>
  void set_type(T typ, std::vector<int>& ids)
  {
    //std::cout << "set_type : 1" << std::endl;
    int nbcells = ncells();
    e_type.resize(nbcells, E_IDX_NONE);
    //std::cout << "set_type : nb cell : " << nbcells << std::endl;
    for (size_t i=0; i < ids.size(); ++i)
    {
      //std::cout << "id/ncell : " << ids[i] << "/" << nbcells << std::endl;
      if (ids[i] >= nbcells) continue; 
      e_type[ids[i]] = (E_Int)typ;
    }
    //std::cout << "set_type : 3" << std::endl;
  }
  
  void set_flag(int i, int val) const // fixme : constness here is bad design
  {
    if (i >= flag.size()) flag.resize(ncells(), E_IDX_NONE);
    flag[i] = val;
  }

  void bbox(K_SEARCH::BBox3D& box) const 
  {
    std::vector<E_Int> uinds;
    trait::unique_indices(cnt, uinds);
    box.compute(crd, uinds, index_start);
  }
  
  void bbox(int i, K_SEARCH::BBox3D& box) const 
  {
    assert(i < ncells());

    elt_t e = element(i);
    e.bbox(crd, box);
  }

  double L2ref() const
  {
    if (not nodal_tolerance.empty())
      return *std::min_element(ALL(nodal_tolerance));

    double minLref = K_CONST::E_MAX_FLOAT;
    for (E_Int i=0; i < ncells(); ++i)
    {
      elt_t e =element(i);
      minLref = std::min(e.L2ref(crd), minLref);
      //std::cout << "minlRef/i/ncel" << minLref << "/" << i << "/" << ncells() << std::endl;
    }
    return minLref;
  }
  
  double L2ref(int i) const
  { 
    elt_t e =element(i);
    
    if (not nodal_tolerance.empty())
      return e.L2ref(nodal_tolerance); // taking into acount surrounding elements
    // otherwise only based on this element
    return e.L2ref(crd);               // isolated val
  }

  double L2ref(const std::vector<E_Int>& cands, E_Int idx_start) const
  {
    double val = K_CONST::E_MAX_FLOAT;
    for (size_t i = 0; i < cands.size(); ++i)
    {
      val = std::min(val, L2ref(cands[i] - idx_start));
    }
    return val;
  }
  
  // WARNING : if T=bool, use predicate compress, if T=int, use keep as a list of indices to keep
  template <typename T>
  void compress(const std::vector<T>& keep)
  {
    trait::compress(cnt, keep);
    std::vector<E_Int> nids;
    trait::compact_to_used_nodes(cnt,crd, nids);
    
    K_CONNECT::IdTool::compact(nodal_tolerance, nids); //sync the tolerance 
    
    if (neighbors != nullptr)
      trait::compress(*neighbors, keep); //sync the neighborhood

    if (e_type.size() == keep.size())
      K_CONNECT::IdTool::compact(e_type, keep);
    else e_type.clear();

    if (flag.size() == keep.size())
      K_CONNECT::IdTool::compact(flag, keep);
    else flag.clear(); 
  }

  void build_localizer() const 
  {
    if (ncells() == 0) return;

    if (localiz != nullptr) delete localiz;

    using tree_t = K_SEARCH::BbTree3D;

    tree_t* t = new tree_t(crd, cnt);
    localiz = new loc_t(t, E_EPSILON/*fixme*/);   //not a leak : mask_loc owes t
  }

  const loc_t* get_localizer(void) const
  {
    if (localiz == nullptr) build_localizer();
    return localiz; //can be null if cnt is empty
  }

  void build_neighbors() const
  {
    if (ncells() == 0) return;
    if (neighbors != nullptr) delete neighbors;

    neighbors = new neighbor_t;
    trait::build_neighbors(cnt, *neighbors);
  }

  const neighbor_t* get_neighbors() const
  {
    if (neighbors != nullptr) { delete neighbors; neighbors = nullptr; }
    if (neighbors == nullptr) build_neighbors();
    return neighbors;
  }
  neighbor_t* get_neighbors()
  {
    if (neighbors == nullptr) build_neighbors();
    return neighbors;
  }

  const std::vector<E_Float>& get_nodal_tolerance() const
  {
    if (nodal_tolerance.empty() || ((E_Int)nodal_tolerance.size() != crd.cols()))
       build_nodal_tolerance(); // already computed and correctly sized
    return nodal_tolerance;
  }

  void build_nodal_tolerance() const
  {
    trait::compute_nodal_tolerance(crd, cnt, nodal_tolerance);

    // check if close point other than cnt (specially with folded surfaces)
    using acrd_t = K_FLD::ArrayAccessor<K_FLD::FloatArray>;
    acrd_t acrd(crd);
    K_SEARCH::KdTree<> tree(acrd);

    double d2;
    for (E_Int i=0; i < crd.cols(); ++i)
    {
      double r2 = (1. - E_EPSILON) * nodal_tolerance[i]; //reduc it
      int N = tree.getClosest(i, r2, d2);
      if (N != E_IDX_NONE)nodal_tolerance[i] = d2;
    }
  }

};

template <eGEODIM GEODIM, bool FIXSTRIDE>
struct vmesh_t
{
  using trait = connect_trait<GEODIM, FIXSTRIDE>;
  using cnt_t = typename trait::cnt_t;
  using elt_t = typename trait::elt_t;
  static const E_Int index_start = trait::index_start;
  
  const K_FLD::FloatArray&  crd;
  const cnt_t&              cnt;

  vmesh_t(const K_FLD::FloatArray &crd, const K_FLD::IntArray& cnt):crd(crd),cnt(cnt){}

  elt_t element(int i) const { return elt_t(cnt, i);}
  
  //mesh_t& operator=(const mesh_t&m){crd = m.crd; cnt = m.cnt;} 
  
  int ncells() const {return connect_trait<GEODIM, FIXSTRIDE>::ncells(cnt);}
  
  void bbox(K_SEARCH::BBox3D& box) const 
  {
    std::vector<E_Int> uinds;
    connect_trait<GEODIM, FIXSTRIDE>::unique_indices(cnt, uinds);
    box.compute(crd, uinds, index_start);
  }

  void bbox(int i, K_SEARCH::BBox3D& box) const 
  {
    assert(i < ncells());

    elt_t e = element(i);
    e.bbox(crd, box);
  }
  
};

template <typename MT>
using boundary_t = mesh_t<MT::trait::BOUND_GEODIM, MT::trait::BOUND_STRIDE>;

using ph_mesh_t = mesh_t<VOLUMIC, false>;
using pg_smesh_t  = mesh_t<SURFACIC,false>;
//using pure_basic_smesh_t = mesh_t<25, true>; 
using edge_mesh_t = mesh_t<LINEIC,true>; // == boundary_t< mesh_t<25, FIXSTRIDE>> qqsoit FIXSTRIDE

}

#endif
