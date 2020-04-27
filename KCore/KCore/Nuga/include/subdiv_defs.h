/*



NUGA



*/
//Authors : S�m Landier (sam.landier@onera.fr)

#ifndef SUBDIV_DEFS_H
#define SUBDIV_DEFS_H

namespace NUGA
{
  enum eSUBDIV_TYPE { ISO = 0, ISO_HEX=1/*all to hexa*/, ISO2/*iso metric field : spheres*/, DIR, ANISO/*aniso medtric field : ellipses*/ };
  enum eDIR { NONE = 0, Xd/*d because conflict with eClassifyer::X*/, Y, XY, /*XZ, YZ*/XYZ };
}

#endif