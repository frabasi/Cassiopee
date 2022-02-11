/*    
    Copyright 2013-2022 Onera.

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

# include "intersector.h"
# include "stub.h"

# include "Nuga/include/ngon_t.hxx"
# include "Nuga/include/Triangulator.h"


//=============================================================================
/* Creates 4 zones : 1) uncomputable polygons 2) uncomputable polyhedra 
   3) uncomputable polyhedra & neighbors 4) complementary of 3) */
//=============================================================================
PyObject* K_INTERSECTOR::extractUncomputables(PyObject* self, PyObject* args)
{
  E_Int neigh_level(1);
  PyObject *arr;

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &neigh_level)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt), uphs/*uncomputable phs*/, uphs_wv1/*uncomputable phs with neighbors*/, remaining;
  ngon_unit upgs; //uncomputable polygons

  err = ngon_type::extract_uncomputables<DELAUNAY::Triangulator>(crd, ngi, neigh_level, upgs, uphs, uphs_wv1, remaining);
  
  if (err)
  {
    PyErr_SetString(PyExc_TypeError, "extract_uncomputables failed.");
    delete f; delete cn;
    return NULL;
  } 
  
  PyObject *l(PyList_New(0)), *tpl;

  if (upgs.size() == 0)
  {
    std::cout << "OK : there are no uncomputable polygons." << std::endl;
    PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnt, -1, eltType, false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }
  else
  {
    // zone 1 : uncomputable pgs
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(upgs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      ngon_type ngo(upgs, false/*one ph per pg*/);
      ngo.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
    
    // zone 2 : uncomputable phs
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(uphs.PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      uphs.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
    
    // zone 3 : uncomputable phs and first neighborhood
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(uphs_wv1.PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      uphs_wv1.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
    // zone 4 : complementary of above selection
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(remaining.PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      remaining.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
  }
  
  
  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX*/
//=============================================================================
PyObject* K_INTERSECTOR::extractPathologicalCells(PyObject* self, PyObject* args)
{
  E_Int neigh_level(2);
  PyObject *arr;

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &neigh_level)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt), neigh_phs/*patho neighbors*/, remaining_phs;
  std::vector<ngon_type> phsv;

  //std::cout << "neigh level : " <<  neigh_level << std::endl;
 
  err = ngon_type::extract_pathological_PHs<DELAUNAY::Triangulator>(crd, ngi, neigh_level, 
                                                                    phsv, neigh_phs, remaining_phs);
  
  if (err)
  {
    PyErr_SetString(PyExc_TypeError, "extract_pathological_phs failed.");
    delete f; delete cn;
    return NULL;
  } 
  
  PyObject *l(PyList_New(0)), *tpl;

  if (phsv.empty())
  {
    std::cout << "OK : all the cells are star-shaped regarding there centroids." << std::endl;
    tpl = K_ARRAY::buildArray(crd, varString, cnt, -1, eltType, false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }
  else
  {
    E_Int nphs = phsv[0].PHs.size();
    //if (nphs)
    // zone 1 : 
    {
      if (nphs) std::cout << "there are " << nphs << " open cells (bug somewhere)." << std::endl;

      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(phsv[0].PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      phsv[0].export_to_array(cnto);
      //patho_name = "open_cells";
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
    
    nphs = phsv[1].PHs.size();
    //if (nphs)
    // zone 2 : 
    {
      if (nphs) std::cout << "there are " << nphs << " cells with degenerated polygons (showstopper?)." << std::endl;

      K_FLD::FloatArray crdtmp(crd);

      ngon_type::compact_to_used_nodes(phsv[1].PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      phsv[1].export_to_array(cnto);

      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }

    nphs = phsv[2].PHs.size();
    //if (nphs)
    // zone 3 : 
    {
      if (nphs) std::cout << "there are " << phsv[2].PHs.size() << " cells with some  delaunay-failure polygons." << std::endl;
      K_FLD::FloatArray crdtmp(crd);

      ngon_type::compact_to_used_nodes(phsv[2].PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      phsv[2].export_to_array(cnto);

      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
    
    nphs = phsv[3].PHs.size();
    if (nphs) std::cout << "there are " << phsv[3].PHs.size() << " non-centroid-star-shaped cells that can be split." << std::endl;
    // zone 4 : 
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(phsv[3].PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      phsv[3].export_to_array(cnto);

      //std::cout << "pg : pg " << phsv[3].PGs.size() << " versus " << cnto[0] << std::endl;

      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }

    // zone 5 : neighbors
    // {
    //   K_FLD::FloatArray crdtmp(crd);
    //   ngon_type::compact_to_used_nodes(neigh_phs.PGs, crdtmp); //reduce points
    //   //export to numpy
    //   K_FLD::IntArray cnto;
    //   neigh_phs.export_to_array(cnto);
    //   tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
    //   PyList_Append(l, tpl);
    //   Py_DECREF(tpl);
    // }

    // // zone 6 : complementary of above selections
    // if (remaining_phs.PHs.size())
    // {
    //   K_FLD::FloatArray crdtmp(crd);
    //   ngon_type::compact_to_used_nodes(remaining_phs.PGs, crdtmp); //reduce points
    //   //export to numpy
    //   K_FLD::IntArray cnto;
    //   remaining_phs.export_to_array(cnto);
    //   tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
    //   PyList_Append(l, tpl);
    //   Py_DECREF(tpl);
    // }
  }
  
  
  delete f; delete cn;
  return l;
}


//=============================================================================
/* Creates 2 zones : 1) outerlayer with firt neighborhoo 2) complementary */
//=============================================================================
PyObject* K_INTERSECTOR::extractOuterLayers(PyObject* self, PyObject* args)
{

  PyObject *arr;
  E_Int N(1), discard_external(0);

  if (!PYPARSETUPLEI(args, "Oll", "Oii", &arr, &N, &discard_external)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt), outer, remaining;
 
  err = ngon_type::extract_n_outer_layers(crd, ngi, N, outer, remaining, discard_external);
  
  if (err)
  {
    PyErr_SetString(PyExc_TypeError, "extract_outer_layers failed.");
    delete f; delete cn;
    return NULL;
  } 
  
  PyObject *l(PyList_New(0)), *tpl;

  {
    //std::cout << "there are " << outer.PHs.size() << " outer cells detected." << std::endl;
    // zone 1 : outer
    if (outer.PHs.size())
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(outer.PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      outer.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
    
    // zone 2 : complementary 
    if (remaining.PHs.size())
    {
      K_FLD::FloatArray crdtmp(crd);
      ngon_type::compact_to_used_nodes(remaining.PGs, crdtmp); //reduce points
      //export to numpy
      K_FLD::IntArray cnto;
      remaining.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, -1, eltType, false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
  }
  
  
  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extractNthCell(PyObject* self, PyObject* args)
{

  PyObject *arr;
  E_Int nth(0);

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &nth)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (nth >= ngi.PHs.size())
  {
    std::cout << "ERROR " << nth << " : is out of range" << std::endl;
    return NULL;
  }
 
  
  ngon_unit ph;
  ph.add(ngi.PHs.stride(nth), ngi.PHs.get_facets_ptr(nth));

  PyObject *l(PyList_New(0)), *tpl;
  
  // Extract the cell
  {
    ngon_type one_ph(ngi.PGs, ph);
    std::vector<E_Int> pgnids, phnids;
    one_ph.remove_unreferenced_pgs(pgnids, phnids);
    K_FLD::FloatArray crdtmp(crd);//we need crd for the neighbor zone
    ngon_type::compact_to_used_nodes(one_ph.PGs, crdtmp);

    K_FLD::IntArray cnto;
    one_ph.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }
  // Exract also its neighbors
  {
    std::vector<bool> keepPG(ngi.PGs.size(), false), keepPH;
    E_Int nb_pgs = ngi.PHs.stride(nth);
    E_Int* pgs = ngi.PHs.get_facets_ptr(nth);
    for (E_Int p=0; p < nb_pgs; ++p) keepPG[*(pgs+p)-1] = true;
    ngi.flag_PHs_having_PGs(keepPG, keepPH);

    keepPH[nth] = false;
    for (size_t i=0; i < keepPH.size(); ++i)
      if (keepPH[i]) std::cout << "neighbor : " << i << std::endl;

    ngon_type ngo;
    std::vector<E_Int> pgnids, phnids;
    ngon_type::select_phs(ngi, keepPH, pgnids, ngo);

    ngo.remove_unreferenced_pgs(pgnids, phnids);
    ngon_type::compact_to_used_nodes(ngo.PGs, crd);

    for (E_Int i=0; i < ngo.PHs.size(); ++i)
    {
      ngon_type one_ph;
      one_ph.addPH(ngo, i, true);

      K_FLD::IntArray cnto;
      one_ph.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
  }

  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extractBiggestCell(PyObject* self, PyObject* args)
{

  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  K_FLD::ArrayAccessor<K_FLD::FloatArray> acrd(crd);

  E_Int nth=-1;
  E_Float dm(0.);
  for (E_Int i=0; i < ngi.PHs.size(); ++i)
  {
    K_SEARCH::BBox3D box;
    K_MESH::Polyhedron<0> PH(&ngi.PGs, ngi.PHs.get_facets_ptr(i), ngi.PHs.stride(i));
    PH.bbox(acrd, box);

    E_Float dx0 = box.maxB[0]-box.minB[0];
    E_Float dx1 = box.maxB[1]-box.minB[1];
    E_Float dx2 = box.maxB[2]-box.minB[2];

    if ( (dm < dx0) || (dm < dx1) || (dm < dx2))
    {
      dm = std::max(dx0, std::max(dx1,dx2));
      nth = i;
    }

  }
 
  std::cout << "biggest cell is : " << nth << std::endl;
  
  ngon_unit ph;
  ph.add(ngi.PHs.stride(nth), ngi.PHs.get_facets_ptr(nth));

  PyObject *l(PyList_New(0)), *tpl;
  
  // Extract the cell
  {
    ngon_type one_ph(ngi.PGs, ph);
    std::vector<E_Int> pgnids, phnids;
    one_ph.remove_unreferenced_pgs(pgnids, phnids);
    K_FLD::FloatArray crdtmp(crd);//we need crd for the neighbor zone
    ngon_type::compact_to_used_nodes(one_ph.PGs, crdtmp);

    K_FLD::IntArray cnto;
    one_ph.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }
  // Exract also its neighbors
  {
    std::vector<bool> keepPG(ngi.PGs.size(), false), keepPH;
    E_Int nb_pgs = ngi.PHs.stride(nth);
    E_Int* pgs = ngi.PHs.get_facets_ptr(nth);
    for (E_Int p=0; p < nb_pgs; ++p) keepPG[*(pgs+p)-1] = true;
    ngi.flag_PHs_having_PGs(keepPG, keepPH);

    keepPH[nth] = false;
    for (size_t i=0; i < keepPH.size(); ++i)
      if (keepPH[i]) std::cout << "neighbor : " << i << std::endl;

    ngon_type ngo;
    std::vector<E_Int> pgnids, phnids;
    ngon_type::select_phs(ngi, keepPH, pgnids, ngo);

    ngo.remove_unreferenced_pgs(pgnids, phnids);
    ngon_type::compact_to_used_nodes(ngo.PGs, crd);

    for (E_Int i=0; i < ngo.PHs.size(); ++i)
    {
      ngon_type one_ph;
      one_ph.addPH(ngo, i, true);

      K_FLD::IntArray cnto;
      one_ph.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
  }

  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::removeNthCell(PyObject* self, PyObject* args)
{

  PyObject *arr;
  E_Int nth(0);

  if (!PyArg_ParseTuple(args, "Ol", &arr, &nth)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (nth >= ngi.PHs.size())
  {
    std::cout << "ERROR " << nth << " : is out of range" << std::endl;
    return NULL;
  }
  
  ngon_unit phs;
  for (E_Int i = 0; i < ngi.PHs.size(); ++i)
  {
    if (i == nth) continue;
    phs.add(ngi.PHs.stride(i), ngi.PHs.get_facets_ptr(i));
  }
  
  ngon_type ng(ngi.PGs, phs);

  std::vector<E_Int> pgnids, phnids;
  ng.remove_unreferenced_pgs(pgnids, phnids);
  ngon_type::compact_to_used_nodes(ng.PGs, crd);
  
  K_FLD::IntArray cnto;
  ng.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::detectIdenticalCells(PyObject* self, PyObject* args)
{

  //std::cout << "detectIdenticalCells : begin" << std::endl;

  PyObject *arr;
  E_Int clean(0);
  E_Float tol(1.e-15);

  if (!PyArg_ParseTuple(args, "Odl", &arr, &tol, &clean)) return NULL;

  //std::cout << "detectIdenticalCells : after parse" << std::endl;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  Vector_t<E_Int> nids;
  ngi.detect_phs_with_same_centroid (crd, nids);
  bool found=false;

  E_Int nb_phs = ngi.PHs.size();
  for (E_Int i = 0; i < nb_phs; ++i)
  {
    if (nids[i] != i)
    {
      std::cout << "detectIdenticalCells : " << i << " is identical to " << nids[i] << std::endl;
      found=true;
    }
  }

  if (!found)
    std::cout << "detectIdenticalCells : OK. No duplicates found." << std::endl;

  if (!clean || !found)
  {
    delete f; delete cn;
    return arr;
  }

  //std::cout << "detectIdenticalCells : clean" << std::endl;

  ngon_unit phs;
  for (E_Int i = 0; i < nb_phs; ++i)
  {
    if (nids[i] != i) continue;
    phs.add(ngi.PHs.stride(i), ngi.PHs.get_facets_ptr(i));
  }

  //std::cout << "detectIdenticalCells : create output" << std::endl;
  
  ngon_type ng(ngi.PGs, phs);

  std::vector<E_Int> pgnids, phnids;
  ng.remove_unreferenced_pgs(pgnids, phnids);
  ngon_type::compact_to_used_nodes(ng.PGs, crd);

  //std::cout << "detectIdenticalCells : build array" << std::endl;
  
  K_FLD::IntArray cnto;
  ng.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);

  //std::cout << "detectIdenticalCells : end" << std::endl;
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::detectOverConnectedFaces(PyObject* self, PyObject* args)
{

  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  E_Int nb_phs = ngi.PHs.size();

  std::vector<E_Int> pgs_occurrences(ngi.PGs.size(), 0);
  for (E_Int i=0; i < nb_phs; ++i)
  {
    const E_Int * faces = ngi.PHs.get_facets_ptr(i);
    E_Int stride = ngi.PHs.stride(i);

    for(E_Int n=0; n < stride; ++n)
    {
      E_Int PGi = *(faces+n)-1;
      ++pgs_occurrences[PGi];
    }
  }

  bool error = false;
  for (size_t i=0; i < pgs_occurrences.size(); ++i)
  {
    if (pgs_occurrences[i] > 2)
    {
      std::cout << "multi PG : " << i << std::endl;
      error = true;
    }
  }

  if (!error)
    std::cout << "OK : no multiple PGs" << std::endl;
  
  K_FLD::IntArray cnto;
  ngi.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extractNthFace(PyObject* self, PyObject* args)
{

  PyObject *arr;
  E_Int nth(0);

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &nth)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (nth >= ngi.PGs.size())
  {
    std::cout << "ERROR " << nth << " : is out of range" << std::endl;
    return NULL;
  }

  PyObject *l(PyList_New(0)), *tpl;

  // Extract the face
  {
    ngon_unit pg;
    pg.add(ngi.PGs.stride(nth), ngi.PGs.get_facets_ptr(nth));

    ngon_type one_ph(pg, true);

    K_FLD::FloatArray crdtmp(crd); //we need crd for the parent elements zone
    ngon_type::compact_to_used_nodes(one_ph.PGs, crdtmp);

    K_FLD::IntArray cnto;
    one_ph.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }
  // Extract the Elements sharing this face
  {
    std::vector<bool> keepPG(ngi.PGs.size(), false);
    keepPG[nth]=true;
    std::vector<bool> keepPH;
    ngi.flag_PHs_having_PGs(keepPG, keepPH);

    for (size_t i=0; i < keepPH.size(); ++i)
      if (keepPH[i]) std::cout << "sharing element : " << i << std::endl;

    ngon_type ngo;
    std::vector<E_Int> pgnids, phnids;
    ngon_type::select_phs(ngi, keepPH, pgnids, ngo);

    ngo.remove_unreferenced_pgs(pgnids, phnids);
    ngon_type::compact_to_used_nodes(ngo.PGs, crd);

    for (E_Int i=0; i < ngo.PHs.size(); ++i)
    {
      ngon_type one_ph;
      one_ph.addPH(ngo, i, true);

      K_FLD::IntArray cnto;
      one_ph.export_to_array(cnto);
      tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
      PyList_Append(l, tpl);
      Py_DECREF(tpl);
    }
  }

  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::checkCellsClosure(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  //K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  err = ngon_type::check_phs_closure(ngi);

  delete f; delete cn;

#ifdef E_DOUBLEINT
    return Py_BuildValue("l", long(err));
#else
    return Py_BuildValue("i", err);
#endif
}

PyObject* K_INTERSECTOR::checkCellsFlux(PyObject* self, PyObject* args)
{
  PyObject *arr, *PE;

  if (!PyArg_ParseTuple(args, "OO", &arr, &PE)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  // Check numpy (parentElement)
  FldArrayI* cFE;
  E_Int res = K_NUMPY::getFromNumpyArray(PE, cFE, true);

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (ngi.PGs.size() != cFE->getSize())
  {
    std::cout << "le ParentElment ne correpsond pas au nb de pgs" << std::endl;
    delete f; delete cn;
    return nullptr;
  }

  std::vector<E_Int> orient;
  E_Int imax=-1;
  E_Float fluxmax = -1;
  for (E_Int i=0; i < ngi.PHs.size(); ++i)
  {
    orient.clear();

    const E_Int* pF = ngi.PHs.get_facets_ptr(i);
    E_Int nbf = ngi.PHs.stride(i);
    orient.resize(nbf, 1);

    for (E_Int f = 0; f < nbf; ++f)
    {
      E_Int PGi = *(pF+f) - 1;
      //std::cout << "PGi bef wwong :" << PGi << std::endl;
      if ((*cFE)(PGi, 1) != i+1) orient[f] = -1;
      assert (((*cFE)(PGi, 1) == i+1) || ((*cFE)(PGi, 2) == i+1) );
    }

    //std::cout << "computing flux for PH : " << i << std::endl;
    K_MESH::Polyhedron<0> PH(ngi, i);
    E_Float flxVec[3];
    PH.flux(crd, &orient[0], flxVec);

    E_Float flux = ::sqrt(K_FUNC::sqrNorm<3>(flxVec));
    E_Float s = PH.surface(crd);

    flux /= s; // normalizing

    if (flux > fluxmax)
    {
      imax = i;
      fluxmax = flux;
    }
  }

  std::cout << "normalized max flux is : " << fluxmax << " reached at cell : " << imax << std::endl;

  delete f; delete cn;

  PyObject *l(PyList_New(0)), *tpl;

#ifdef E_DOUBLEINT
  tpl =  Py_BuildValue("l", long(imax));
   PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("i", imax);
  PyList_Append(l, tpl);
#endif

#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(fluxmax));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", float(fluxmax);
  PyList_Append(l, tpl);
#endif
  
  return l;
}

int comp_vol(const K_FLD::FloatArray& crd, const ngon_type& ngi, const FldArrayI* cFE, std::vector<E_Int>& orient, E_Int i, DELAUNAY::Triangulator & dt, double &v)
{
  //std::cout << "PH : " << i << std::endl;
  orient.clear();

  const E_Int* pF = ngi.PHs.get_facets_ptr(i);
  E_Int nbf = ngi.PHs.stride(i);
  orient.resize(nbf, 1);

  for (E_Int j = 0; j < nbf; ++j)
  {
    E_Int PGi = *(pF+j) - 1;
    //std::cout << "PGi bef wwong :" << PGi << std::endl;
    if ((*cFE)(PGi, 1) != i+1) orient[j] = -1;
    //assert (((*cFE)(PGi, 1) == i+1) || ((*cFE)(PGi, 2) == i+1) );
  }

  //std::cout << "computing flux for PH : " << i << std::endl;
  K_MESH::Polyhedron<0> PH(ngi, i);
  
  E_Int err = PH.volume<DELAUNAY::Triangulator>(crd, &orient[0], v, dt);

  return err;
}

PyObject* K_INTERSECTOR::checkCellsVolume(PyObject* self, PyObject* args)
{
  PyObject *arr, *PE;

  if (!PyArg_ParseTuple(args, "OO", &arr, &PE)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  // Check numpy (parentElement)
  FldArrayI* cFE;
  E_Int res = K_NUMPY::getFromNumpyArray(PE, cFE, true);

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (ngi.PGs.size() != cFE->getSize())
  {
    std::cout << "le ParentElment ne correpsond pas au nb de pgs" << std::endl;
    delete f; delete cn;
    return nullptr;
  }

  E_Int imin=-1;
  E_Float vmin = NUGA::FLOAT_MAX;

  E_Int nb_max_threads = __NUMTHREADS__;
  //std::cout << "nb threads max : " << nb_max_threads << std::endl;
    
  std::vector<E_Int> im(nb_max_threads, IDX_NONE);
  std::vector<E_Float> vm(nb_max_threads, NUGA::FLOAT_MAX);
  std::vector<std::vector<E_Int>> orient(nb_max_threads);

  E_Int i, id{0};
  DELAUNAY::Triangulator dt;

#pragma omp parallel shared(vm, im, ngi, crd, cFE, orient) private (i, id, dt) default(none)
{
  id = __CURRENT_THREAD__;
  //std::cout << "before loop thread : " << id  << std::endl;
#pragma omp for //schedule(dynamic)
  for (i=0; i < ngi.PHs.size(); ++i)
  {
    double v;
    E_Int err = comp_vol(crd, ngi, cFE, orient[id], i, dt, v);
    if (!err && v < vm[id]) // min for current thread
    {
      im[id] = i;
      vm[id] = v;
    }
  }
}

  for (E_Int i=0; i < nb_max_threads; ++i)
  {
    if (vm[i] < vmin)
    {
      imin = im[i];
      vmin = vm[i];
    }
  }

  std::cout << "min vol is : " << vmin << " reached at cell : " << imin << std::endl;

  delete f; delete cn;

  PyObject *l(PyList_New(0)), *tpl;

#ifdef E_DOUBLEINT
  tpl =  Py_BuildValue("l", long(imin));
   PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("i", imin);
  PyList_Append(l, tpl);
#endif

#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(vmin));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", float(vmin);
  PyList_Append(l, tpl);
#endif
  
  return l;
}

PyObject* K_INTERSECTOR::checkCellsVolumeAndGrowthRatio(PyObject* self, PyObject* args)
{
  PyObject *arr, *PE;
  double aratio{0.125}, vmin{0.};
  int nneighs{1};

  if (!PyArg_ParseTuple(args, "OO", &arr, &PE)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  // Check numpy (parentElement)
  FldArrayI* cFE;
  E_Int res = K_NUMPY::getFromNumpyArray(PE, cFE, true);

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (ngi.PGs.size() != cFE->getSize())
  {
    std::cout << "le ParentElement ne correpsond pas au nb de pgs" << std::endl;
    delete f; delete cn;
    return nullptr;
  }

  E_Int nphs = ngi.PHs.size();

  std::vector<double> vols(nphs, NUGA::FLOAT_MAX);

  E_Int nb_max_threads = __NUMTHREADS__;
  //std::cout << "nb threads max : " << nb_max_threads << std::endl;
    
  std::vector<std::vector<E_Int>> orient(nb_max_threads);

  E_Int i, id{0};
  DELAUNAY::Triangulator dt;

#pragma omp parallel shared(ngi, crd, cFE, orient, vols) private (i, id, dt) default(none)
  {
    id = __CURRENT_THREAD__;
    //std::cout << "before loop thread : " << id  << std::endl;
#pragma omp for //schedule(dynamic)
    for (i=0; i < ngi.PHs.size(); ++i)
    {
      double v;
      E_Int err = comp_vol(crd, ngi, cFE, orient[id], i, dt, v);
      if (!err)
        vols[i] = v;
    }
  }

  //
  ngon_unit neighborsi;
  ngi.build_ph_neighborhood(neighborsi);

  Vector_t<E_Float> growth_ratio;
  ngon_type::stats_bad_volumes<DELAUNAY::Triangulator>(crd, ngi, neighborsi, vols, -1., growth_ratio);

  E_Int ivolmin,igrmin;
  E_Float volmin{NUGA::FLOAT_MAX}, grmin{NUGA::FLOAT_MAX};

  for (size_t i=0; i < vols.size(); ++i)
  {
    if (vols[i] < volmin)
    {
      volmin = vols[i];
      ivolmin = i;
    }
  }

  for (size_t i=0; i < growth_ratio.size(); ++i)
  {
    if (growth_ratio[i] < grmin)
    {
      grmin = growth_ratio[i];
      igrmin = i;
    }
  }
 
  delete f; delete cn;
 
  PyObject *l(PyList_New(0)), *tpl;

#ifdef E_DOUBLEINT
  tpl =  Py_BuildValue("l", long(ivolmin));
   PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("i", ivolmin);
  PyList_Append(l, tpl);
#endif

#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(volmin));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", float(volmin);
  PyList_Append(l, tpl);
#endif

#ifdef E_DOUBLEINT
  tpl =  Py_BuildValue("l", long(igrmin));
   PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("i", igrmin);
  PyList_Append(l, tpl);
#endif

#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(grmin));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", float(grmin);
  PyList_Append(l, tpl);
#endif
  
  return l;
}

PyObject* K_INTERSECTOR::extractBadVolCells(PyObject* self, PyObject* args)
{
  PyObject *arr, *PE;
  double aratio{0.125}, vmin{0.};
  int nneighs{1};

  if (!PYPARSETUPLE(args, "OOddl", "OOddi", "OOffl", "OOffi", &arr, &PE, &aratio, &vmin, &nneighs)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  // Check numpy (parentElement)
  FldArrayI* cFE;
  E_Int res = K_NUMPY::getFromNumpyArray(PE, cFE, true);

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (ngi.PGs.size() != cFE->getSize())
  {
    std::cout << "le ParentElement ne correpsond pas au nb de pgs" << std::endl;
    delete f; delete cn;
    return nullptr;
  }

  E_Int nphs = ngi.PHs.size();

  std::vector<E_Int> orient;
  std::vector<double> vols(nphs, NUGA::FLOAT_MAX);
  E_Int imin=-1;
  
  //compute volumes using input orientation 
  for (E_Int i=0; i < ngi.PHs.size(); ++i)
  {
    orient.clear();

    const E_Int* pF = ngi.PHs.get_facets_ptr(i);
    E_Int nbf = ngi.PHs.stride(i);
    orient.resize(nbf, 1);

    for (E_Int f = 0; f < nbf; ++f)
    {
      E_Int PGi = *(pF+f) - 1;
      //std::cout << "PGi bef wwong :" << PGi << std::endl;
      if ((*cFE)(PGi, 1) != i+1) orient[f] = -1;
      assert (((*cFE)(PGi, 1) == i+1) || ((*cFE)(PGi, 2) == i+1) );
    }

    //std::cout << "computing flux for PH : " << i << std::endl;
    K_MESH::Polyhedron<0> PH(ngi, i);
    double v;
    DELAUNAY::Triangulator dt;
    E_Int err = PH.volume<DELAUNAY::Triangulator>(crd, &orient[0], v, dt);

    if (!err)
      vols[i] = v;
    else
    {
      //std::cout << "error to triangulate cell " << i << "at face : " << err-1 << std::endl;
      //medith::write("badcell", crd, ngi, i);
      //medith::write("faultyPG", crd, ngi.PGs.get_facets_ptr(err-1), ngi.PGs.stride(err-1), 1);
    }
  }

  ngon_unit neighborsi;
  ngi.build_ph_neighborhood(neighborsi);

  Vector_t<E_Float> growth_ratio;
  ngon_type::stats_bad_volumes<DELAUNAY::Triangulator>(crd, ngi, neighborsi, vols, -1., growth_ratio);

  std::vector<bool> keep(nphs, false);
  E_Int badcount=0;
  for (size_t i=0; i < nphs; ++i)
  {
    if ( (growth_ratio[i] < aratio) || (vols[i] < vmin) ) {
      ++badcount;
      keep[i]=true;
    }
  }

  //std::cout << "nb of bad cells found : " << badcount << " (over " << nphs << ")" << std::endl;

  // extend with second neighborhood and separate from non-involved polyhedra
  for (E_Int j=0; j< nneighs; ++j)
    ngon_type::flag_neighbors(ngi, keep);
    
  ngon_type ngo;  
  {
    Vector_t<E_Int> ngpids;
    ngi.select_phs(ngi, keep, ngpids, ngo);
  }

  ngon_type::compact_to_used_nodes(ngo.PGs, crd); //reduce points

  K_FLD::IntArray cnto;
  ngo.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

///
PyObject* K_INTERSECTOR::volume(PyObject* self, PyObject* args)
{
  PyObject *arr, *axcelln;

  if (!PyArg_ParseTuple(args, "OO", &arr, &axcelln)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  // Check numpy (xcelln)
  bool use_xcelln = false;

  K_FLD::FloatArray* xcelln(nullptr);
  K_FLD::IntArray *cn1(0);
  char *varString1;
  E_Int ni, nj, nk;
  E_Int res = 0;
  if (axcelln != Py_None) res = K_ARRAY::getFromArray(axcelln, varString1, xcelln, ni, nj, nk, cn1, eltType);
  if (res == 1) use_xcelln = true;

  //std::cout << py_xcelln << std::endl;

  // E_Int res = 0;
  // if (py_xcelln != Py_None)
  // {
  //   std::cout << "get numpy " << std::endl;

  //   res = K_NUMPY::getFromNumpyArray(py_xcelln, xcelln, true);
  //   std::cout << xcelln << std::endl;
  //   if (res == 1) use_xcelln = true;
  // }

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  // std::cout << "use_xcelln ? " << use_xcelln << std::endl;
  // std::cout << "xcelln ? " << xcelln << std::endl;
  // std::cout << "res : " << res << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  if (use_xcelln && (xcelln != nullptr) && (ngi.PHs.size() != xcelln->getSize()))
  {
    std::cout << "le champ xcelln ne correpsond pas au nb de polyedres => pas pris en compte" << std::endl;
    std::cout << "nb phs : " << ngi.PHs.size() << std::endl;
    std::cout << "taille xcelln : " << xcelln->getSize() << std::endl;
    use_xcelln = false;
  }

  std::vector<E_Float> vols;
  ngon_type::volumes<DELAUNAY::Triangulator>(crd, ngi, vols, false/*not all cvx*/, true/*new algo*/);

  //std::cout << "use_xcelln ?" << use_xcelln << std::endl;
  E_Float V = 0.;
  if (use_xcelln)
  {
    for (size_t i = 0; i < vols.size(); ++i)
      V += vols[i] * (*xcelln)[i];
  }
  else
    for (size_t i = 0; i < vols.size(); ++i)
      V += vols[i];

  delete f; delete cn;

#ifdef E_DOUBLEREAL
  return Py_BuildValue("d", V);
#else
    return Py_BuildValue("f", float(V));
#endif
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::removeBaffles(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  E_Int nb_cells_w_baffles = ngi.remove_baffles();
  if (nb_cells_w_baffles) std::cout << "number of cells with baffles found : " << nb_cells_w_baffles << std::endl;

  K_FLD::IntArray cnto;
  ngi.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::checkForDegenCells(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  //K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  E_Int degen = 0, imin(0), imax(0), mins(4), maxs(0);
  for (E_Int i=0; i< ngi.PHs.size(); ++i)
  {
    E_Int s = ngi.PHs.stride(i);
    if (s < mins)
    {
      mins=s;
      imin=i;
    }
    if (maxs < s)
    {
      maxs=s;
      imax=i;
    }
    if (s < 4) ++degen;
  }

  if (degen == 0) std::cout << "OK : There are no cell with less than 4 faces." << std::endl;
  else
    {
      std::cout << "ERROR : There are " << degen << " cells with less than 4 faces." << std::endl;
      std::cout << "the min of " << mins << " is reached first at : " << imin << std::endl;
      std::cout << "the max of " << maxs << " is reached first at : " << imax << std::endl;
    }

  delete f; delete cn;

#ifdef E_DOUBLEINT
    return Py_BuildValue("l", long(err));
#else
    return Py_BuildValue("i", err);
#endif
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::checkForBigCells(PyObject* self, PyObject* args)
{
  PyObject *arr;
  E_Int N{8};

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &N)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);
  E_Int maxf{0};
  E_Int idm{IDX_NONE};
  E_Int count{0};
  std::vector<bool> keep(ngi.PHs.size(), false);
  for (E_Int i=0; i< ngi.PHs.size(); ++i)
  {
    E_Int s = ngi.PHs.stride(i);
    if (s > N)
    {
      ++count;
      keep[i]=true;
    }
    if (s > maxf)
    {
      maxf = s;
      idm = i;
    }
    maxf = std::max(maxf, s);
    
  }

  PyObject* tpl{nullptr};

  if (idm != IDX_NONE) std::cout << idm << " is the biggest with " << maxf << " faces" << std::endl;

  if (count > 0)
  {
    std::cout << count << " cells over the specified number of faces have been found." << std::endl;
    
    std::vector<E_Int> pgnids, phnids;
    ngon_type ngo;
    ngi.select_phs(ngi, keep, phnids, ngo);
    ngo.remove_unreferenced_pgs(pgnids, phnids);
    K_FLD::IntArray cnto;
    ngo.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  }
  else
    std::cout << "No cells over the specified number of faces have been found." << std::endl;

  delete f; delete cn;
  return tpl;

}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::edgeLengthExtrema(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);
  E_Float Lmin,Lmax;
  E_Int imin, imax;

  ngon_type::edge_length_extrema(ngi.PGs, crd, Lmin, imin, Lmax, imax);

  //std::cout << "Minimum Edge Length : " << Lmin << " reached at PG : " << imin << std::endl;
  //std::cout << "Maximum Edge Length : " << Lmax << " reached at PG : " << imax << std::endl;

  delete f; delete cn;

#ifdef E_DOUBLEREAL
    return Py_BuildValue("d", double(Lmin));
#else
    return Py_BuildValue("f", float(Lmin));
#endif
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::computeGrowthRatio(PyObject* self, PyObject* args)
{

  PyObject *arr;
  E_Float vmin(0.);

  if (!PYPARSETUPLEF(args, "Od", "Of", &arr, &vmin)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  ngon_unit neighborsi;
  ngi.build_ph_neighborhood(neighborsi);

  std::vector<E_Float> vols;
  ngon_type::volumes<DELAUNAY::Triangulator>(crd, ngi, vols, false/*not all cvx*/, false/* ! new algo*/);


  Vector_t<E_Float> growth_ratio;
  ngon_type::stats_bad_volumes<DELAUNAY::Triangulator>(crd, ngi, neighborsi, vols, vmin, growth_ratio);
  
  size_t sz = growth_ratio.size();
  FloatArray ar(1, sz);
  for (size_t i = 0; i < sz; ++i) ar[i] = growth_ratio[i];

  PyObject* tpl = K_ARRAY::buildArray(ar, "growth_ratio", *cn, -1, "NGON", true);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extrudeBC(PyObject* self, PyObject* args)
{
  PyObject *arr, *pgs;
  E_Float height(0.25);
  E_Int   strategy(0);   // 0 : CST_ABS , 1 : CST_REL_MEAN, 2 : CST_REL_MIN, 3 : VAR_REL_MEAN, 4 : VAR_REL_MIN
  E_Int create_ghost(true);
 
  if (!PYPARSETUPLE(args, "OOdll", "OOdii", "OOfll", "OOfii", &arr, &pgs, &height, &strategy, &create_ghost)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  std::vector<E_Int> PGlist;
  // Passing the specified wall pgs to the boolean to ignore cells that fall inside bodies
  {

    FldArrayI* inds=NULL;
    E_Int res=0;
    if (pgs != Py_None)
      res = K_NUMPY::getFromNumpyArray(pgs, inds, true);

    std::unique_ptr<FldArrayI> pL(inds); // to avoid to call explicit delete at several places in the code.
  
    //std::cout << "result for NUMPY is : " << res << std::endl;
    if ((res == 1) && (inds != NULL)  && (inds->getSize() != 0))
    {
      size_t nb_special_pgs = (size_t)inds->getSize();
      //E_Int minid(INT_MAX), maxid(-1);
      PGlist.resize(nb_special_pgs);
      for (size_t i = 0; i < nb_special_pgs; ++i) 
      {
        PGlist[i]=(*inds)[i]-1;
        //std::cout << pgsList[i] << std::endl;
        //minid = std::min(minid, pgsList[i]);
        //maxid = std::max(maxid, pgsList[i]);
      }

      //std::cout << "min/max : " << minid << "/" << maxid << std::endl;
    }
  }

  ngi.flag_externals(INITIAL_SKIN);
  bool has_been_reversed;
  DELAUNAY::Triangulator dt;
  err = ngon_type::reorient_skins(dt, crd, ngi, has_been_reversed);
  //std::cout << "reversed ? " << has_been_reversed << std::endl;
  if (!err)
  {
    ngon_type::eExtrudeStrategy strat = (ngon_type::eExtrudeStrategy)strategy;
    err = ngon_type::extrude_faces(crd, ngi, PGlist, height, bool(create_ghost), strat);
    //std::cout << "extrude_faces status : " << err << std::endl;
  }

  PyObject* tpl = NULL;

  if (!err)
  {
    K_FLD::IntArray cnto;
    ngi.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  }
  
  delete f; delete cn;

  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extrudeSurf(PyObject* self, PyObject* args)
{
  PyObject *arr;
  E_Float layer_height(0.25);
  E_Int   strategy(0), nlayers(1);   // 0 : CST_ABS , 1 : CST_REL_MEAN, 2 : CST_REL_MIN, 3 : VAR_REL_MEAN, 4 : VAR_REL_MIN
 
  if (!PYPARSETUPLE(args, "Odll", "Odii", "Ofll", "Ofii", &arr, &layer_height, &nlayers, &strategy)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngio(cnt);

  ngio.PHs.clear();

  std::vector<int> pglist;
  K_CONNECT::IdTool::init_inc(pglist, ngio.PGs.size());

  ngon_type::eExtrudeStrategy strat = (ngon_type::eExtrudeStrategy)strategy;

  std::vector<int> tops;
  int smooth_iters = 0;

  for (int l = 0; l < nlayers; ++l)
  {
    tops.clear();
    ngon_type::extrude_faces(crd, ngio, pglist, layer_height, true, strat, smooth_iters, &tops);
    pglist.clear();
    pglist.insert(pglist.end(), tops.begin(), tops.end());
  }

  PyObject* tpl = NULL;

  if (!err)
  {
    K_FLD::IntArray cnto;
    ngio.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  }
  
  delete f; delete cn;

  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extrudeRevolSurf(PyObject* self, PyObject* args)
{
  PyObject *arr;
  E_Float pt[3], dir[3];
  E_Int   nlayers(1);   // 0 : CST_ABS , 1 : CST_REL_MEAN, 2 : CST_REL_MIN, 3 : VAR_REL_MEAN, 4 : VAR_REL_MIN

  //std::cout << "extrudeRevolSurf : 1" << std::endl;
 
  if (!PYPARSETUPLE(args, "O(ddd)(ddd)l", "O(ddd)(ddd)i", "O(fff)(fff)l", "O(fff)(fff)i", &arr, &pt[0], &pt[1], &pt[2], &dir[0], &dir[1], &dir[2], &nlayers)) return NULL;

  //std::cout << "extrudeRevolSurf : 2" << std::endl;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngio(cnt);

  ngio.PHs.clear();

  std::vector<int> pglist;
  K_CONNECT::IdTool::init_inc(pglist, ngio.PGs.size());

  std::vector<int> tops;

  E_Float angle = K_FUNC::normalize<3>(dir);

  //std::cout << "extrudeRevolSurf : 3 : angle : " << angle << std::endl;

  for (int l = 0; l < nlayers; ++l)
  {
    //std::cout << "extrudeRevolSurf : layer : " << l << std::endl;
    tops.clear();
    ngon_type::extrude_revol_faces(crd, ngio, pglist, dir, pt, angle, &tops);

    pglist.clear();
    pglist.insert(pglist.end(), tops.begin(), tops.end());
  }

  //std::cout << "extrudeRevolSurf : 4" << std::endl;

  PyObject* tpl = NULL;

  if (!err)
  {
    K_FLD::IntArray cnto;
    ngio.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  }

  //std::cout << "extrudeRevolSurf : 5" << std::endl;
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* reorient specified polygons. */
//=============================================================================
PyObject* K_INTERSECTOR::reorientSpecifiedFaces(PyObject* self, PyObject* args)
{
  PyObject *arr, *py_pgs;
  E_Int dir(1); //1 : outward -1 : inward

  if (!PYPARSETUPLEI(args, "OOl", "OOi", &arr, &py_pgs, &dir)) return NULL;

  if (dir != -1 && dir != 1) dir = 1;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //std::cout << "before numpy" << std::endl;

  E_Int res=0;
  E_Int* pgsList=NULL;
  E_Int size, nfld;
  if (py_pgs != Py_None)
    res = K_NUMPY::getFromNumpyArray(py_pgs, pgsList, size, nfld, true/*shared*/);

  //std::cout << "after numpy" << std::endl;

  std::cout << res << std::endl;

  if (res != 1) return NULL;
  
  //std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  
  ngon_type ngio(cnt);

  //std::cout << "after ngio construct" << std::endl;

  std::vector<E_Int> plist, oids;
  plist.insert(plist.end(), pgsList, pgsList+size);

  //std::cout << "after insert" << std::endl;
  //K_CONNECT::IdTool::shift(plist, -1);

  //std::cout << "min pg specified : " << *std::min_element(pgsList, pgsList+size) << std::endl;
  //std::cout << "max pg specified : " << *std::max_element(pgsList, pgsList+size) << std::endl;

  ngon_unit pgs;
  ngio.PGs.extract(plist, pgs, oids);

  std::vector<E_Int> orient;
  ngon_type::reorient_connex_PGs(pgs, (dir==-1), orient);

  // replace reverted polygons
  E_Int count(0);
  E_Int nb_pgs = pgs.size();
  for (E_Int i=0; i < nb_pgs; ++i)
  {
    if (orient[i] == 1) continue;

    ++count;
    
    E_Int PGi = oids[i];

    E_Int s = ngio.PGs.stride(PGi);
    E_Int* p = ngio.PGs.get_facets_ptr(PGi);
    std::reverse(p, p + s);
  }

  //std::cout << "nb of reoriented : "  << count  << " over " << size << " in pglist"<< std::endl;
    
  K_FLD::IntArray cnto;
  ngio.export_to_array(cnto);
  
  // pushing out the mesh
  PyObject *tpl = K_ARRAY::buildArray(crd, varString, cnto, -1, eltType, false);   
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::reorient(PyObject* self, PyObject* args)
{
  PyObject *arr;
  E_Int dir(1); //1 : outward -1 : inward

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &dir)) return NULL;

  if (dir != -1 && dir != 1) dir = 1;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  
  ngon_type ngio(cnt);

  ngio.flag_externals(INITIAL_SKIN);
  bool has_been_reversed;
  DELAUNAY::Triangulator t;
  err = ngon_type::reorient_skins(t, crd, ngio, has_been_reversed);

  if (dir == -1)
  {
    Vector_t<E_Int> oids;
    ngon_unit pg_ext;
    ngio.PGs.extract_of_type(INITIAL_SKIN, pg_ext, oids);

    E_Int nb_pgs = pg_ext.size();
    for (E_Int i=0; i < nb_pgs; ++i)
    {
      E_Int PGi = oids[i];

      E_Int s = ngio.PGs.stride(PGi);
      E_Int* p = ngio.PGs.get_facets_ptr(PGi);
      std::reverse(p, p + s);
    }
  }
    
  K_FLD::IntArray cnto;
  ngio.export_to_array(cnto);
  
  // pushing out the mesh
  PyObject *tpl = K_ARRAY::buildArray(crd, varString, cnto, -1, eltType, false);   
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::externalFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::diffMesh(PyObject* self, PyObject* args)
{
  PyObject *arr1, *arr2;

  if (!PyArg_ParseTuple(args, "OO", &arr1, &arr2)) return NULL;

  K_FLD::FloatArray *f1(0), *f2(0);
  K_FLD::IntArray *cn1(0), *cn2(0);
  char *varString1, *varString2, *eltType1, *eltType2;
  // Check array # 1
  E_Int err = check_is_NGON(arr1, f1, cn1, varString1, eltType1);
  if (err) return NULL;
  // Check array # 2
  err = check_is_NGON(arr2, f2, cn2, varString2, eltType2);
  if (err) return NULL;

  std::unique_ptr<K_FLD::FloatArray> pf1(f1), pf2(f2);   //for memory cleaning
  std::unique_ptr<K_FLD::IntArray> pcn1(cn1), pcn2(cn2); //for memory cleaning

  K_FLD::FloatArray & crd = *f1;
  K_FLD::IntArray & cnt = *cn1;
  K_FLD::FloatArray & crd2 = *f2;
  K_FLD::IntArray & cnt2 = *cn2;

  // std::cout << "crd1 : " << crd.cols() << "/" << crd.rows() << std::endl;
  // std::cout << "cnt1 : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  // std::cout << "crd2 : " << crd2.cols() << "/" << crd2.rows() << std::endl;
  // std::cout << "cnt2 : " << cnt2.cols() << "/" << cnt2.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  
  ngon_type ng(cnt), ng1(cnt), ng2(cnt2);

  size_t nb_cells1 = (size_t)ng.PHs.size();
  size_t nb_cells2 = (size_t)ng2.PHs.size();

  // concatenate the meshes
  E_Int shft = crd.cols();
  ng2.PGs.shift(shft);
  ng.append(ng2);
  crd.pushBack(crd2);

  typedef K_FLD::ArrayAccessor<K_FLD::FloatArray> acrd_t;
  acrd_t acrd(crd);

  // detect identical cells
  std::vector<E_Int> nids; // cell ids in the concatenated ng
  E_Int nb_match = ng.detect_phs_with_same_centroid<acrd_t> (acrd, nids);

  //std::cout << "detect_phs_with_same_centroid : " << nb_match << std::endl;
  //
  if (nb_match == 0)
    std::cout << "the meshes are totally unmatching" << std::endl;

  std::vector<bool> keep1(nb_cells1, true);
  std::vector<bool> keep2(nb_cells2, true);
  //std::vector<bool> *pKa, *pKb;

  //
  size_t sz = nids.size();
  for (size_t i=0; i < sz; ++i)
  {
    size_t nid = nids[i];
    if (nid == i) continue;

    // 2 cells are matching
    if (i < nb_cells1)
      keep1[i] = false;
    else
      keep2[i-nb_cells1]=false;

    if (nid < nb_cells1)
      keep1[nid] = false;
    else
      keep2[nid-nb_cells1]=false;
  }

  PyObject *l(PyList_New(0)), *tpl;

  {
    ngon_type ng1o;
    std::vector<E_Int> pgnids, phnids;
    ng1.select_phs(ng1, keep1, phnids, ng1o);
    ng1o.remove_unreferenced_pgs(pgnids, phnids);

    K_FLD::FloatArray crdtmp(crd);
    ngon_type::compact_to_used_nodes(ng1o.PGs, crdtmp); //reduce points
    //export to numpy
    K_FLD::IntArray cnto;
    ng1o.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString1, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }

  {
    ngon_type ng2o;
    std::vector<E_Int> pgnids, phnids;
    ng2.select_phs(ng2, keep2, phnids, ng2o);
    ng2o.remove_unreferenced_pgs(pgnids, phnids);

    K_FLD::FloatArray crdtmp(crd);
    ngon_type::compact_to_used_nodes(ng2o.PGs, crdtmp); //reduce points
    //export to numpy
    K_FLD::IntArray cnto;
    ng2o.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString1, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }

  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::statsUncomputableFaces(PyObject* self, PyObject* args)
{

  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  std::vector<ngon_type::ePathoPG> flags;
  ngon_type::detect_uncomputable_pgs<DELAUNAY::Triangulator>(crd, ngi.PGs, flags);
  
  delete f; delete cn;

#ifdef E_DOUBLEINT
  return Py_BuildValue("l", long(0));
#else
  return Py_BuildValue("i", 0);
#endif
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::statsSize(PyObject* self, PyObject* args)
{

  PyObject *arr;
  E_Int comp_metrics(1);

  if (!PYPARSETUPLEI(args, "Ol", "Oi", &arr, &comp_metrics)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  // Span of the mesh
  // Create the box
  K_SEARCH::BBox3D box;
  K_FLD::ArrayAccessor<K_FLD::FloatArray> acrd(crd);
  box.compute(acrd);
  // Box center and deltas
  E_Float dMax=0.;
  for (int i=0; i < 3; ++i)
   dMax = std::max(dMax, box.maxB[i] - box.minB[i]);
  //std::cout << "the span is : " << dMax << std::endl;

  E_Float smin(0.), smax(0.);
  E_Float vmin, vmax(-1.);

  if (comp_metrics == 1)
  {
    //
    E_Int imin, imax;
	  
	  ngon_type::surface_extrema(ngi.PGs, crd, smin, imin, smax, imax);
	  //std::cout << "the " << imin << "-th face has the smallest surface : " << smin << std::endl;
	  //std::cout << "the " << imax << "-th face has the biggest surface : " << smax << std::endl;
	  //
	  
	  ngon_type::volume_extrema<DELAUNAY::Triangulator>(ngi, crd, vmin, imin, vmax, imax);
	  std::cout << "the " << imin << "-th cells has the smallest volume : " << vmin << std::endl;
    // if not a single cell
	  //if (imax != E_IDX_NONE) std::cout << "the " << imax << "-th cells has the biggest volume : " << vmax << std::endl;
  }

  if (ngi.PGs.size() == 1) smax = -1.;
  if (ngi.PHs.size() == 1) vmax = -1.;

  PyObject *l(PyList_New(0)), *tpl;
  
  delete f; delete cn;

#ifdef E_DOUBLEINT
  tpl = Py_BuildValue("d", long(dMax));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", dMax);
  PyList_Append(l, tpl);
#endif

#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(smin));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", smin);
  PyList_Append(l, tpl);
#endif
#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(smax));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", smax);
  PyList_Append(l, tpl);
#endif
#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(vmin));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", vmin);
  PyList_Append(l, tpl);
#endif
#ifdef E_DOUBLEREAL
  tpl = Py_BuildValue("d", double(vmax));
  PyList_Append(l, tpl);
#else
  tpl =  Py_BuildValue("f", vmax);
  PyList_Append(l, tpl);
#endif

  return l;

}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::convert2Polyhedron(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);
  K_FLD::IntArray cnto;
  ngi.export_surfacic_view(cnto);
  ngon_unit pgs(cnto.begin());
  ngon_type ngo(pgs, true);

  cnto.clear();
  ngo.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::oneZonePerCell(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  PyObject *l(PyList_New(0)), *tpl;

  E_Int nb_phs = ngi.PHs.size();
  std::cout << "nb phs : " << nb_phs << std::endl;
  for (E_Int i=0; i < nb_phs; ++i)
  {
    ngon_type ngo;
    ngo.addPH(ngi, i, true);

    K_FLD::FloatArray crdtmp(crd);
    ngon_type::compact_to_used_nodes(ngo.PGs, crdtmp); //reduce points
    //export to numpy
    K_FLD::IntArray cnto;
    ngo.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }

  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::oneZonePerFace(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  PyObject *l(PyList_New(0)), *tpl;

  E_Int nb_pgs = ngi.PGs.size();
  std::cout << "nb pgs : " << nb_pgs << std::endl;
  for (E_Int i=0; i < nb_pgs; ++i)
  {
    ngon_unit ngu;
    ngu.add(ngi.PGs.stride(i), ngi.PGs.get_facets_ptr(i));
    
    ngon_type ngo(ngu, true);

    K_FLD::FloatArray crdtmp(crd);
    ngon_type::compact_to_used_nodes(ngo.PGs, crdtmp); //reduce points
    //export to numpy
    K_FLD::IntArray cnto;
    ngo.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crdtmp, varString, cnto, 8, "NGON", false);
    PyList_Append(l, tpl);
    Py_DECREF(tpl);
  }

  delete f; delete cn;
  return l;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::immerseNodes(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::closeCells(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  ngon_type::close_phs(ngi, crd);

  K_FLD::IntArray cnto;
  ngi.export_to_array(cnto);
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* Converts a surfacic NGON from Cassiopee format to nuga format*/
//=============================================================================
PyObject* K_INTERSECTOR::convertNGON2DToNGON3D(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt), outer, remaining;

  ngi.export_surfacic_view(cnt);

  ngon_unit pgs(&cnt[0]);

  PyObject* tpl = NULL;

  if (cnt.cols() != 0)
  {
  	ngon_type ngo(pgs, true);
  	K_FLD::IntArray cnto;
    ngo.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  }

  delete f; delete cn;

  return tpl;
}

//=============================================================================
/* Converts a surfacic Basic to NGON nuga format*/
//=============================================================================
PyObject* K_INTERSECTOR::convertBasic2NGONFaces(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_BASICF(arr, f, cn, varString, eltType);
  if (err)
  {
    std::cout << "convertBasic2NGONFaces : ERROR : " << err << std::endl;
    return nullptr;
  }

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  ngon_unit pgs;
  ngon_unit::convert_fixed_stride_to_ngon_unit(cnt, 1, pgs);

  PyObject* tpl = NULL;

  if (pgs.size() != 0)
  {
    ngon_type wNG(pgs, true);
    K_FLD::IntArray cnto;
    wNG.export_to_array(cnto);
    tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  }

  delete f; delete cn;

  return tpl;
}

//=============================================================================
/* remove any cell contributing to a non-manifold boundary */
//=============================================================================
PyObject* K_INTERSECTOR::removeNonManifoldExternalCells(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  //std::cout << "nb initial phs : " << ngi.PHs.size() << std::endl;

  ngi.flag_external_pgs(INITIAL_SKIN);
  Vector_t<E_Int> oids;
  ngon_unit pg_ext;
  ngi.PGs.extract_of_type (INITIAL_SKIN, pg_ext, oids);

  //std::cout << "nb of external pgs : " << pg_ext.size() << std::endl;
    
  std::set<E_Int> pgids;
  ngon_type::get_pgs_with_non_manifold_edges(pg_ext, pgids);

  if (pgids.empty()) std::cout << "removeNonManifoldExternalCells : surface is clean" << std::endl;

  //for (auto i=pgids.begin(); i != pgids.end(); ++i) std::cout << "loc non manif id : " << *i << std::endl;

  std::set<E_Int> npgids;
  for (auto &i : pgids) npgids.insert(oids[i-1]);

  //for (auto i=npgids.begin(); i != npgids.end(); ++i) std::cout << "glob non manif id : " << *i << std::endl;
    
  std::set<E_Int> PHlist;
  ngi.get_PHs_having_PGs(npgids, 0, PHlist);

  //for (auto i=PHlist.begin(); i != PHlist.end(); ++i) std::cout << "PH id : " << *i << std::endl;

  ngi.remove_phs(PHlist);

  //std::cout << "nb final phs : " << ngi.PHs.size() << std::endl;

  K_FLD::IntArray cnto;
  ngi.export_to_array(cnto);

  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* Computes centroids */
//=============================================================================
PyObject* K_INTERSECTOR::centroids(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  using ngon_type = ngon_t<K_FLD::IntArray>;
  ngon_type::eGEODIM geodim = ngon_type::get_ngon_geodim(cnt);

  //std::cout << "GEO dIM ? " << geodim << std::endl;

  if (geodim == ngon_type::eGEODIM::ERROR)
  {
    std::cout << "centroids : Input Error : mesh is corrupted." << std::endl;
    return nullptr;
  }
  if (geodim == ngon_type::eGEODIM::MIXED)
  {
    std::cout << "centroids : Input Error : mesh mixed elt types (lineic and/or surfacic and /or volumic." << std::endl;
    return nullptr;
  }
  if (geodim == ngon_type::eGEODIM::LINEIC)
  {
    std::cout << "centroids : Unsupported : lineic NGON are not handled." << std::endl;
    return nullptr;
  }

  // so either SURFACIC, SURFACIC_CASSIOPEE or VOLUMIC

  if (geodim == ngon_type::eGEODIM::SURFACIC_CASSIOPEE)
  {
    ngon_type ng(cnt);
    // convert to SURFACIC (NUGA)
    K_FLD::IntArray cnt1;
    ng.export_surfacic_view(cnt1);
    //std::cout << "exported" << std::endl;
    geodim = ngon_type::eGEODIM::SURFACIC;
    cnt=cnt1;
  }

  ngon_type ngi(cnt);

  K_FLD::FloatArray cents;

  if (geodim == ngon_type::eGEODIM::SURFACIC)
    ngon_type::centroids(ngi.PGs, crd, cents);
  else // (geodim == ngon_type::eGEODIM::VOLUMIC)
    ngon_type::centroids<DELAUNAY::Triangulator>(ngi, crd, cents);

  K_FLD::IntArray cnto;

  PyObject* tpl = K_ARRAY::buildArray(cents, varString, cnto, 0, "NODE", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* Computes volumes */
//=============================================================================
PyObject* K_INTERSECTOR::volumes(PyObject* self, PyObject* args)
{
  PyObject *arr;
  E_Int algo(0);
  E_Int all_pgs_cvx(0);

  if (!PYPARSETUPLEI(args, "Oll", "Oii", &arr, &algo, &all_pgs_cvx)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;

  //std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;

  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  std::vector<E_Float> vols;
  ngon_type::volumes<DELAUNAY::Triangulator>(crd, ngi, vols, (all_pgs_cvx == 1), (algo == 1));

  size_t sz = vols.size();
  FloatArray ar(1, sz);
  for (size_t i = 0; i < sz; ++i) 
    ar[i] = vols[i];

  PyObject* tpl = K_ARRAY::buildArray(ar, "volumes", *cn, -1, "NGON", true);

  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* retrieves any polygon that are overlapping */
//=============================================================================
PyObject* K_INTERSECTOR::getOverlappingFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
PyObject* K_INTERSECTOR::getCollidingTopFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* retrieves any cells that are colliding */
//=============================================================================
PyObject* K_INTERSECTOR::getCollidingCells(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* Computes cell sensor data from the metric in a mesh */
//=============================================================================
PyObject* K_INTERSECTOR::estimateAdapReq(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::getNthNeighborhood(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* retrieves any polygon that are connecting 2 aniso HEXA */
//=============================================================================
PyObject* K_INTERSECTOR::getAnisoInnerFaces(PyObject* self, PyObject* args)
{
  PyObject *arr;
  E_Float aniso_ratio(0.05);
  
  if (!PYPARSETUPLEF(args, "Od", "Of", &arr, &aniso_ratio)) return NULL;

  K_FLD::FloatArray *f(0);
  K_FLD::IntArray *cn(0);
  char *varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;

  std::unique_ptr<K_FLD::FloatArray> pf(f);   //for memory cleaning
  std::unique_ptr<K_FLD::IntArray> pcn(cn); //for memory cleaning

  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  
  ngon_type ng(cnt);

  E_Int nb_pgs = ng.PGs.size();
  std::vector<E_Int> flag(nb_pgs, 0);
  E_Int nb_phs = ng.PHs.size();
  E_Int bot, top;
  for (E_Int i=0; i < nb_phs; ++i)
  {
    const E_Int* faces = ng.PHs.get_facets_ptr(i);
    if (K_MESH::Polyhedron<0>::is_aniso_HX8(crd, ng.PGs, faces, ng.PHs.stride(i), aniso_ratio, bot, top))
    {
      ++flag[*(faces+bot)-1];
      ++flag[*(faces+top)-1];
    }
  }
  std::vector<E_Int> pgids;
  for (E_Int i=0; i < nb_pgs; ++i)
  {
    if (flag[i] == 2) //inner
      pgids.push_back(i);
  }

  std::cout << "getAnisoInnerFaces : " << pgids.size() << std::endl;

  PyObject* tpl = K_NUMPY::buildNumpyArray(&pgids[0], pgids.size(), 1, 0);

  return tpl;

}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::merge(PyObject* self, PyObject* args)
{

  PyObject *arr1, *arr2;
  E_Float tolerance(1.e-15);
  if (!PYPARSETUPLEF(args, "OOd", "OOf", &arr1, &arr2, &tolerance)) return NULL;

  char *varString, *eltType;
  E_Int ni, nj, nk;
  K_FLD::FloatArray *f1(0), *f2(0);
  K_FLD::IntArray *cn1(0);
  
  /*E_Int res = */K_ARRAY::getFromArray(arr1, varString, f1, ni, nj, nk, cn1, eltType);

  // if (strcmp(eltType, "NODE") != 0)
  // {
  //   PyErr_SetString(PyExc_TypeError, "input error : invalid array, must be a NODE array.");
  //   delete f1; delete cn1;
  //   return nullptr;
  // }

  /*res = */K_ARRAY::getFromArray(arr2, varString, f2, ni, nj, nk, cn1, eltType);

  // if (strcmp(eltType, "NODE") != 0)
  // {
  //   PyErr_SetString(PyExc_TypeError, "input error : invalid array, must be a NODE array.");
  //   delete f1, delete f2; delete cn1;
  //   return nullptr;
  // }
 
  K_FLD::FloatArray crd = *f1;
  crd.pushBack(*f2);

  K_FLD::ArrayAccessor<K_FLD::FloatArray> ca(crd);
  Vector_t<E_Int> nids;
  /*E_Int nb_merges = */::merge(ca, tolerance, nids);

  E_Int sz = f2->cols();
  Vector_t<E_Int> nids_for_2(sz);
  for (E_Int i=0; i < sz; ++i)
  {
    nids_for_2[i] = nids[i+sz];
  }

  PyObject*tpl = K_NUMPY::buildNumpyArray(&nids_for_2[0], sz, 1, 0);
  delete f1, delete f2; delete cn1;

  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::oneph(PyObject* self, PyObject* args)
{
  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  char *varString, *eltType;
  E_Int ni, nj, nk;
  K_FLD::FloatArray *f(0);
  K_FLD::IntArray *cn(0);
  
  /*E_Int res = */K_ARRAY::getFromArray(arr, varString, f, ni, nj, nk, cn, eltType);
  if ( (strcmp(eltType, "TRI") != 0) && (strcmp(eltType, "QUAD") != 0) )
  {
    PyErr_SetString(PyExc_TypeError, "input error : invalid array, must be a TRI or QUAD array.");
    delete f; delete cn;
    return nullptr;
  }

  //K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  K_FLD::FloatArray& crd = *f;

  ngon_unit pgs;
  ngon_unit::convert_fixed_stride_to_ngon_unit(cnt, 1, pgs);
 
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  
  ngon_type ng(pgs, 1);
  //ngon_type::clean_connectivity(ng, crd);

  K_FLD::IntArray cnto;
  ng.export_to_array(cnto);
  
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::concatenate(PyObject* self, PyObject* args)
{

  E_Float tol(1.e-15);
  PyObject* arrs;

  if (!PYPARSETUPLEF(args, "Od", "Of", &arrs, &tol))
  {
    PyErr_SetString(PyExc_TypeError, "concatenate : wrong args");
    return NULL;
  }

  E_Int nb_zones = PyList_Size(arrs);
  

  std::vector<K_FLD::FloatArray*> crds(nb_zones, nullptr);
  std::vector<K_FLD::IntArray*>   cnts(nb_zones, nullptr);
  char* varString, *eltType;
  
  // get the zones
  for (E_Int i=0; i < nb_zones; ++i)
  {
    //std::cout << "getting zone in list : " << i << std::endl;
    PyObject* py_zone = PyList_GetItem(arrs, i);
    
    E_Int err = check_is_NGON(py_zone, crds[i], cnts[i], varString, eltType);
    if (err)
    {
      for (E_Int i=0; i < nb_zones; ++i)
      {
        delete crds[i];
        delete cnts[i];
      }
      return NULL;
    }

    //std::cout << "zone sizes : " << crd1s[i]->cols() << " points" << std::endl;
    //std::cout << "zone sizes : " << cnt1s[i]->cols() << " cells" << std::endl;
  }

  // join and close

  ngon_type ng;
  K_FLD::FloatArray crd;
  for (size_t i=0; i < cnts.size(); ++i)
  {
    //std::cout << "appending" << std::endl;
    ngon_type ngt(*cnts[i]);
    ngt.PGs.shift(crd.cols());
    ng.append(ngt);
    crd.pushBack(*crds[i]);
  }

  //std::cout << "before clean : nb_phs/phs/crd : " << ng.PHs.size() << "/" << ng.PGs.size() << "/" << crd.cols() << std::endl;

  ngon_type::clean_connectivity(ng, crd, -1, tol);

  if (ng.PHs.size() == cnts.size()) // NUGA SURF
    ng = ngon_type(ng.PGs, true);

  //std::cout << "after clean : nb_phs/phs/crd : " << ng.PHs.size() << "/" << ng.PGs.size() << "/" << crd.cols() << std::endl;
 

  K_FLD::IntArray cnto;
  ng.export_to_array(cnto);
  
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);
  
  for (E_Int i=0; i < nb_zones; ++i)
  {
    delete crds[i];
    delete cnts[i];
  }
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::drawOrientation(PyObject* self, PyObject* args)
{

  PyObject *arr;

  if (!PyArg_ParseTuple(args, "O", &arr)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr, f, cn, varString, eltType);
  if (err) return NULL;
    
  K_FLD::FloatArray & crd = *f;
  K_FLD::IntArray & cnt = *cn;
  
  //~ std::cout << "crd : " << crd.cols() << "/" << crd.rows() << std::endl;
  //~ std::cout << "cnt : " << cnt.cols() << "/" << cnt.rows() << std::endl;
  
  typedef ngon_t<K_FLD::IntArray> ngon_type;
  ngon_type ngi(cnt);

  K_FLD::IntArray cntE;
  K_FLD::FloatArray crdE;
  for (E_Int i=0; i < ngi.PGs.size(); ++i)
  {
    
    const E_Int* nodes = ngi.PGs.get_facets_ptr(i);
    E_Int nb_nodes = ngi.PGs.stride(i);

    E_Float c[3], n[3], top[3];
    K_MESH::Polygon::centroid<3>(crd, nodes, nb_nodes, 1, c);
    K_MESH::Polygon::normal<K_FLD::FloatArray, 3>(crd, nodes, nb_nodes, 1, n);

    K_MESH::Polygon pg(nodes, nb_nodes, -1);
    double Lref = ::sqrt(pg.Lref2(crd));

    K_FUNC::sum<3>(1., c, Lref, n, top);

    E_Int id = crdE.cols();
    crdE.pushBack(c, c+3);
    crdE.pushBack(top, top+3);

    E_Int e[] = {id, id+1};
    cntE.pushBack(e, e+2);

  }
  
  PyObject* tpl = K_ARRAY::buildArray(crdE, varString, cntE, -1, "BAR", false);
  
  delete f; delete cn;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::getFaceIdsWithCentroids(PyObject* self, PyObject* args)
{
  PyObject *arr1, *arr2;

  if (!PyArg_ParseTuple(args, "OO", &arr1, &arr2)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr1, f, cn, varString, eltType);
  if (err) return NULL;

  E_Int ni, nj, nk;
  K_FLD::FloatArray* f2(nullptr);
  K_FLD::IntArray* cn2(nullptr);
  char* varString2, *eltType2;
  E_Int res = K_ARRAY::getFromArray(arr2, varString2, f2, ni, nj, nk, cn2, eltType2);

  K_FLD::FloatArray& crd = *f;
  K_FLD::IntArray& cnt = *cn;

  K_FLD::FloatArray& crd2 = *f2;

  using ngon_type = ngon_t<K_FLD::IntArray>;
  ngon_type::eGEODIM geodim = ngon_type::get_ngon_geodim(cnt);

  //std::cout << "GEO dIM ? " << geodim << std::endl;

  if (geodim == ngon_type::eGEODIM::ERROR)
  {
    std::cout << "externalFaces : Input Error : mesh is corrupted." << std::endl;
    return nullptr;
  }
  if (geodim == ngon_type::eGEODIM::MIXED)
  {
    std::cout << "externalFaces : Input Error : mesh mixed elt types (lineic and/or surfacic and /or volumic." << std::endl;
    return nullptr;
  }
  if (geodim == ngon_type::eGEODIM::LINEIC)
  {
    std::cout << "externalFaces : Unsupported : lineic NGON are not handled." << std::endl;
    return nullptr;
  }

  // so either SURFACIC, SURFACIC_CASSIOPEE or VOLUMIC

  if (geodim == ngon_type::eGEODIM::SURFACIC_CASSIOPEE)
  {
    ngon_type ng(cnt);
    // convert to SURFACIC (NUGA)
    K_FLD::IntArray cnt1;
    ng.export_surfacic_view(cnt1);
    //std::cout << "exported" << std::endl;
    geodim = ngon_type::eGEODIM::SURFACIC;
    cnt=cnt1;
  }

  PyObject *tpl = nullptr;
  
  // SURFACIC OR VOLUMIC ?

  ngon_type ngi(cnt);
  ngon_unit& PGS = ngi.PGs;

  K_FLD::FloatArray cents;
  ngon_type::centroids(PGS, crd, cents);

  E_Int nb_pts2 = crd2.cols();
  //std::cout << "nb pts : " << nb_pts2 << std::endl;

  using acrd_t = K_FLD::ArrayAccessor<K_FLD::FloatArray>;
  acrd_t acrd(cents);
  K_SEARCH::KdTree<> tree(acrd);

  std::vector<E_Int> ids;

  for (E_Int i=0; i < nb_pts2; ++i)
  {
    double d2;
    int N = tree.getClosest(crd2.col(i));
    if (N != IDX_NONE)
      ids.push_back(N+1);
  }

  tpl = K_NUMPY::buildNumpyArray(&ids[0], ids.size(), 1, 0);

  delete f; delete cn;
  delete f2; delete cn2;
  return tpl;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::getFaceIdsCollidingVertex(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::getCells(PyObject* self, PyObject* args)
{
  PyObject *arr1, *arr2;
  E_Int is_face_id{1};

  if (!PYPARSETUPLEI(args, "OOl", "OOi", &arr1, &arr2, &is_face_id)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr1, f, cn, varString, eltType);
  if (err) return NULL;

  E_Int res=0;
  E_Int* ids=NULL;
  E_Int size, nfld;
  if (arr2 != Py_None)
    res = K_NUMPY::getFromNumpyArray(arr2, ids, size, nfld, true/*shared*/);

  K_FLD::FloatArray& crd = *f;
  K_FLD::IntArray& cnt = *cn;

  using ngon_type = ngon_t<K_FLD::IntArray>;
  ngon_type ng(cnt), ngo;

  std::vector<bool> keep;

  if (is_face_id)
  {
    std::set<E_Int> sids(ids, ids+size);    
    std::vector<E_Int> elts;
    ng.PHs.find_elts_with_facets(sids, elts);

    keep.resize(ng.PHs.size(), false);
    for (size_t i=0; i < elts.size(); ++i) keep[elts[i]]=true;
  }
  else // cell id
  {
    keep.resize(ng.PHs.size(), false);
    for (size_t i=0; i < size; ++i) keep[ids[i]]=true;
  }

  Vector_t<E_Int> nids;
  ngon_type::select_phs(ng, keep, nids, ngo);

  K_FLD::IntArray cnto;
  ngo.export_to_array(cnto);
  
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);

  delete f; delete cn;
  return tpl;

}

PyObject* K_INTERSECTOR::getFaces(PyObject* self, PyObject* args)
{
  PyObject *arr1, *arr2;

  if (!PyArg_ParseTuple(args, "OO", &arr1, &arr2)) return NULL;

  K_FLD::FloatArray* f(0);
  K_FLD::IntArray* cn(0);
  char* varString, *eltType;
  // Check array # 1
  E_Int err = check_is_NGON(arr1, f, cn, varString, eltType);
  if (err) return NULL;

  E_Int res=0;
  E_Int* pgids=NULL;
  E_Int size, nfld;
  if (arr2 != Py_None)
    res = K_NUMPY::getFromNumpyArray(arr2, pgids, size, nfld, true/*shared*/);

  K_FLD::FloatArray& crd = *f;
  K_FLD::IntArray& cnt = *cn;

  ngon_type ng(cnt);

  ngon_unit nguo;

  std::vector< E_Int> oids;
  ng.PGs.extract(pgids, size, nguo, oids);

  ngon_type ngo(nguo, true);

  K_FLD::IntArray cnto;
  ngo.export_to_array(cnto);
  
  PyObject* tpl = K_ARRAY::buildArray(crd, varString, cnto, 8, "NGON", false);

  delete f; delete cn;
  return tpl;

}

//=======================  Intersector/PolyMeshTools/utils.cpp ====================
