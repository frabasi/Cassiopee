/*



--------- NUGA v1.0



*/
//Authors : Sâm Landier (sam.landier@onera.fr)

#ifndef NUGA_SUBDIVISION_HXX
#define NUGA_SUBDIVISION_HXX

#include<vector>
#include "Nuga/include/macros.h"
#include "Nuga/include/subdiv_defs.h"

#include "Nuga/include/Tetrahedron.h"
#include "Nuga/include/Hexahedron.h"
#include "Nuga/include/Prism.h"
#include "Nuga/include/Pyramid.h"
#include "Nuga/include/Basic.h"
#include "Nuga/include/Polyhedron.h"

#include "Nuga/include/Triangle.h"
#include "Nuga/include/Quadrangle.h"
#include "Nuga/include/Polygon.h"


namespace NUGA
{

  //
  template <typename ELT_t, eSUBDIV_TYPE STYPE>
  struct subdiv_pol;

  //
  template <>
  struct subdiv_pol<K_MESH::Hexahedron, ISO>
  {
    enum { PGNBC = 4, PHNBC = 8, NBI = 12 };

    using ph_arr_t = K_FLD::IntArray;
    using pg_arr_t = K_FLD::IntArray;
  };

  //
  template <>
  struct subdiv_pol<K_MESH::Tetrahedron, ISO>
  {
    enum { PGNBC = 4, PHNBC = 8, NBI = 8 };

    using ph_arr_t = K_FLD::IntArray;
    using pg_arr_t = K_FLD::IntArray;
  };

  //
  template <>
  struct subdiv_pol<K_MESH::Prism, ISO>
  {
    enum { PGNBC = 4, PHNBC = 8, NBI = 10 }; // NBI : 4 T3 + 6 Q4

    using ph_arr_t = K_FLD::IntArray;
    using pg_arr_t = K_FLD::IntArray;
  };

  //
  template <>
  struct subdiv_pol<K_MESH::Pyramid, ISO>
  {
    enum { PGNBC = 4, PHNBC = 10, NBI = 13 }; // NBI : 12 T3 + 1 Q4

    using ph_arr_t = K_FLD::IntArray;
    using pg_arr_t = K_FLD::IntArray;

  };

  // Basic - ISO
  template <>
  struct subdiv_pol<K_MESH::Basic, ISO>
  {
    enum { PGNBC = -1, PHNBC = -1/*, NBI = -1 */ };

    using ph_arr_t = ngon_unit;
    using pg_arr_t = K_FLD::IntArray;
  };

  // ISO_HEX Poyhedron subdivision => N HEXA children , with N is the nb of nodes
  template <>
  struct subdiv_pol<K_MESH::Polyhedron<0>, ISO_HEX>
  {
    enum { PGNBC = -1, PHNBC = -1/*, NBI = -1 */ };

    using ph_arr_t = ngon_unit;
    using pg_arr_t = ngon_unit;

    static E_Int nbc(const ngon_unit& PGS, const E_Int* first_pg, E_Int nb_pgs)
    {
      std::vector<E_Int> unodes;
      K_MESH::Polyhedron<UNKNOWN>::unique_nodes(PGS, first_pg, nb_pgs, unodes);
      return (E_Int)unodes.size();
    }

    static void nbc_list(const ngon_type& ng, const std::vector<E_Int>& PHlist, std::vector<E_Int>& pregnant)
    {
      E_Int nbPHtoadapt = PHlist.size();
      pregnant.resize(nbPHtoadapt);

      E_Int PHl, nb_pgs;
      const E_Int* first_pg;
      for (E_Int l = 0; l < nbPHtoadapt; l++)
      {
        PHl = PHlist[l];
        nb_pgs = ng.PHs.stride(PHl);
        first_pg = ng.PHs.get_facets_ptr(PHl);
        pregnant[l] = nbc(ng.PGs, first_pg, nb_pgs);
      }
    }

    static E_Int nbi(const ngon_unit& PGS, const E_Int* first_pg, E_Int nb_pgs)
    {
      return (K_MESH::Polyhedron<UNKNOWN>::cumulated_arity(PGS, first_pg, nb_pgs) / 2);
    }

