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

// Display List OpenGL 2.0 treatement for one shot display
#ifndef _CPLOT_DATADL_H_
# define _CPLOT_DATADL_H_
# include "Data.h"
# include "CPlotStateDL.h"
# include "ZoneImplDL.h"

class DataDL : public Data
{
public:
  static Data* getInstance();

  DataDL();
  virtual ~DataDL();

  virtual void createGPURes();
  virtual void freeGPURes(int mode, int start, int end, int permanent);
  virtual void freeGPURes(int mode, int size, int* ptr, int permanent);

  virtual void createIsoGPURes(int nofield); // scalaire
  virtual void createIsoGPURes(int nofield1, int nofield2, int nofield3); // vecteur
  virtual void createIsoGPUResForRender(); // scalaire for render mode
  virtual void createGPUSMeshZone(StructZone* zonep, int zone);
  virtual void createGPUUMeshZone(UnstructZone* zonep, int zone, int zonet);
  virtual void createGPUUMeshZone_ho(UnstructZone* zonep, int zone, int zonet);
  virtual void createGPUSSolidZone(StructZone* zonep, int zone);
  virtual void createGPUUSolidZone(UnstructZone* zonep, int zone, int zonet);
  virtual void createGPUUSolidHOZone(UnstructZone* zonep, int zone, int zonet);
  virtual void createGPUSIsoSolidZone(StructZone* zonep, int zone, int nofield);
  virtual void createGPUSIsoSolidZone(StructZone* zonep, int zone, int nofield1,
			      int nofield2, int nofield3);
  virtual void createGPUUIsoSolidZone(UnstructZone* zonep, int zone, int zonet, 
			      int nofield);
  virtual void createGPUUIsoSolidZone(UnstructZone* zonep, int zone, int zonet, 
			      int nofield1, int nofield2, int nofield3);

  virtual void renderGPUSMeshZone(StructZone* zonep, int zone);
  virtual void renderGPUUMeshZone(UnstructZone* zonep, int zone, int zonet);
  virtual void renderGPUSSolidZone(StructZone* zonep, int zone);
  virtual void renderGPUUSolidZone(UnstructZone* zonep, int zone, int zonet);
  virtual void renderGPUUSolidHOZone(UnstructZone* zonep, int zone, int zonet);
  virtual void renderSIsoSolidZone(StructZone* zonep, int zone, int nofield);
  virtual void renderSIsoSolidZone(StructZone* zonep, int zone, int nofield1,
				   int nofield2, int nofield3);
  virtual void renderUIsoSolidZone(UnstructZone* zonep, int zonet, int nofield);
  virtual void renderUIsoSolidZone(UnstructZone* zonep, int zonet, int nofield1,
				   int nofield2, int nofield3);

  virtual void displaySMesh();
  virtual void displayUMesh();
  virtual void displaySSolid();
  virtual void displayUSolid();
  virtual void displaySIsoSolid();
  virtual void displayUIsoSolid();

  virtual int initZoneData( std::vector<K_FLD::FldArrayF*>& structF,
			    std::vector<char*>& structVarString,
			    std::vector<E_Int>& nit,
			    std::vector<E_Int>& njt, 
			    std::vector<E_Int>& nkt,
			    std::vector<K_FLD::FldArrayF*>& unstrF,
			    std::vector<char*>& unstrVarString,
			    std::vector<K_FLD::FldArrayI*>& cnt,
			    std::vector<char*>& eltType,
			    std::vector<char*>& zoneNames,
			    std::vector<char*>& zoneTags,
          E_Int referenceNfield=-1,
          char** referenceVarNames=NULL);
  virtual void initState();

protected:
  //virtual void freeGPUResources( int mode, int start, int end, int permanent );
  //virtual void updateGPUResources( int mode, int size, int permanent, void* updatedPointer );

  virtual ZoneImpl* createZoneImpl( );
};


#endif
