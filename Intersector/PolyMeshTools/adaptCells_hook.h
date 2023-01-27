/*    
    Copyright 2013-2023 Onera.

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

#ifndef ADAPTCELLS_HOOK
#define ADAPTCELLS_HOOK


#define HMESH_HOOK_ID 77
#define SENSOR_HOOK_ID 78

#define HMESH_PACK_SIZE 6

//// DICO / MAP utils //////////////////////////
 
inline void convert_dico_to_map___int_int_vecint
(
  PyObject *py_zone_to_rid_to_list,
  std::map<int, std::map<int, std::vector<E_Int>>>& zone_to_rid_to_list)
{
  if (PyDict_Check(py_zone_to_rid_to_list))
  {
    //E_Int nzid = PyDict_Size(py_zone_to_rid_to_list);

    PyObject *py_zid/*key*/, *py_rid_to_list /*value : map rid to ptlist*/;
    Py_ssize_t pos = 0;

    while (PyDict_Next(py_zone_to_rid_to_list, &pos, &py_zid, &py_rid_to_list))
    {
      int zid = (int) PyInt_AsLong(py_zid);

      assert (PyDict_Check(py_rid_to_list) == 1); // it s a map

      PyObject *py_rid/*key*/, *py_ptlist /*value : ptlist*/;
      Py_ssize_t pos1 = 0;

      while (PyDict_Next(py_rid_to_list, &pos1, &py_rid, &py_ptlist))
      {
        int rid = (int) PyInt_AsLong(py_rid);

        assert (PyArray_Check(py_ptlist) == 1) ; // it s a numpy
        
        PyArrayObject* pyarr = reinterpret_cast<PyArrayObject*>(py_ptlist);

        long ndims = PyArray_NDIM(pyarr);
        assert (ndims == 1); // vector
        npy_intp* dims = PyArray_SHAPE(pyarr);

        E_Int ptl_sz = dims[0];
        
        //long* dataPtr = static_cast<long*>(PyArray_DATA(pyarr));
        E_Int* dataPtr = (E_Int*)PyArray_DATA(pyarr);

        std::vector<E_Int> ptl(ptl_sz);
        for (size_t u=0; u < ptl_sz; ++u) ptl[u] = dataPtr[u];

        //std::cout << "max in C is : " << *std::max_element(ALL(ptl)) << std::endl;

        zone_to_rid_to_list[zid][rid]=ptl;

      }
    }
  }
}

inline void convert_dico_to_map__int_pairint
(
  PyObject *py_rid_to_zones,
  std::map<int, std::pair<int,int>>& rid_to_zones)
{
  if (PyDict_Check(py_rid_to_zones))
  {
    // E_Int nzid = PyDict_Size(py_rid_to_zones);

    PyObject *py_rid/*key*/, *py_pair /*value : map zid to ptlist*/;
    Py_ssize_t pos = 0;

    while (PyDict_Next(py_rid_to_zones, &pos, &py_rid, &py_pair))
    {
      int rid = (int) PyInt_AsLong(py_rid);

      assert (PyTuple_Check(py_pair) == 1); // is it a tuple ?

      PyTupleObject* pytup = reinterpret_cast<PyTupleObject*>(py_pair);    
      Py_ssize_t nb = PyTuple_GET_SIZE(pytup);

      // -----

      std::pair<int,int> pair_zid;

      PyObject * z1 PyTuple_GET_ITEM(pytup, 0);
      PyObject * z2 PyTuple_GET_ITEM(pytup, 1);

      pair_zid.first  = (double) PyFloat_AsDouble(z1);
      pair_zid.second = (double) PyFloat_AsDouble(z2);

      rid_to_zones[rid] = pair_zid;
    }
  }
}

struct transf_t {
  double t[6];
  bool operator==(const transf_t& r) const
  {
    for (size_t k=0; k < 6; ++k)
      if (t[k] != r.t[k]) return false;
    return true;
  }
  bool operator<(const transf_t& r) const
  {
    if (*this == r) return false;
    for (size_t k=0; k < 6; ++k)
      if (t[k] <r.t[k]) return true;
    return false;
  }
};