    static E_Int nbi_sum(const ngon_type& ng, const std::vector<E_Int>& PHlist)
    {
      E_Int sum(0);
      E_Int nbPHtoadapt = PHlist.size();
      E_Int PHl, nb_pgs;
      const E_Int* first_pg;
      for (E_Int l = 0; l < nbPHtoadapt; l++)
      {
        PHl = PHlist[l];
        nb_pgs = ng.PHs.stride(PHl);
        first_pg = ng.PHs.get_facets_ptr(PHl);

        sum = sum + nbi(ng.PGs, first_pg, nb_pgs);
      }
      return sum;
    }
  };

  // DIR for HEXA
  template <>
  struct subdiv_pol < K_MESH::Hexahedron, DIR >
  {
    enum { PGNBC = -1, PHNBC = -1/*, NBI = -1 */ };

    using ph_arr_t = ngon_unit;
    using pg_arr_t = ngon_unit;

    static E_Int nbi_sum(const ngon_type& ng, const std::vector<E_Int>& PHlist, const std::vector<eDIR>& PH_directive)
    {
      E_Int sum(0);
      E_Int nbPHtoadapt = PHlist.size();
      E_Int PHl, nb_pgs;
      const E_Int* first_pg;
      for (E_Int l = 0; l < nbPHtoadapt; l++)
      {
        PHl = PHlist[l];
        const auto& dir = PH_directive[l];
        if (dir == XYZ) sum += 12;
        else if (dir == XY || dir == XZ || dir == YZ) sum += 4;
        else if (dir == Xd || dir == Y || dir == Z) sum += 1;
      }
      return sum;
    }

    static void nbc_list(const ngon_type& ng, const std::vector<E_Int>& PHlist, const std::vector<eDIR>& PH_directive, std::vector<E_Int>& pregnant)
    {
      E_Int nbPHtoadapt = PHlist.size();

      pregnant.clear();
      pregnant.resize(nbPHtoadapt);

      E_Int PHl, nb_pgs;
      const E_Int* first_pg;
      for (E_Int l = 0; l < nbPHtoadapt; l++)
      {
        PHl = PHlist[l];
        const auto& dir = PH_directive[l];

        if (dir == XYZ) pregnant[l] = 8;
        else if (dir == XY || dir == XZ || dir == YZ) pregnant[l] = 4;
        else if (dir == Xd || dir == Y || dir == Z) pregnant[l] = 2;
      }
    }
  };

  // isotropic HEXA subdivision => 4 Quadrangles children => fixed stride array
  template <>
  struct subdiv_pol<K_MESH::Quadrangle, ISO>
  {
    enum { NBC = 4 };
    using pg_arr_t = K_FLD::IntArray;

    static void reorder_children(E_Int* child, E_Int nchildren/*dummy*/, bool reverse, E_Int i0)
    {
      K_CONNECT::IdTool::right_shift<4>(&child[0], i0);
      if (reverse)
        std::swap(child[1], child[3]);
    }

  };

  // isotropic Triangle subdivision => 4 Trianges children => fixed stride array
  template <>
  struct subdiv_pol<K_MESH::Triangle, ISO>
  {
    enum { NBC = 4 };
    using pg_arr_t = K_FLD::IntArray;

    static void reorder_children(E_Int* child, E_Int nchildren/*dummy*/, bool reverse, E_Int i0)
    {
      K_CONNECT::IdTool::right_shift<3>(&child[0], i0);
      if (reverse)
        std::swap(child[1], child[2]);
    }
  };

  // directional QUAD subdivision => 2 Quadrangles children => fixed stride array
  template <>
  struct subdiv_pol<K_MESH::Quadrangle, DIR>
  {
    using pg_arr_t = ngon_unit;

    static void reorder_children(E_Int* child, E_Int nchildren/*dummy*/, bool reverse, E_Int i0)
    {
      //K_CONNECT::IdTool::right_shift<2>(&child[0], i0);
      //if (reverse)
        //std::swap(child[0], child[1]);
    }

