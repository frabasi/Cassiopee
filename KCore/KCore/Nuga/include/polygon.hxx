/*
 
 
 
              NUGA 
 
 
 
 */
//Author : Sâm Landier (sam.landier@onera.fr)

#ifndef NUGA_POLYGON_HXX
#define NUGA_POLYGON_HXX

#include "MeshElement/Polygon.h"

namespace NUGA
{
///autonomous (i.e. self contained) polygon
struct aPolygon : public K_MESH::Polygon
{
  using parent_type = K_MESH::Polygon;
  
  std::vector<E_Int>   m_nodes;
  K_FLD::FloatArray    m_crd;
  E_Float              m_L2ref;
  mutable E_Float      m_normal[3];
  mutable E_Float      m_centroid[3];
    
  aPolygon() = delete;
  aPolygon(const E_Int* nodes, E_Int nb_nodes, const K_FLD::FloatArray& crd) = delete; // from "mesh" to autonmous
  aPolygon(const parent_type& pg, const K_FLD::FloatArray& crd):parent_type(pg.begin(), pg.nb_nodes(), pg.shift()), m_L2ref(-1.)
  {
    K_CONNECT::MeshTool::compact_to_mesh(crd, _nodes, _nb_nodes, -_shift, m_crd);
    
    m_nodes.clear();
    K_CONNECT::IdTool::init_inc(m_nodes, pg.nb_nodes(), 0);
    
    parent_type::_nb_nodes = m_nodes.size();
    parent_type::_nodes = &m_nodes[0];
    _triangles = nullptr;
    parent_type::_shift = 0;
    m_normal[0] = m_centroid[0] = K_CONST::E_MAX_FLOAT;
  }
  
  aPolygon(const parent_type& pg, const K_FLD::FloatArray& crd, E_Float L2r) :aPolygon(pg, crd) { m_L2ref = L2r; m_normal[0] = m_centroid[0] = K_CONST::E_MAX_FLOAT; }

  aPolygon(K_FLD::FloatArray && crd):parent_type(nullptr, 0), m_crd(std::move(crd))
  {
    m_nodes.clear();
    K_CONNECT::IdTool::init_inc(m_nodes, m_crd.cols(), 0);

    parent_type::_nb_nodes = m_nodes.size();
    parent_type::_nodes = &m_nodes[0];
    _triangles = nullptr;
    parent_type::_shift = 0;
    m_normal[0] = m_centroid[0] = K_CONST::E_MAX_FLOAT;
  }

  aPolygon(const ngon_unit& ngu, E_Int ith, const K_FLD::FloatArray& crd): parent_type(nullptr, 0)
  {
    parent_type::_nb_nodes = ngu.stride(ith);
    _triangles = nullptr;
    parent_type::_shift = 0;
    m_normal[0] = m_centroid[0] = K_CONST::E_MAX_FLOAT;

    K_CONNECT::MeshTool::compact_to_mesh(crd, ngu.get_facets_ptr(ith), parent_type::_nb_nodes, 1, m_crd);
    
    m_nodes.clear();
    K_CONNECT::IdTool::init_inc(m_nodes, parent_type::_nb_nodes, 0);

    parent_type::_nodes = &m_nodes[0];
  }
    
  aPolygon& operator=(const parent_type& rhs) = delete;
  aPolygon& operator=(const aPolygon& rhs)
  {
    m_nodes = rhs.m_nodes;
    m_crd = rhs.m_crd;
    m_L2ref = rhs.m_L2ref;

    parent_type::_nb_nodes = m_nodes.size();
    parent_type::_nodes = &m_nodes[0];
    _triangles = nullptr;
    parent_type::_shift = 0;

    m_normal[0] = rhs.m_normal[0];
    m_normal[1] = rhs.m_normal[1];
    m_normal[2] = rhs.m_normal[2];

    m_centroid[0] = rhs.m_centroid[0];
    m_centroid[1] = rhs.m_centroid[1];
    m_centroid[2] = rhs.m_centroid[2];

    return *this;
  }

  aPolygon(aPolygon&& rhs) :/* = default; rejected by old compiler intel (15)*/
  parent_type(rhs), m_nodes(std::move(rhs.m_nodes)), m_crd(std::move(rhs.m_crd)), m_L2ref(rhs.m_L2ref)
  {
    m_normal[0] = rhs.m_normal[0];
    m_normal[1] = rhs.m_normal[1];
    m_normal[2] = rhs.m_normal[2];

    m_centroid[0] = rhs.m_centroid[0];
    m_centroid[1] = rhs.m_centroid[1];
    m_centroid[2] = rhs.m_centroid[2];
  }

  aPolygon& operator=(aPolygon&& rhs)
  {
    m_nodes = std::move(rhs.m_nodes);
    m_crd = std::move(rhs.m_crd);
    m_L2ref = rhs.m_L2ref;

    parent_type::_nb_nodes = m_nodes.size();
    parent_type::_nodes = &m_nodes[0];
    _triangles = rhs._triangles;
    parent_type::_shift = rhs._shift;

    m_normal[0] = rhs.m_normal[0];
    m_normal[1] = rhs.m_normal[1];
    m_normal[2] = rhs.m_normal[2];

    m_centroid[0] = rhs.m_centroid[0];
    m_centroid[1] = rhs.m_centroid[1];
    m_centroid[2] = rhs.m_centroid[2];

    rhs._nb_nodes = 0;
    rhs._shift = 0;
    rhs._triangles = nullptr;
    rhs._nodes = nullptr;
    rhs.m_normal[0] = K_CONST::E_MAX_FLOAT;//fixme: necessary ?

    return *this;
  }
  
  template <short DIM> void normal(double* norm) const 
  {
    parent_type::normal<K_FLD::FloatArray, DIM>(m_crd, norm);
  }

  template <short DIM> void centroid(double* G) const
  {
    parent_type::centroid<DIM>(m_crd, _nodes, _nb_nodes, 0, G);
  }
  
  template <typename TriangulatorType>
  E_Int triangulate(const TriangulatorType& dt) const
  {
    return parent_type::triangulate(dt, m_crd);
  }

  double extent() const
  {
    return parent_type::surface<K_FLD::FloatArray, 3>(m_crd, parent_type::_nodes, parent_type::_nb_nodes, parent_type::_shift);
  }

  double metrics() const
  {
    normal<3>(m_normal);
    return extent();
  }

  const double* get_normal() const { 
    if (m_normal[0] == K_CONST::E_MAX_FLOAT)
      normal<3>(m_normal);
    return m_normal;
  }

  const double* get_centroid() const {
    if (m_centroid[0] == K_CONST::E_MAX_FLOAT)
      parent_type::centroid<3>(m_crd, parent_type::_nodes, parent_type::_nb_nodes, parent_type::_shift, m_centroid);
    return m_centroid;
  }

  double L2ref() const { return (m_L2ref > 0.) ? m_L2ref : parent_type::L2ref(m_crd);} // if passed by mesh_t, return it, otherwise compute it first
};

}

#endif /* POLYGON_HXX */