inline int convert_dico_to_map___transfo_to_vecint
(
  PyObject *py_transfo_to_list,
  std::map<transf_t, std::vector<int>>& transfo_to_list
)
{
  if (PyDict_Check(py_transfo_to_list) == 0) return 1;
  
  //E_Int nzid = PyDict_Size(transfo_to_list);

  PyObject *py_transfo/*key*/, *py_vecint /*value : vector<int>*/;
  Py_ssize_t pos = 0;

  transf_t t;

  while (PyDict_Next(py_transfo_to_list, &pos, &py_transfo, &py_vecint))
  {
    // key
    assert (PyTuple_Check(py_transfo) == 1) ; // it s a tuple (Xx, Yc, Zc, R)
    PyTupleObject* pytup = reinterpret_cast<PyTupleObject*>(py_transfo);
    Py_ssize_t nb = PyTuple_GET_SIZE(pytup);

    assert (nb == 6);
    for (size_t i=0; i < 6; ++i)
    {
      PyObject * p PyTuple_GET_ITEM(pytup, i);
      t.t[i] = (double) PyFloat_AsDouble(p);
      //std::cout << "transfo " << i << " : " << t.t[i] << std::endl;
    }

    // val
    assert (PyArray_Check(py_vecint) == 1) ; // it s a numpy
    PyArrayObject* pyarr = reinterpret_cast<PyArrayObject*>(py_vecint);

    long ndims = PyArray_NDIM(pyarr);
    assert (ndims == 1); // vector
    npy_intp* dims = PyArray_SHAPE(pyarr);

    E_Int sz = dims[0];
    
    //long* dataPtr = static_cast<long*>(PyArray_DATA(pyarr));
    E_Int* dataPtr = (E_Int*)PyArray_DATA(pyarr);

    transfo_to_list[t].resize(sz);
    for (size_t u=0; u < sz; ++u) transfo_to_list[t][u] = dataPtr[u];
  }
}

////////////////////////////////////////////////


//=============================================================================
/* get hmesh hook  */
//=============================================================================
inline void* unpackHMesh(PyObject* hook_hmesh, int *&hook_hm_id, int *&subdiv_type, int *&elt_type, int *&zid, std::string *&vString, void **&packet)
{
  //std::cout << "unpackHMesh : begin : " << hook_hmesh << std::endl;

#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 7) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 1)
  packet = (void**)PyCObject_AsVoidPtr(hook_hmesh);
#else
  packet = (void**)PyCapsule_GetPointer(hook_hmesh, NULL);
#endif

  //std::cout << "unpackHMesh : after capsule : " << packet << std::endl;

  if (packet == nullptr)
  {
    PyErr_SetString(PyExc_TypeError,
      "unpackHMesh: PyCapsule_GetPointer failure.");
    return nullptr;
  }

  hook_hm_id = (int*)packet[0];        // type of hook

  //std::cout << "unpackHMesh : after type" << std::endl;

  if (*hook_hm_id != HMESH_HOOK_ID)
  {
    PyErr_SetString(PyExc_TypeError,
      "unpackHMesh: hook id failure.");
    return nullptr;
  }

  //std::cout << "unpackHMesh : before setting vals" << std::endl;
  
  void* hmesh          = packet[1];                // untyped hmesh ptr
  subdiv_type          = (int*)packet[2];        // subdivision type ISO, ISO_HEX, DIR...  
  elt_type             = (int*)packet[3];        // type of elements in hmesh
  vString              = (std::string*)packet[4];  // for buildArray
  zid                  = (int*) packet[5];

  //std::cout << "unpackHMesh : end" << std::endl;

  return hmesh;
}

//=============================================================================
/* get sensor hook  */
//=============================================================================
inline void* unpackSensor(PyObject* hook_sensor, int *&hook_ss_id, int *&sensor_type, int *&smoothing_type, int *&subdiv_type, int *&elt_type, void **&packet_ss)
{

#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 7) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 1)
  packet_ss = (void**)PyCObject_AsVoidPtr(hook_sensor);
#else
  packet_ss = (void**)PyCapsule_GetPointer(hook_sensor, NULL);
#endif

  hook_ss_id = (int*)packet_ss[0];

  if (*hook_ss_id != SENSOR_HOOK_ID)
  {
    PyErr_SetString(PyExc_TypeError,
      "unpackSensor: this function requires a identify sensor hook.");
    return nullptr;
  }

  sensor_type    = (int*)packet_ss[1];
  smoothing_type = (int*)packet_ss[3];
  elt_type       = (int*)packet_ss[4];
  subdiv_type    = (int*)packet_ss[5];
  void* sensor   = packet_ss[2];

  return sensor;
}


#endif