    static E_Int nbc_list(const ngon_unit& PGs, const std::vector<E_Int>& PGlist, const std::vector<eDIR>& PG_directive, std::vector<E_Int>& pregnant)
    {
      pregnant.clear();
      pregnant.resize(PGlist.size());
      for (size_t i = 0; i < PG_directive.size(); ++i)
      {
        if (PG_directive[i] == XY) pregnant[i] = 4;
        else pregnant[i] = 2;
      }

      return 0;
    }

  };

  // directional Triangle subdivision => 2 Trianges children => fixed stride array
  template <>
  struct subdiv_pol<K_MESH::Triangle, DIR>
  {
    using pg_arr_t = ngon_unit;

    static void reorder_children(E_Int* child, E_Int nchildren/*dummy*/, bool reverse, E_Int i0)
    {
      K_CONNECT::IdTool::right_shift<2>(&child[0], i0);
      if (reverse)
        std::swap(child[0], child[1]);
    }
  };

  // ISO_HEX Polygon subdivision => N quad children , with N is the nb of nodes
  template <>
  struct subdiv_pol<K_MESH::Polygon, ISO_HEX>
  {
    enum { PGNBC = -1, PHNBC = -1/*, NBI = -1 */ };
    using pg_arr_t = ngon_unit;

    static void reorder_children(E_Int* child, E_Int nchildren, bool reverse, E_Int i0)
    {
      K_CONNECT::IdTool::right_shift(&child[0], nchildren, i0);
      if (reverse)
        std::reverse(child, child+nchildren);
    }

    static E_Int nbc_list(const ngon_unit& PGs, const std::vector<E_Int>& PGlist, const std::vector<eDIR>& PG_directive, std::vector<E_Int>& pregnant)
    {
      pregnant.clear();
      pregnant.resize(PGlist.size());

      for (size_t i = 0; i < PGlist.size(); ++i)
      {
        E_Int PGi = PGlist[i];
        pregnant[i] = PGs.stride(PGi);
      }
      return 0;
    }

  };

  template <E_Int DIM>
  struct dir_type
  {
    E_Int n[DIM];
    explicit dir_type(E_Int val) { n[0] = n[1] = val; if (DIM == 3) n[2] = val; }
    dir_type& operator=(E_Int val) { n[0] = n[1] = val; if (DIM == 3) n[2] = val; return *this; }
    dir_type& operator=(const dir_type& d) { n[0] = d.n[0];  n[1] = d.n[1]; n[2] = d.n[2]; return *this; }
    //E_Int operator+(E_Int v) const { return max() + v; }
    dir_type& operator+(E_Int val) { n[0] += val;  n[1] += val; if (DIM == 3) n[2] += val; return *this; }
    dir_type& operator--() { --n[0]; --n[1]; if (DIM == 3)--n[2]; return *this; }

    bool operator>=(E_Int v) const { return (max() >= v); }
    bool operator<=(E_Int v) const { return (max() <= v); }
    bool operator>(E_Int v) const { return (max() > v); }
    bool operator==(E_Int v) const { return (max() == v) && (min() == v); }
    bool operator!=(E_Int val) const {
      if (n[0] != val) return true;
      if (n[1] != val) return true;
      if ((DIM == 3) && (n[2] != val)) return true;
      return false;
    }
    //DANGEROUS because weird logic
    bool operator>(const dir_type& d) {
      if (n[0] > d.n[0]) return true;
      if (n[1] > d.n[1]) return true;
      if ((DIM == 3) && (n[2] > d.n[2])) return true;
      return false;
    }
    dir_type& operator+=(E_Int val) { n[0] = std::max(n[0] + val, 0); n[1] = std::max(n[1] + val, 0);; if (DIM == 3) n[2] = std::max(n[2] + val, 0); return *this; }

    dir_type operator-(const dir_type& d) { dir_type res(0);  res.n[0] = n[0] - d.n[0]; res.n[1] = n[1] - d.n[1]; if (DIM == 3) res.n[2] = n[2] - d.n[2]; return res; }

