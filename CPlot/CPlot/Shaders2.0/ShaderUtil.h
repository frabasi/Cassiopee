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
#ifndef _CPLOT_SHADERUTIL_HPP_
#define _CPLOT_SHADERUTIL_HPP_
#include <stdio.h>

//#define DEBUG_SHADER 

#ifdef DEBUG_SHADER
#include <GL/glu.h>
static int checkGLError(char *file, int line)
{
  GLenum glErr;
  int    retCode = 0;
  
  glErr = glGetError();
  while (glErr != GL_NO_ERROR) 
  {
    const GLubyte* sError = gluErrorString(glErr);

    if (sError)
      printf("GL Error # %d (%s) in file %s at line %d.\n",
             glErr, gluErrorString(glErr), file, line);
    else
       printf("GL Error # %d (no message) in file %s at line %d.\n",
             glErr, file, line);
			
    retCode = 1;
    glErr = glGetError();
  }
  return retCode;
}
#define CHECK_GL_ERROR() checkGLError((char*)__FILE__, __LINE__)
#else
#define CHECK_GL_ERROR()
#endif
#endif
