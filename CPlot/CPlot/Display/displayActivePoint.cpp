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
#include "../Data.h"

//============================================================================
/*
  Display active point 
  - red box.
  - pour les zones non-structurees: axes x,y,z
  - pour les zones Structurees: directions i,j,k
  - optionel: HUD
*/
//============================================================================
void Data::displayActivePoint()
{
  int nz = ptrState->selectedZone;
  if (nz == 0) return;
  if (_zones[nz-1]->active == 0) return;

  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-1., -10.);
  if (_view.clipping >= 1) glDisable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(1., 0.9, 0.9, 0.4);
  double x, y, z, dx, dy, dz, r, p;
  double ex, ey, ez;

  x = ptrState->activePointX;
  y = ptrState->activePointY;
  z = ptrState->activePointZ;

  ex = _view.xcam - _view.xeye;
  ey = _view.ycam - _view.yeye;
  ez = _view.zcam - _view.zeye;  

  p = ex*ex+ey*ey+ez*ez;
  p = 1./K_FUNC::E_max(sqrt(p), 1.e-12); // norme de e

  dx = _view.xcam - x;
  dy = _view.ycam - y;
  dz = _view.zcam - z;

  r = ex*dx + ey*dy + ez*dz;
  dx = r*ex*p; dy = r*ey*p; dz = r*ez*p;

  //r = dx*dx+dy*dy+dz*dz;
  r = sqrt(r)*0.005;
  p = 3.5*r;

  /* box */
  glBegin(GL_QUADS);
  glNormal3f(0,0,-1);
  glVertex3d(x-r, y-r, z-r);
  glVertex3d(x-r, y+r, z-r);
  glVertex3d(x+r, y+r, z-r);
  glVertex3d(x+r, y-r, z-r);
  
  glNormal3f(0,0,1);
  glVertex3d(x-r, y-r, z+r);
  glVertex3d(x-r, y+r, z+r);
  glVertex3d(x+r, y+r, z+r);
  glVertex3d(x+r, y-r, z+r);
  
  glNormal3f(-1,0,0);
  glVertex3d(x-r, y-r, z-r);
  glVertex3d(x-r, y+r, z-r);
  glVertex3d(x-r, y+r, z+r);
  glVertex3d(x-r, y-r, z+r);

  glNormal3f(1,0,0);
  glVertex3d(x+r, y-r, z-r);
  glVertex3d(x+r, y+r, z-r);
  glVertex3d(x+r, y+r, z+r);
  glVertex3d(x+r, y-r, z+r);

  glNormal3f(0,-1,0);
  glVertex3d(x-r, y-r, z-r);
  glVertex3d(x+r, y-r, z-r);
  glVertex3d(x+r, y-r, z+r);
  glVertex3d(x-r, y-r, z+r);

  glNormal3f(0,1,0);
  glVertex3d(x-r, y+r, z-r);
  glVertex3d(x+r, y+r, z-r);
  glVertex3d(x+r, y+r, z+r);
  glVertex3d(x-r, y+r, z+r);

  glEnd();

  nz = nz-1;
  
  if (nz < _numberOfStructZones)
  {
    glColor4f(1., 0., 0.,1.);
    glLineWidth(4.);
    glBegin(GL_LINES);

    /* Structurees: axes */
    StructZone* zp = (StructZone*)_zones[nz];
    int ni = zp->ni;
    int nj = zp->nj;
    int nk = zp->nk;
    int nij = ni*nj;
    int is = MAX(ptrState->activePointI-1,0); is = MIN(is, ni-1);
    int js = MAX(ptrState->activePointJ-1,0); js = MIN(js, nj-1);
    int ks = MAX(ptrState->activePointK-1,0); ks = MIN(ks, nk-1);
    int ind = is + js*ni + ks*nij;
    double* px = zp->x;
    double* py = zp->y;
    double* pz = zp->z;
    double vx, vy, vz, n;
    int indp, ip, jp, kp;

    // axe i
    if (is < ni-1 && ni > 1)
    {
      ip = is+1; jp = js; kp = ks;
      indp = ip + jp*ni + kp*nij;
      vx = px[indp]-px[ind];
      vy = py[indp]-py[ind];
      vz = pz[indp]-pz[ind];
      n = sqrt(vx*vx + vy*vy + vz*vz);
      n = 1./K_FUNC::E_max(n, 1.e-10);
      vx *= n; vy *= n; vz *= n;
      vx = vx*p; vy = vy*p; vz = vz*p;
      glColor4f(1.,0.,0.,1.);
      glVertex3d(x, y, z);
      glVertex3d(x+vx, y+vy, z+vz);
    }
    else if (ni > 1)
    {
      ip = is-1; jp = js; kp = ks;
      indp = ip + jp*ni + kp*nij;
      vx = px[indp]-px[ind];
      vy = py[indp]-py[ind];
      vz = pz[indp]-pz[ind];
      n = sqrt(vx*vx + vy*vy + vz*vz);
      n = 1./K_FUNC::E_max(n, 1.e-10);
      vx *= n; vy *= n; vz *= n;
      vx = -vx*p; vy = -vy*p; vz = -vz*p;
      glColor4f(1.,0.,0.,1.);
      glVertex3d(x, y, z);
      glVertex3d(x+vx, y+vy, z+vz);
    }
    
    // axe j
    if (js < nj-1 && nj > 1)
    {
      ip = is; jp = js+1; kp = ks;
      indp = ip + jp*ni + kp*nij;
      vx = px[indp]-px[ind];
      vy = py[indp]-py[ind];
      vz = pz[indp]-pz[ind];
      n = sqrt(vx*vx + vy*vy + vz*vz);
      n = 1./K_FUNC::E_max(n, 1.e-10);
      vx *= n; vy *= n; vz *= n;
      vx = vx*p; vy = vy*p; vz = vz*p;
      glColor4f(0.,1.,0.,1.);
      glVertex3d(x, y, z);
      glVertex3d(x+vx, y+vy, z+vz);
    }
    else if (nj > 1)
    {
      ip = is; jp = js-1; kp = ks;
      indp = ip + jp*ni + kp*nij;
      vx = px[indp]-px[ind];
      vy = py[indp]-py[ind];
      vz = pz[indp]-pz[ind];
      n = sqrt(vx*vx + vy*vy + vz*vz);
      n = 1./K_FUNC::E_max(n, 1.e-10);
      vx *= n; vy *= n; vz *= n;
      vx = -vx*p; vy = -vy*p; vz = -vz*p;
      glColor4f(0.,1.,0.,1.);
      glVertex3d(x, y, z);
      glVertex3d(x+vx, y+vy, z+vz);
    }

    // axe k
    if (ks < nk-1 && nk > 1)
    {
      ip = is; jp = js; kp = ks+1;
      indp = ip + jp*ni + kp*nij;
      vx = px[indp]-px[ind];
      vy = py[indp]-py[ind];
      vz = pz[indp]-pz[ind];
      n = sqrt(vx*vx + vy*vy + vz*vz);
      n = 1./K_FUNC::E_max(n, 1.e-10);
      vx *= n; vy *= n; vz *= n;
      vx = vx*p; vy = vy*p; vz = vz*p;
      glColor4f(0.,0.,1.,1.);
      glVertex3d(x, y, z);
      glVertex3d(x+vx, y+vy, z+vz);
    }
    else if (nk > 1)
    {
      ip = is; jp = js; kp = ks-1;
      indp = ip + jp*ni + kp*nij;
      vx = px[indp]-px[ind];
      vy = py[indp]-py[ind];
      vz = pz[indp]-pz[ind];
      n = sqrt(vx*vx + vy*vy + vz*vz);
      n = 1./K_FUNC::E_max(n, 1.e-10);
      vx *= n; vy *= n; vz *= n;
      vx = -vx*p; vy = -vy*p; vz = -vz*p;
      glColor4f(1.,0.,1.,1.);
      glVertex3d(x, y, z);
      glVertex3d(x+vx, y+vy, z+vz);
    }
    glEnd();
  }
  else
  {
    /* Non structurees: axes */
    glLineWidth(4.);
    glColor4f(1., 0., 0.,1.);
    glBegin(GL_LINES);
    glVertex3d(x-p, y, z);
    glVertex3d(x+p, y, z);
    glVertex3d(x, y-p, z);
    glVertex3d(x, y+p, z);
    glVertex3d(x, y, z-p);
    glVertex3d(x, y, z+p);
    glEnd();
  }

  /* HUD (try) */
  glDisable(GL_DEPTH_TEST);
  double dirx, diry, dirz, vx, vy, vz, n;
  dirx = _view.dirx; diry = _view.diry; dirz = _view.dirz;
  vx = diry*dz - dirz*dy;
  vy = dirz*dx - dirx*dz;
  vz = dirx*dy - diry*dx;
  n = sqrt(vx*vx+vy*vy+vz*vz);
  n = 1./K_FUNC::E_max(n, 1.e-12);
  vx *= n; vy *= n; vz *= n;

  glLineWidth(2.);
  glColor4f(1., 0., 0.,1.);
  glBegin(GL_LINES);
  glVertex3d(x, y, z);
  glVertex3d(x+5*r*(vx+dirx), y+5*r*(vy+diry), z+5*r*(vz+dirz));
  glVertex3d(x+5*r*(vx+dirx), y+5*r*(vy+diry), z+5*r*(vz+dirz));
  glVertex3d(x+5*r*(2*vx+dirx), y+5*r*(2*vy+diry), z+5*r*(2*vz+dirz));
  glEnd();
  glLineWidth(1.);
  
  char msg[512]; char msg2[512]; msg2[0] = '\0';
  if (ptrState->selectedZone > 0)
  {
    Zone* zone = _zones[ptrState->selectedZone-1];
    if (ptrState->selectedZone-1 < _numberOfStructZones)
    {
      /*
      if (ptrState->mode == MESH) // xyz + ijk
      {
        sprintf(msg, "%g, %g, %g", 
                ptrState->activePointX, ptrState->activePointY, ptrState->activePointZ);
        sprintf(msg2, "(i=%d, j=%d, k=%d)",
                ptrState->activePointI, ptrState->activePointJ, abs(ptrState->activePointK));
      }*/
      if (ptrState->mode == SCALARFIELD)
      {
        sprintf(msg, "%s=%g",
                zone->varnames[ptrState->scalarField], 
                ptrState->activePointF[ptrState->scalarField]);
      }
      else if (ptrState->mode == VECTORFIELD)
      {
        sprintf(msg, "%s=%g, %s=%g, %s=%g",
                zone->varnames[ptrState->vectorField1], 
                zone->f[ptrState->vectorField1][ptrState->activePointI],
                zone->varnames[ptrState->vectorField2],
                zone->f[ptrState->vectorField2][ptrState->activePointI],
                zone->varnames[ptrState->vectorField3],
                zone->f[ptrState->vectorField3][ptrState->activePointI]);
      }
      else
        sprintf(msg,"%s (STRUCT)", zone->zoneName);
    }
    else
    {
      UnstructZone* zu = (UnstructZone*)zone;
      /*
      if (ptrState->mode == MESH) // xyz + np,ne
      {
        sprintf(msg, "%g, %g, %g", 
        ptrState->activePointX, ptrState->activePointY, ptrState->activePointZ);
        sprintf(msg2, "(np=%d, ne=%d, nf=%d)",
                ptrState->activePointI, ptrState->activePointJ, abs(ptrState->activePointK));
      }*/
      if (ptrState->mode == SCALARFIELD)
      {
        sprintf(msg, "%s=%g",
                zone->varnames[ptrState->scalarField], 
                ptrState->activePointF[ptrState->scalarField]);
      }
      else if (ptrState->mode == VECTORFIELD)
      {
        sprintf(msg, "%s=%g, %s=%g, %s=%g",
                zone->varnames[ptrState->vectorField1], 
                zone->f[ptrState->vectorField1][ptrState->activePointI],
                zone->varnames[ptrState->vectorField2],
                zone->f[ptrState->vectorField2][ptrState->activePointI],
                zone->varnames[ptrState->vectorField3],
                zone->f[ptrState->vectorField3][ptrState->activePointI]);
      }
      else
      {
        switch (zu->eltType)
        {
          case 0:
            sprintf(msg,"%s (NODE)", zone->zoneName); break;
          case 1:
            sprintf(msg,"%s (BAR)", zone->zoneName); break;
          case 2:
            sprintf(msg,"%s (TRI)", zone->zoneName); break;
          case 3:
            sprintf(msg,"%s (QUAD)", zone->zoneName); break;
          case 4:
            sprintf(msg,"%s (TETRA)", zone->zoneName); break;
          case 5:
            sprintf(msg,"%s (PENTA)", zone->zoneName); break;
          case 6:
            sprintf(msg,"%s (PYRA)", zone->zoneName); break;
          case 7:
            sprintf(msg,"%s (HEXA)", zone->zoneName); break;
          case 10:
            sprintf(msg,"%s (NGON)", zone->zoneName); break;
          default:
            sprintf(msg,"%s (UNKNOWN)", zone->zoneName); break;
        }
      }
    }

    // Patch by the CB
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    double winx,winy,winz;
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    gluProject(x, y, z, modelview, projection,
               viewport, &winx, &winy, &winz);
    double posX,posY,posZ;
    double shtx, shty, shtz;
    double shnx, shny, shnz;
    double ptx, pty, ptz;
    ptx = x+5.*r*(2.2*vx+dirx);
    pty = y+5.*r*(2.2*vy+diry);
    ptz = z+5.*r*(2.2*vz+dirz);
    gluUnProject(winx+1., winy, winz, modelview, projection, viewport, 
                 &posX, &posY, &posZ);
    shtx = posX-x; shty = posY-y; shtz = posZ-z;
    gluUnProject(winx, winy+1., winz, modelview, projection, viewport, 
                 &posX, &posY, &posZ);
    shnx = posX-x; shny = posY-y; shnz = posZ-z;
    
    // z offset - inutile sans depth test
    /*
    double crossx, crossy, crossz;
    crossx = shty*shnz-shtz*shny;
    crossy = shtz*shnx-shtx*shnz;
    crossz = shtx*shny-shty*shnx;
    if (crossx > 0) crossx = sqrt(crossx);
    else crossx = -sqrt(-crossx);
    if (crossy > 0) crossy = sqrt(crossy);
    else crossy = -sqrt(-crossy);
    if (crossz > 0) crossz = sqrt(crossz);
    else crossz = -sqrt(-crossz);
    ptx = ptx+10*crossx;
    pty = pty+10*crossy;
    ptz = ptz+10*crossz;
    */
    
    // render rectangle
    int width = textWidth(FONT3, msg);
    int height = textHeight(FONT3);
    double x0 = ptx-5*shtx-8*shnx;
    double y0 = pty-5*shty-8*shny;
    double z0 = ptz-5*shtz-8*shnz;
    double x1 = x0+2*5*shtx+width*shtx;
    double y1 = y0+2*5*shty+width*shty;
    double z1 = z0+2*5*shtz+width*shtz;
    double x2 = x1+height*shnx+13*shnx;
    double y2 = y1+height*shny+13*shny;
    double z2 = z1+height*shnz+13*shnz;
    double x3 = x0+height*shnx+13*shnx;
    double y3 = y0+height*shny+13*shny;
    double z3 = z0+height*shnz+13*shnz;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0, 0.0, 1., 0.7);
    glBegin(GL_QUADS);
    glVertex3d(x0,y0,z0);
    glVertex3d(x1,y1,z1);
    glVertex3d(x2,y2,z2);
    glVertex3d(x3,y3,z3);
    glEnd();    
    glDisable(GL_BLEND);
    
    glColor4f(1.0, 1.0, 1., 1.);
    glBegin(GL_LINES);
    glVertex3d(x0,y0,z0);
    glVertex3d(x1,y1,z1);
    glVertex3d(x1,y1,z1);
    glVertex3d(x2,y2,z2);
    glVertex3d(x2,y2,z2);
    glVertex3d(x3,y3,z3);
    glVertex3d(x3,y3,z3);
    glVertex3d(x0,y0,z0);
    glEnd();

    // render HUD string
    renderStringWithShadow(ptx, pty, ptz, 
                           FONT3, msg,
                           1., 1., 1., 1., 
                           0.1, 0.1, 0.1, 0.5,
                           shtx, shty, shtz,
                           shnx, shny, shnz,
                           1.);
    if (msg2[0] != '\0') // deuxieme ligne eventuelle
      renderStringWithShadow(ptx-(20+height)*shnx, pty-(20+height)*shny, ptz-(20+height)*shnz, 
                            FONT3, msg2,
                            1., 1., 1., 1., 
                            0.1, 0.1, 0.1, 0.5,
                            shtx, shty, shtz,
                            shnx, shny, shnz,
                            1.);
  }
 
  /* End of HUD */

  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glLineWidth(1.);
  glColor4f(1., 1., 1., 1.);
}
