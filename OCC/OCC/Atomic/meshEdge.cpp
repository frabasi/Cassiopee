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

#include "occ.h"

#include "TopoDS.hxx"
#include "BRep_Tool.hxx"
#include "BRepAdaptor_Curve.hxx"
#include "GCPnts_AbscissaPoint.hxx" 
#include "GCPnts_UniformDeflection.hxx"
#include "GCPnts_UniformAbscissa.hxx"
#include "TopTools_IndexedMapOfShape.hxx"
#include "TopExp_Explorer.hxx"

// Open cascade snippets pour recuperer une pCurve
//Standard_Real aFirst, aLast, aPFirst, aPLast;
//Handle(Geom_Curve) aCurve3d = BRep_Tool::Curve(anEdge, aFirst, aLast);
//Handle(Geom2d_Curve) aPCurve = BRep_Tool::CurveOnSurface(anEdge, aFace, aPFirst, aPLast);

// ============================================================================
// Mesh an edge uniformly with nbPoints
// If edge is degenerated, return two identical points
// Return 1 if fails, 0 otherwise
// ============================================================================
E_Int __meshEdge(const TopoDS_Edge& E, 
E_Int& nbPoints, K_FLD::FldArrayF& coords)
{
  BRepAdaptor_Curve C0(E);
  GeomAdaptor_Curve geomAdap(C0.Curve()); // Geometric Interface <=> access to discretizations tool
  Standard_Real u0 = geomAdap.FirstParameter();
  Standard_Real u1 = geomAdap.LastParameter();
  E_Float* px = coords.begin(1);
  E_Float* py = coords.begin(2);
  E_Float* pz = coords.begin(3);

  // degenerated
  if (BRep_Tool::Degenerated(E))
  {
    gp_Pnt Pt;
    C0.D0(0., Pt);
    for (E_Int i = 0; i < nbPoints; i++)
    {
      px[i] = Pt.X(); py[i] = Pt.Y(); pz[i] = Pt.Z();
    }
    return 1;
  }

  // non degenrated
  GCPnts_UniformAbscissa unifAbs(geomAdap, nbPoints, u0, u1);
  if (!unifAbs.IsDone()) return 1;
  if (nbPoints != unifAbs.NbPoints()) return 1;
    
#pragma omp parallel
  {
    gp_Pnt Pt;
#pragma omp for
    for (Standard_Integer i = 1; i <= nbPoints; i++)
    {
      C0.D0(unifAbs.Parameter(i), Pt);
      px[i-1] = Pt.X(); py[i-1] = Pt.Y(); pz[i-1] = Pt.Z(); 
    }
  }
  return 0;
}

// Return the nbPts for meshing E regular with hmax
// If E is degenerated, return 2 points.
E_Int __getNbPts(const TopoDS_Edge& E, E_Float hmax, E_Int& nbPoints)
{
  if (BRep_Tool::Degenerated(E)) { nbPoints=2; return 1; }
  BRepAdaptor_Curve C0(E);
  GeomAdaptor_Curve geomAdap(C0.Curve()); // Geometric Interface <=> access to discretizations tool
  E_Float u0 = geomAdap.FirstParameter();
  E_Float u1 = geomAdap.LastParameter();
  
  E_Float L = (E_Float) GCPnts_AbscissaPoint::Length(geomAdap, u0, u1);
  nbPoints = (E_Int)round(L / hmax);
  nbPoints = std::max(nbPoints, 2);
  return 0;
}

// ============================================================================
/* Mesh global edges of CAD, regular hmax, return STRUCT */
// ============================================================================
PyObject* K_OCC::meshGlobalEdges(PyObject* self, PyObject* args)
{
  PyObject* hook; E_Float hmax;
  if (!PYPARSETUPLEF(args, "Od", "Of", &hook, &hmax)) return NULL;

  void** packet = NULL;
#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 7) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 1)
  packet = (void**) PyCObject_AsVoidPtr(hook);
#else
  packet = (void**) PyCapsule_GetPointer(hook, NULL);
#endif

  E_Int nbPoints;
  PyObject* out = PyList_New(0);
  TopTools_IndexedMapOfShape& edges = *(TopTools_IndexedMapOfShape*)packet[2];
  for (E_Int i=1; i <= edges.Extent(); i++)
  {
    const TopoDS_Edge& E = TopoDS::Edge(edges(i));
    __getNbPts(E, hmax, nbPoints);

    // create array
    PyObject* o = K_ARRAY::buildArray2(3, "x,y,z", nbPoints, 1, 1, 1);
    FldArrayF* f; K_ARRAY::getFromArray2(o, f);

    // fill array
    __meshEdge(E, nbPoints, *f);
    RELEASESHAREDS(o, f);
    PyList_Append(out, o); Py_DECREF(o);
  }
  return out;
}

// ============================================================================
/* Mesh global edges of CAD, fixed number of points, return STRUCT */
// ============================================================================
PyObject* K_OCC::meshGlobalEdges2(PyObject* self, PyObject* args)
{
  PyObject* hook; E_Int nbPoints;
  if (!PYPARSETUPLEI(args, "Ol", "Oi", &hook, &nbPoints)) return NULL;

  void** packet = NULL;
#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 7) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 1)
  packet = (void**) PyCObject_AsVoidPtr(hook);
#else
  packet = (void**) PyCapsule_GetPointer(hook, NULL);
#endif

  PyObject* out = PyList_New(0);
  TopTools_IndexedMapOfShape& edges = *(TopTools_IndexedMapOfShape*)packet[2];
  for (E_Int i=1; i <= edges.Extent(); i++)
  {
    const TopoDS_Edge& E = TopoDS::Edge(edges(i));
    
    // create array
    PyObject* o = K_ARRAY::buildArray2(3, "x,y,z", nbPoints, 1, 1, 1);
    FldArrayF* f; K_ARRAY::getFromArray2(o, f);

    // fill array
    __meshEdge(E, nbPoints, *f);
    RELEASESHAREDS(o, f);
    PyList_Append(out, o); Py_DECREF(o);
  }
  return out;
}

