/*    
    Copyright 2013-2024 Onera.

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
# include "Array/Array.h"
# include <string.h>

using namespace std;

//=============================================================================
/* Ajoute un suffixe a tous les noms de variables contenus dans varString. 
   IN: varString: la chaine des variables style "a,b,c"
   IN: suffix: la chaine de suffix a ajouter a chaque nom de variable
   OUT: varStringOut: la chaine resultat (doit etre deja allouee). */
//=============================================================================
void K_ARRAY::addSuffixInVarString(const char* varString, const char* suffix,
                                   char* varStringOut)
{
  vector<char*> vars;
  extractVars(varString, vars);
  E_Int size = vars.size();

  strcpy(varStringOut, "");
  for (E_Int v = 0; v < size; v++)
  {
    strcat(varStringOut, vars[v]);
    strcat(varStringOut, suffix);
    if (v < size-1) strcat(varStringOut, ",");
  } 

  for (E_Int v = 0; v < size; v++) delete [] vars[v];
}

//=============================================================================
/* Ajoute une etoile a tous les noms de variables contenus dans varString
   pour tagger ces variables comme etant definies aux centres
   IN: varString: la chaine des variables style "a,b,c"
   OUT: varStringOut: la chaine resultat "a*,b*,c*" (doit etre deja allouee). */
//=============================================================================
void K_ARRAY::starVarString(const char* varString, char* varStringOut)
{
  // Do not star twice
  if (strchr(varString, '*') == NULL)
    addSuffixInVarString(varString, "*", varStringOut);
  else strcpy(varStringOut, varString);
}

//=============================================================================
/* Retire un suffixe a tous les noms de variables contenus dans varString. 
   IN: varString: la chaine des variables style "a,b,c"
   IN: suffix: la chaine de suffix a retirer a chaque nom de variable
   OUT: varStringOut: la chaine resultat (doit etre deja allouee). */
//=============================================================================
void K_ARRAY::rmSuffixInVarString(const char* varString, const char* suffix,
                                  char* varStringOut)
{
  vector<char*> vars;
  extractVars(varString, vars);
  E_Int size = vars.size();
  E_Int lensuffix = strlen(suffix);

  strcpy(varStringOut, "");
  for (E_Int v = 0; v < size; v++)
  {
    char *p = strstr(vars[v], suffix);
    if (p)
    {
      // pattern found
      E_Int pos = p - vars[v];
      E_Int lenvar = strlen(vars[v]);
      // check if pattern is a suffix
      if (pos + lensuffix == lenvar) 
      {
        char* buffer = new char[VARNAMELENGTH];
        strncpy(buffer, vars[v], pos);
        strcat(varStringOut, buffer);
      }
      else strcat(varStringOut, vars[v]);
    }
    else strcat(varStringOut, vars[v]);
    if (v < size-1) strcat(varStringOut, ",");
  } 

  for (E_Int v = 0; v < size; v++) delete [] vars[v];
}

//=============================================================================
/* Retire l etoile de tous les noms de variables contenus dans varString
   pour tagger ces variables comme etant definies aux centres
   IN: varString: la chaine des variables style "a*,b*,c*"
   OUT: varStringOut: la chaine resultat "a,b,c" (doit etre deja allouee). */
//=============================================================================
void K_ARRAY::unstarVarString(const char* varString, char* varStringOut)
{
  if (strchr(varString, '*') != NULL)
    rmSuffixInVarString(varString, "*", varStringOut);
  else strcpy(varStringOut, varString);
}