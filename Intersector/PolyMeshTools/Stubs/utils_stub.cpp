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

# include "intersector.h"
# include "stub.h"


//=============================================================================
/* Creates 4 zones : 1) uncomputable polygons 2) uncomputable polyhedra 
   3) uncomputable polyhedra & neighbors 4) complementary of 3) */
//=============================================================================
PyObject* K_INTERSECTOR::extractUncomputables(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX*/
//=============================================================================
PyObject* K_INTERSECTOR::extractPathologicalCells(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}


//=============================================================================
/* Creates 2 zones : 1) outerlayer with firt neighborhoo 2) complementary */
//=============================================================================
PyObject* K_INTERSECTOR::extractOuterLayers(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extractNthCell(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extractBiggestCell(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::removeNthCell(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::detectIdenticalCells(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::detectOverConnectedFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extractNthFace(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::checkCellsClosure(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

PyObject* K_INTERSECTOR::volume(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::removeBaffles(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::checkForDegenCells(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::edgeLengthExtrema(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::computeAspectRatio(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extrudeBC(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extrudeSurf(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::extrudeRevolSurf(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::reorientExternalFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* reorient specified polygons. */
//=============================================================================
PyObject* K_INTERSECTOR::reorientSpecifiedFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::reorientSurf(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::diffMesh(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::statsUncomputableFaces(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::statsSize(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::convert2Polyhedron(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::oneZonePerCell(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::closeCells(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* Converts a surfacic NGON from Cassiopee format to nuga format*/
//=============================================================================
PyObject* K_INTERSECTOR::convertNGON2DToNGON3D(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* Converts a surfacic Basic to NGON nuga format*/
//=============================================================================
PyObject* K_INTERSECTOR::convertBasic2NGONFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* remove any cell contributing to a non-manifold boundary */
//=============================================================================
PyObject* K_INTERSECTOR::removeNonManifoldExternalCells(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* Computes centroids*/
//=============================================================================
PyObject* K_INTERSECTOR::centroids(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* Computes volumes */
//=============================================================================
PyObject* K_INTERSECTOR::volumes(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
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
/* retrieves any polygon that are connecting 2 aniso HEXA */
//=============================================================================
PyObject* K_INTERSECTOR::getAnisoInnerFaces(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;

}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::merge(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::oneph(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::concatenate(PyObject* self, PyObject* args)
{

  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=============================================================================
/* XXX */
//=============================================================================
PyObject* K_INTERSECTOR::drawOrientation(PyObject* self, PyObject* args)
{
  PyErr_SetString(PyExc_NotImplementedError, STUBMSG);
  return NULL;
}

//=======================  Intersector/PolyMeshTools/utils.cpp ====================