    E_Int max() const {
      if (DIM == 3) return std::max(n[0], std::max(n[1], n[2]));
      else return std::max(n[0], n[1]);
    }

    E_Int min() const {
      if (DIM == 3) return std::min(n[0], std::min(n[1], n[2]));
      else return std::min(n[0], n[1]);
    }
  };

  template <E_Int DIM> inline dir_type<DIM> max(dir_type<DIM>&d, E_Int v) { dir_type<DIM> res(0); res.n[0] = std::max(d.n[0], v); res.n[1] = std::max(d.n[1], v); if (DIM == 3) res.n[2] = std::max(d.n[2], v); return res; }//hack fr CLEF : l.362(hmesh.xhh)
  inline dir_type<3> abs(dir_type<3> d) { dir_type<3> res(0);  res.n[0] = ::abs(d.n[0]); res.n[1] = ::abs(d.n[1]); res.n[2] = ::abs(d.n[2]); return res; }
  inline dir_type<3> max(dir_type<3> a, dir_type<3> b) { dir_type<3> res(0); res.n[0] = std::max(a.n[0], b.n[0]); res.n[1] = std::max(a.n[1], b.n[1]); res.n[2] = std::max(a.n[2], b.n[2]); return res; }


  template <E_Int DIM> inline std::ostream &operator<<(std::ostream& out, const dir_type<DIM>& d)
  {
    out << d.n[0] << "/" << d.n[1];

    if (DIM == 3)
      out << "/" << d.n[2];
    out << std::endl;

    return out;
  }

  template <E_Int DIM>
  struct dir_vector_incr
  {
    using vec_t = Vector_t<dir_type<DIM>>;
    vec_t vec;
    dir_type<DIM>& operator[](E_Int i) { return vec[i]; };

    const dir_type<DIM>& operator[](E_Int i) const { return vec[i]; };
    size_t size() const { return vec.size(); }
    void clear() { vec.clear(); }
    void resize(E_Int sz, E_Int val) { dir_type<DIM> dt(val); vec.resize(sz, dt); }
    typename vec_t::iterator begin() { return vec.begin(); }
    typename vec_t::iterator end() { return vec.end(); }

    E_Int max(E_Int k) { return vec[k].max(); }
    E_Int min(E_Int k) { return vec[k].min(); }

    dir_vector_incr& operator=(const Vector_t<E_Int>& v)
    {
      resize(v.size(), 0);
      for (size_t k = 0; k < v.size(); ++k) { vec[k].n[0] = vec[k].n[1] = v[k]; if (DIM == 3) vec[k].n[2] = v[k]; }
      return *this;
    }
  };

  template <eSUBDIV_TYPE STYPE> // ISO impl
  struct incr_type
  {
    Vector_t<E_Int> cell_adap_incr;
    using pg_directive_type = Vector_t<E_Int>;
    pg_directive_type face_adap_incr;

    E_Int cmin(E_Int k) { return cell_adap_incr[k]; }
    E_Int cmax(E_Int k) { return cell_adap_incr[k]; }
    E_Int fmax(E_Int k) { return face_adap_incr[k]; }

    NUGA::eDIR get_face_dir(E_Int k) const
    {
      if (face_adap_incr[k] != 0) return XY;
      return NONE;
    }

  };

  template<>
  struct incr_type<DIR>
  {
    dir_vector_incr<3> cell_adap_incr;
    using pg_directive_type = dir_vector_incr<2>;
    pg_directive_type face_adap_incr;

    E_Int cmin(E_Int k) { return cell_adap_incr.min(k); }
    E_Int cmax(E_Int k) { return cell_adap_incr.max(k); }
    E_Int fmax(E_Int k) { return face_adap_incr.max(k); }

    NUGA::eDIR get_face_dir(E_Int k) const {

      if ((face_adap_incr[k].n[0] != 0) && (face_adap_incr[k].n[1] != 0)) return XY;
      if (face_adap_incr[k].n[0] != 0) return Xd;
      if (face_adap_incr[k].n[1] != 0) return Y;
      return NONE;
    }

  };


} // NUGA

#endif
