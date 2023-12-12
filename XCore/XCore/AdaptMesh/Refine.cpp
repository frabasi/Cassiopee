#include "Proto.h"

static
void update_external_own_nei(E_Int cell, AMesh *M)
{
  Element *Elem = M->cellTree[cell];

  for (E_Int i = 0; i < Elem->nchildren; i++) {
    E_Int child = Elem->children[i];
    for (E_Int j = M->indPH[child]; j < M->indPH[child+1]; j++) {
      E_Int face = M->nface[j];
      if      (M->owner[face] == cell) M->owner[face] = child;
      else if (M->neigh[face] == cell) M->neigh[face] = child;
    }
  }
}

void get_ref_cells_and_faces(AMesh *M, std::vector<E_Int> &ref_cells,
  std::vector<E_Int> &ref_faces)
{
  ref_cells.clear();
  ref_faces.clear();

  E_Int *vcells = (E_Int *)XCALLOC(M->ncells, sizeof(E_Int));

  for (E_Int i = 0; i < M->nfaces; i++) {
    E_Int own = M->owner[i];
    if (M->ref_data[3*own] > 0) {
      ref_faces.push_back(i);
      if (!vcells[own]) {
        vcells[own] = 1;
        ref_cells.push_back(own);
      }
      continue;
    }

    E_Int nei = M->neigh[i];
    if (nei == -1) continue;

    if (M->ref_data[3*nei] > 0) {
      ref_faces.push_back(i);
      if (!vcells[nei]) {
        vcells[nei] = 1;
        ref_cells.push_back(nei);
      }
    }
  }

  XFREE(vcells);
}

static
void set_child_elem(Element **elemTree, E_Int parent, E_Int cpos, E_Int type,
  E_Int next_level, E_Int nelem)
{
  E_Int pos = nelem + cpos;
  assert(elemTree[pos]->children == NULL);
  elemTree[pos]->nchildren = 0;
  elemTree[pos]->parent = parent;
  elemTree[pos]->position = cpos;
  elemTree[pos]->type = type;
  elemTree[pos]->level = next_level;
}

static
void set_parent_elem(Element **elemTree, E_Int elem, E_Int nchildren,
  E_Int nelem)
{
  elemTree[elem]->children = (E_Int *)XMALLOC(nchildren * sizeof(E_Int));
  for (E_Int i = 0; i < nchildren; i++)
    elemTree[elem]->children[i] = nelem + i;
  elemTree[elem]->nchildren = nchildren;
}

static
void refine_tri(E_Int face, AMesh *M)
{
  if (M->faceTree[face]->nchildren > 0) return;

  E_Int *pn = get_facets(face, M->ngon, M->indPG);

  Edge E;
  E_Int ec[3]; // Edge center nodes

  for (E_Int i = 0; i < 3; i++) {
    E_Int ni = pn[i];
    E_Int nj = pn[(i+1)%3];
    E.set(ni, nj);

    auto search = M->ecenter.find(E);
    if (search == M->ecenter.end()) {
      M->x[M->npoints] = 0.5*(M->x[ni] + M->x[nj]);
      M->y[M->npoints] = 0.5*(M->y[ni] + M->y[nj]);
      M->z[M->npoints] = 0.5*(M->z[ni] + M->z[nj]);
      ec[i] = M->npoints;
      M->ecenter[E] = M->npoints++;
    } else {
      ec[i] = search->second;
    }
  }

  // Set faces in ngon, starting from ptr
  E_Int *ptr = &M->indPG[M->nfaces];
  
  // First face
  ptr[1] = *ptr + 3;
  M->ngon[*ptr    ] = pn[0];
  M->ngon[*ptr + 1] = ec[0];
  M->ngon[*ptr + 2] = ec[2];
  ptr++;

  // Second face
  ptr[1] = *ptr + 3;
  M->ngon[*ptr    ] = ec[0];
  M->ngon[*ptr + 1] = pn[1];
  M->ngon[*ptr + 2] = ec[1];
  ptr++;

  // Third face
  ptr[1] = *ptr + 3;
  M->ngon[*ptr    ] = ec[2];
  M->ngon[*ptr + 1] = ec[1];
  M->ngon[*ptr + 2] = pn[2];
  ptr++;

  // Fourth face
  ptr[1] = *ptr + 3;
  M->ngon[*ptr    ] = ec[0];
  M->ngon[*ptr + 1] = ec[1];
  M->ngon[*ptr + 2] = ec[2];

  // Set faces in faceTree
  E_Int next_level = M->faceTree[face]->level + 1;
  set_parent_elem(M->faceTree, face, 4, M->nfaces);
  for (E_Int i = 0; i < 4; i++)
    set_child_elem(M->faceTree, face, i, TRI, next_level, M->nfaces);

  // Own and nei
  M->owner[M->nfaces] = M->owner[face];
  M->neigh[M->nfaces] = M->neigh[face];
  M->owner[M->nfaces+1] = M->owner[face];
  M->neigh[M->nfaces+1] = M->neigh[face];
  M->owner[M->nfaces+2] = M->owner[face];
  M->neigh[M->nfaces+2] = M->neigh[face];
  M->owner[M->nfaces+3] = M->owner[face];
  M->neigh[M->nfaces+3] = M->neigh[face];

  M->nfaces += 4;
}

static
void T6_get_ordered_data(AMesh *M, E_Int i0, E_Int reorient,
  E_Int *children, E_Int *pn, E_Int *local)
{
  for (E_Int i = 0; i < 3; i++) local[i] = pn[i];

  E_Int *pn0 = &M->ngon[M->indPG[children[0]]];
  E_Int *pn1 = &M->ngon[M->indPG[children[1]]];
  E_Int *pn2 = &M->ngon[M->indPG[children[2]]];

  assert(pn0[0] == local[0]);
  assert(pn1[1] == local[1]);
  assert(pn2[2] == local[2]);

  local[3] = pn0[1];
  local[4] = pn1[2];
  local[5] = pn2[0];

  Right_shift(local, i0, 3);
  Right_shift(local+3, i0, 3);
  Right_shift(children, i0, 3);

  if (reorient) {
    std::swap(children[1], children[2]);
    std::swap(local[1], local[2]);
    std::swap(local[3], local[5]);
  }
}

static
void refine_tetra(E_Int cell, AMesh *M)
{
  E_Int NODES[10], FACES[16];
  E_Int *BOT = FACES;
  E_Int *LFT = FACES + 4;
  E_Int *RGT = FACES + 8;
  E_Int *FRO = FACES + 12;
  E_Int *pf = get_facets(cell, M->nface, M->indPH);

  E_Int face, i0, reorient, *children, nchildren, *pn, local[6];
  Element **faceTree = M->faceTree;

  // BOT
  face = pf[0];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = 0;
  reorient = get_reorient(face, cell, normalIn_T[0], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) BOT[i] = children[i]; 
  T6_get_ordered_data(M, i0, reorient, BOT, pn, local);
  for (E_Int i = 0; i < 3; i++) NODES[i] = local[i];
  NODES[4] = local[3];
  NODES[5] = local[4];
  NODES[6] = local[5];

  // LFT
  face = pf[1];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 3);
  reorient = get_reorient(face, cell, normalIn_T[1], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) LFT[i] = children[i]; 
  T6_get_ordered_data(M, i0, reorient, LFT, pn, local);
  assert(local[5] == NODES[6]);
  NODES[3] = local[1];
  NODES[7] = local[4];
  NODES[8] = local[3];

  // RGT
  face = pf[2];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 3);
  reorient = get_reorient(face, cell, normalIn_T[2], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) RGT[i] = children[i]; 
  T6_get_ordered_data(M, i0, reorient, RGT, pn, local);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[3]); 
  assert(local[2] == NODES[2]);
  assert(local[4] == NODES[7]);
  assert(local[5] == NODES[5]);
  NODES[9] = local[3];

  // FRO
  face = pf[3];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 3);
  reorient = get_reorient(face, cell, normalIn_T[3], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) FRO[i] = children[i]; 
  T6_get_ordered_data(M, i0, reorient, FRO, pn, local);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[1]);
  assert(local[2] == NODES[3]);
  assert(local[3] == NODES[4]);
  assert(local[4] == NODES[9]);
  assert(local[5] == NODES[8]);

  // Set internal faces in ngon
  E_Int *ptr = &M->indPG[M->nfaces];

  // nfaces
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[4];  M->ngon[*ptr+1] = NODES[8];
  M->ngon[*ptr+2] = NODES[6];
  ptr++;
  
  // nfaces+1
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[4];  M->ngon[*ptr+1] = NODES[9];
  M->ngon[*ptr+2] = NODES[5];
  ptr++; 

  // nfaces+2
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[6];  M->ngon[*ptr+1] = NODES[5];
  M->ngon[*ptr+2] = NODES[7];
  ptr++;
  
  // nfaces+3
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[8];  M->ngon[*ptr+1] = NODES[9];
  M->ngon[*ptr+2] = NODES[7];
  ptr++;

  // Connect nodes 4 and 7 -> four new faces
  // TODO(Imad): test the other two choices of diagonal
  // nfaces+4 (const)
  ptr[1] = ptr[0] + 3; 
  M->ngon[*ptr  ] = NODES[6];  M->ngon[*ptr+1] = NODES[4];
  M->ngon[*ptr+2] = NODES[7];
  ptr++;
  
  // nfaces+5
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[4];  M->ngon[*ptr+1] = NODES[9];
  M->ngon[*ptr+2] = NODES[7];
  ptr++;
  
  // nfaces+6 (const)
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[4];  M->ngon[*ptr+1] = NODES[8];
  M->ngon[*ptr+2] = NODES[7];
  ptr++;
  
  // nfaces+7
  ptr[1] = ptr[0] + 3;
  M->ngon[*ptr  ] = NODES[4];  M->ngon[*ptr+1] = NODES[7];
  M->ngon[*ptr+2] = NODES[5];


  // Assemble children
  ptr = &M->indPH[M->ncells]; 
  // ncells
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = BOT[0];      M->nface[*ptr+1] = LFT[0];
  M->nface[*ptr+2] = M->nfaces;   M->nface[*ptr+3] = FRO[0];
  ptr++;

  // ncells+1
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = BOT[1];      M->nface[*ptr+1] = M->nfaces+1;
  M->nface[*ptr+2] = RGT[0];      M->nface[*ptr+3] = FRO[1];
  ptr++;

  // ncells+2
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = BOT[2];      M->nface[*ptr+1] = LFT[2];
  M->nface[*ptr+2] = RGT[2];      M->nface[*ptr+3] = M->nfaces+2;
  ptr++;
  
  // ncells+3
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = M->nfaces+3; M->nface[*ptr+1] = LFT[1];
  M->nface[*ptr+2] = RGT[1];      M->nface[*ptr+3] = FRO[2];
  ptr++;

  // Octahedron -> 4 new tetra (clockwise)
  // ncells+4
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = M->nfaces+4; M->nface[*ptr+1] = LFT[3];
  M->nface[*ptr+2] = M->nfaces+6; M->nface[*ptr+3] = M->nfaces;
  ptr++;
 
  // ncells+5
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = M->nfaces+5; M->nface[*ptr+1] = M->nfaces+6;
  M->nface[*ptr+2] = M->nfaces+3; M->nface[*ptr+3] = FRO[3];
  ptr++;

  // ncells+6
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = M->nfaces+2; M->nface[*ptr+1] = M->nfaces+4;
  M->nface[*ptr+2] = M->nfaces+7; M->nface[*ptr+3] = BOT[3];
  ptr++;
  
  // ncells+7
  ptr[1] = ptr[0] + 4;
  M->nface[*ptr  ] = M->nfaces+1; M->nface[*ptr+1] = M->nfaces+7;
  M->nface[*ptr+2] = RGT[3]; M->nface[*ptr+3] = M->nfaces+5;

  // Set new cells in tree
  E_Int next_level = M->cellTree[cell]->level + 1;
  set_parent_elem(M->cellTree, cell, 8, M->ncells);
  for (E_Int i = 0; i < 8; i++)
    set_child_elem(M->cellTree, cell, i, TETRA, next_level, M->ncells);

  // Set external faces owns and neis
  update_external_own_nei(cell, M);

  // Set owns and neis of internal faces
  M->owner[M->nfaces]   = M->ncells+4; M->neigh[M->nfaces]   = M->ncells+0;
  M->owner[M->nfaces+1] = M->ncells+1; M->neigh[M->nfaces+1] = M->ncells+7;
  M->owner[M->nfaces+2] = M->ncells+2; M->neigh[M->nfaces+2] = M->ncells+6;
  M->owner[M->nfaces+3] = M->ncells+5; M->neigh[M->nfaces+3] = M->ncells+3;

  M->owner[M->nfaces+4] = M->ncells+6; M->neigh[M->nfaces+4] = M->ncells+4;
  M->owner[M->nfaces+5] = M->ncells+7; M->neigh[M->nfaces+5] = M->ncells+5;
  M->owner[M->nfaces+6] = M->ncells+5; M->neigh[M->nfaces+6] = M->ncells+4;
  M->owner[M->nfaces+7] = M->ncells+7; M->neigh[M->nfaces+7] = M->ncells+6;

  for (E_Int i = 0; i < 8; i++)
    check_canon_tetra(M->ncells+i, M);

  M->ncells += 8;
  M->nfaces += 8;
}

static
void refine_quad(E_Int face, AMesh *M)
{
  if (M->faceTree[face]->nchildren > 0) return;

  E_Int *pn = get_facets(face, M->ngon, M->indPG);

  Edge E;
  E_Int ec[4]; // Edge center nodes

  for (E_Int i = 0; i < 4; i++) {
    E_Int ni = pn[i];
    E_Int nj = pn[(i+1)%4];
    E.set(ni, nj);

    auto search = M->ecenter.find(E);
    if (search == M->ecenter.end()) {
      M->x[M->npoints] = 0.5*(M->x[ni] + M->x[nj]);
      M->y[M->npoints] = 0.5*(M->y[ni] + M->y[nj]);
      M->z[M->npoints] = 0.5*(M->z[ni] + M->z[nj]);
      ec[i] = M->npoints;
      M->ecenter[E] = M->npoints++;
    } else {
      ec[i] = search->second;
    }
  }

  // Note(Imad): face center is supposed to be already computed
  E_Float *fc = &M->fc[3*face];
  M->x[M->npoints] = fc[0];
  M->y[M->npoints] = fc[1];
  M->z[M->npoints] = fc[2];
  E_Int ncenter = M->npoints++;

  // Set faces in ngon, starting from ptr
  E_Int *ptr = &M->indPG[M->nfaces];

  // First face
  ptr[1] = *ptr + 4;
  M->ngon[*ptr    ] = pn[0];
  M->ngon[*ptr + 1] = ec[0];
  M->ngon[*ptr + 2] = ncenter;
  M->ngon[*ptr + 3] = ec[3];
  ptr++;

  // Second face
  ptr[1] = *ptr + 4;
  M->ngon[*ptr    ] = ec[0];
  M->ngon[*ptr + 1] = pn[1];
  M->ngon[*ptr + 2] = ec[1];
  M->ngon[*ptr + 3] = ncenter;
  ptr++;

  // Third face
  ptr[1] = *ptr + 4;
  M->ngon[*ptr    ] = ncenter;
  M->ngon[*ptr + 1] = ec[1];
  M->ngon[*ptr + 2] = pn[2];
  M->ngon[*ptr + 3] = ec[2];
  ptr++;

  // Fourth face
  ptr[1] = *ptr + 4;
  M->ngon[*ptr    ] = ec[3];
  M->ngon[*ptr + 1] = ncenter;
  M->ngon[*ptr + 2] = ec[2];
  M->ngon[*ptr + 3] = pn[3];

  // Set faces in faceTree
  E_Int next_level = M->faceTree[face]->level + 1;
  set_parent_elem(M->faceTree, face, 4, M->nfaces);
  for (E_Int i = 0; i < 4; i++)
    set_child_elem(M->faceTree, face, i, QUAD, next_level, M->nfaces);

  M->owner[M->nfaces] = M->owner[face];
  M->neigh[M->nfaces] = M->neigh[face];
  M->owner[M->nfaces+1] = M->owner[face];
  M->neigh[M->nfaces+1] = M->neigh[face];
  M->owner[M->nfaces+2] = M->owner[face];
  M->neigh[M->nfaces+2] = M->neigh[face];
  M->owner[M->nfaces+3] = M->owner[face];
  M->neigh[M->nfaces+3] = M->neigh[face];

  M->nfaces += 4;
}

static
void Q9_get_ordered_data(AMesh *M, E_Int i0, E_Int reorient,
  E_Int *children, E_Int *pn, E_Int *local)
{
  E_Int *pn0 = &M->ngon[M->indPG[children[0]]];
  E_Int *pn2 = &M->ngon[M->indPG[children[2]]];

  for (E_Int i = 0; i < 4; i++) local[i] = pn[i];

  local[4] = pn0[1];
  local[5] = pn2[1];
  local[6] = pn2[3];
  local[7] = pn0[3];
  local[8] = pn0[2];

  Right_shift(local, i0, 4);
  Right_shift(local+4, i0, 4);
  Right_shift(children, i0, 4);

  if (reorient) {
  	std::swap(local[1], local[3]);
  	std::swap(local[4], local[7]);
  	std::swap(local[5], local[6]);
  	std::swap(children[1], children[3]);
  }
}

static
void refine_hexa(E_Int cell, AMesh *M)
{
  E_Int NODES[27], FACES[24];
  for (E_Int i = 0; i < 27; i++) NODES[i] = -1;
  E_Int *BOT = FACES;
  E_Int *TOP = FACES + 4;
  E_Int *LFT = FACES + 8;
  E_Int *RGT = FACES + 12;
  E_Int *FRO = FACES + 16;
  E_Int *BCK = FACES + 20;
  E_Int *pf = get_facets(cell, M->nface, M->indPH);

  E_Int face, i0, reorient, *children, nchildren, *pn, local[9];
  Element **faceTree = M->faceTree;

  // BOT
  face = pf[0];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = 0;
  reorient = get_reorient(face, cell, normalIn_H[0], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) BOT[i] = children[i]; 
  Q9_get_ordered_data(M, i0, reorient, BOT, pn, local);
  for (E_Int i = 0; i < 4; i++) NODES[i] = local[i];
  NODES[8] = local[4];
  NODES[9] = local[5];
  NODES[10] = local[6];
  NODES[11] = local[7];
  NODES[12] = local[8];

  // LFT
  face = pf[2];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[2], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) LFT[i] = children[i]; 
  Q9_get_ordered_data(M, i0, reorient, LFT, pn, local);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[3]);
  assert(local[4] == NODES[11]);
  NODES[7] = local[2];
  NODES[4] = local[3];
  NODES[13] = local[5];
  NODES[14] = local[6];
  NODES[15] = local[7];
  NODES[16] = local[8];

  // RGT
  face = pf[3];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[3], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) RGT[i] = children[i]; 
  Q9_get_ordered_data(M, i0, reorient, RGT, pn, local);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[2]);
  assert(local[4] == NODES[9]);
  NODES[6] = local[2];
  NODES[5] = local[3];
  NODES[17] = local[5];
  NODES[18] = local[6];
  NODES[19] = local[7];
  NODES[20] = local[8];

  // FRO
  face = pf[4];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[4], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) FRO[i] = children[i]; 
  Q9_get_ordered_data(M, i0, reorient, FRO, pn, local);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[0]);
  assert(local[2] == NODES[4]);
  assert(local[3] == NODES[5]);
  assert(local[4] == NODES[8]);
  assert(local[5] == NODES[15]);
  assert(local[7] == NODES[19]);
  NODES[21] = local[6];
  NODES[22] = local[8];

  // BCK
  face = pf[5];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[2], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[5], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) BCK[i] = children[i]; 
  Q9_get_ordered_data(M, i0, reorient, BCK, pn, local);
  assert(local[0] == NODES[2]);
  assert(local[1] == NODES[3]);
  assert(local[2] == NODES[7]);
  assert(local[3] == NODES[6]);
  assert(local[4] == NODES[10]);
  assert(local[5] == NODES[13]);
  assert(local[7] == NODES[17]);
  NODES[23] = local[6];
  NODES[24] = local[8];

  // TOP
  face = pf[1];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[4], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[1], M);
  children = faceTree[face]->children;
  assert(children);
  nchildren = faceTree[face]->nchildren;
  assert(nchildren == 4);
  for (E_Int i = 0; i < nchildren; i++) TOP[i] = children[i]; 
  Q9_get_ordered_data(M, i0, reorient, TOP, pn, local);
  assert(local[0] == NODES[4]);
  assert(local[1] == NODES[5]);
  assert(local[2] == NODES[6]);
  assert(local[3] == NODES[7]);
  assert(local[4] == NODES[21]);
  assert(local[5] == NODES[18]);
  assert(local[6] == NODES[23]);
  assert(local[7] == NODES[14]);
  NODES[25] = local[8];

  // Add cell centroid
  // Note(Imad): supposed to be already computed
  NODES[26] = M->npoints;
  M->x[M->npoints] = M->cx[cell];
  M->y[M->npoints] = M->cy[cell];
  M->z[M->npoints] = M->cz[cell];
  M->npoints++;

  // Set internal faces in ngon
  E_Int *ptr = &M->indPG[M->nfaces];

  // NCELLS TOP && RGT && BCK

  // nfaces
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[15]; M->ngon[*ptr+1] = NODES[22];
  M->ngon[*ptr+2] = NODES[26]; M->ngon[*ptr+3] = NODES[16];
  ptr++;

  // nfaces+1
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[8];  M->ngon[*ptr+1] = NODES[12];
  M->ngon[*ptr+2] = NODES[26]; M->ngon[*ptr+3] = NODES[22];
  ptr++;

  // nfaces+2
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[12]; M->ngon[*ptr+1] = NODES[11];
  M->ngon[*ptr+2] = NODES[16]; M->ngon[*ptr+3] = NODES[26];
  ptr++;

  // NCELLS+1 TOP && BCK

  // nfaces+3
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[22]; M->ngon[*ptr+1] = NODES[19];
  M->ngon[*ptr+2] = NODES[20]; M->ngon[*ptr+3] = NODES[26];
  ptr++;

  // nfaces+4
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[9]; M->ngon[*ptr+1] = NODES[12];
  M->ngon[*ptr+2] = NODES[26]; M->ngon[*ptr+3] = NODES[20];
  ptr++;

  // NCELLS+2 TOP && LFT

  // nfaces+5
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[26]; M->ngon[*ptr+1] = NODES[20];
  M->ngon[*ptr+2] = NODES[17]; M->ngon[*ptr+3] = NODES[24];
  ptr++;

  // nfaces+6
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[12]; M->ngon[*ptr+1] = NODES[10];
  M->ngon[*ptr+2] = NODES[24]; M->ngon[*ptr+3] = NODES[26];
  ptr++;

  // NCELLS+3 TOP

  // nfaces+7
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[16];  M->ngon[*ptr+1] = NODES[26];
  M->ngon[*ptr+2] = NODES[24]; M->ngon[*ptr+3] = NODES[13];
  ptr++;


  // NCELLS+4 RGT && BCK

  // nfaces+8
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[22]; M->ngon[*ptr+1] = NODES[26];
  M->ngon[*ptr+2] = NODES[25]; M->ngon[*ptr+3] = NODES[21];
  ptr++;

  // nfaces+9
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[26]; M->ngon[*ptr+1] = NODES[16];
  M->ngon[*ptr+2] = NODES[14]; M->ngon[*ptr+3] = NODES[25];
  ptr++;

  // NCELLS+5 BCK

  // nfaces+10
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[20]; M->ngon[*ptr+1] = NODES[26];
  M->ngon[*ptr+2] = NODES[25]; M->ngon[*ptr+3] = NODES[18];
  ptr++;

  // NCELLS+6 LFT

  // nfaces+11
  ptr[1] = ptr[0] + 4;
  M->ngon[*ptr  ] = NODES[26]; M->ngon[*ptr+1] = NODES[24];
  M->ngon[*ptr+2] = NODES[23]; M->ngon[*ptr+3] = NODES[25];

  // Assemble children
  ptr = &M->indPH[M->ncells];

  // ncells
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = BOT[0];       M->nface[*ptr+1] = M->nfaces;
  M->nface[*ptr+2] = LFT[0];       M->nface[*ptr+3] = M->nfaces+1;
  M->nface[*ptr+4] = FRO[1];       M->nface[*ptr+5] = M->nfaces+2;
  ptr++;

  // ncells+1
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = BOT[1];       M->nface[*ptr+1] = M->nfaces+3;
  M->nface[*ptr+2] = M->nfaces+1;  M->nface[*ptr+3] = RGT[0];
  M->nface[*ptr+4] = FRO[0];       M->nface[*ptr+5] = M->nfaces+4;
  ptr++;

  // ncells+2
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = BOT[2];       M->nface[*ptr+1] = M->nfaces+5;
  M->nface[*ptr+2] = M->nfaces+6;  M->nface[*ptr+3] = RGT[1];
  M->nface[*ptr+4] = M->nfaces+4;  M->nface[*ptr+5] = BCK[0];
  ptr++;

  // ncells+3
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = BOT[3];       M->nface[*ptr+1] = M->nfaces+7;
  M->nface[*ptr+2] = LFT[1];       M->nface[*ptr+3] = M->nfaces+6;
  M->nface[*ptr+4] = M->nfaces+2;  M->nface[*ptr+5] = BCK[1];
  ptr++;

  /*********/

  // ncells+4
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = M->nfaces;    M->nface[*ptr+1] = TOP[0];
  M->nface[*ptr+2] = LFT[3];       M->nface[*ptr+3] = M->nfaces+8;
  M->nface[*ptr+4] = FRO[2];       M->nface[*ptr+5] = M->nfaces+9;
  ptr++;

  // ncells+5
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = M->nfaces+3;  M->nface[*ptr+1] = TOP[1];
  M->nface[*ptr+2] = M->nfaces+8;  M->nface[*ptr+3] = RGT[3];
  M->nface[*ptr+4] = FRO[3];       M->nface[*ptr+5] = M->nfaces+10;
  ptr++;

  // ncells+6
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = M->nfaces+5; M->nface[*ptr+1] = TOP[2];
  M->nface[*ptr+2] = M->nfaces+11; M->nface[*ptr+3] = RGT[2];
  M->nface[*ptr+4] = M->nfaces+10; M->nface[*ptr+5] = BCK[3];
  ptr++;

  // ncells+7
  ptr[1] = ptr[0] + 6;
  M->nface[*ptr  ] = M->nfaces+7;  M->nface[*ptr+1] = TOP[3];
  M->nface[*ptr+2] = LFT[2];  M->nface[*ptr+3] = M->nfaces+11;
  M->nface[*ptr+4] = M->nfaces+9; M->nface[*ptr+5] = BCK[2];

  // Set new cells in tree
  E_Int next_level = M->cellTree[cell]->level + 1;
  set_parent_elem(M->cellTree, cell, 8, M->ncells);
  for (E_Int i = 0; i < 8; i++)
    set_child_elem(M->cellTree, cell, i, HEXA, next_level, M->ncells);

  // Set external faces owns and neis
  update_external_own_nei(cell, M);

  // Set owns and neis of internal faces
  M->owner[M->nfaces]    = M->ncells;   M->owner[M->nfaces+1]  = M->ncells;
  M->neigh[M->nfaces]    = M->ncells+4; M->neigh[M->nfaces+1]  = M->ncells+1;

  M->owner[M->nfaces+2]  = M->ncells;   M->owner[M->nfaces+3]  = M->ncells+1;
  M->neigh[M->nfaces+2]  = M->ncells+3; M->neigh[M->nfaces+3]  = M->ncells+5;

  M->owner[M->nfaces+4]  = M->ncells+1; M->owner[M->nfaces+5]  = M->ncells+2;
  M->neigh[M->nfaces+4]  = M->ncells+2; M->neigh[M->nfaces+5]  = M->ncells+6;

  M->owner[M->nfaces+6]  = M->ncells+3; M->owner[M->nfaces+7]  = M->ncells+3;
  M->neigh[M->nfaces+6]  = M->ncells+2; M->neigh[M->nfaces+7]  = M->ncells+7;

  M->owner[M->nfaces+8]  = M->ncells+4; M->owner[M->nfaces+9]  = M->ncells+4;
  M->neigh[M->nfaces+8]  = M->ncells+5; M->neigh[M->nfaces+9]  = M->ncells+7;

  M->owner[M->nfaces+10] = M->ncells+5; M->owner[M->nfaces+11] = M->ncells+7;
  M->neigh[M->nfaces+10] = M->ncells+6; M->neigh[M->nfaces+11] = M->ncells+6;

  for (E_Int i = 0; i < 8; i++)
    check_canon_hexa(M->ncells+i, M);

  M->ncells += 8;
  M->nfaces += 12;
}

static
void refine_penta(E_Int cell, AMesh *M)
{}

static
void refine_pyra(E_Int cell, AMesh *M)
{}

void refine_faces(const std::vector<E_Int> &ref_faces, AMesh *M)
{
  for (size_t i = 0; i < ref_faces.size(); i++) {
    E_Int face = ref_faces[i];
    E_Int type = M->faceTree[face]->type;
    if      (type == TRI)  refine_tri(face, M);
    else if (type == QUAD) refine_quad(face, M);
    else assert(0);
  }
}

void refine_cells(const std::vector<E_Int> &ref_cells, AMesh *M)
{
  for (size_t i = 0; i < ref_cells.size(); i++) {
    E_Int cell = ref_cells[i];
    E_Int type = M->cellTree[cell]->type;
    switch (type) {
      case HEXA:
        refine_hexa(cell, M);
        break;
      case TETRA:
        refine_tetra(cell, M);
        break;
      case PENTA:
        refine_penta(cell, M);
        break;
      case PYRA:
        refine_pyra(cell, M);
        break;
      default:
        assert(0);
        break;
    }
  }
}

void resize_data_for_refinement(AMesh *M, E_Int nref_cells, E_Int nref_faces)
{
  E_Int cell_incr = nref_cells * 8;
  E_Int face_incr = nref_faces * 4 // OK for quads and tris
                  + cell_incr * 12; // max 12 internal faces per refined cell
  E_Int nnew_cells = M->ncells + cell_incr;
  E_Int nnew_faces = M->nfaces + face_incr;
  // max 5 new points per refined quad + nref_cells centroids
  E_Int nnew_points = M->npoints + nref_faces*5 + nref_cells;
  
  M->cellTree = (Element **)XRESIZE(M->cellTree, nnew_cells*sizeof(Element *));
  M->faceTree = (Element **)XRESIZE(M->faceTree, nnew_faces*sizeof(Element *));
  for (E_Int i = M->ncells; i < nnew_cells; i++)
    M->cellTree[i] = (Element *)XCALLOC(1, sizeof(Element));
  for (E_Int i = M->nfaces; i < nnew_faces; i++)
    M->faceTree[i] = (Element *)XCALLOC(1, sizeof(Element));
  
  M->ngon  = (E_Int *)  XRESIZE(M->ngon,  (4*nnew_faces) * sizeof(E_Int));
  M->indPG = (E_Int *)  XRESIZE(M->indPG, (nnew_faces+1) * sizeof(E_Int));
  M->nface = (E_Int *)  XRESIZE(M->nface, (6*nnew_cells) * sizeof(E_Int));
  M->indPH = (E_Int *)  XRESIZE(M->indPH, (nnew_cells+1) * sizeof(E_Int));
  M->owner = (E_Int *)  XRESIZE(M->owner, (nnew_faces)   * sizeof(E_Int));
  M->neigh = (E_Int *)  XRESIZE(M->neigh, (nnew_faces)   * sizeof(E_Int));
  M->x     = (E_Float *)XRESIZE(M->x,     (nnew_points)  * sizeof(E_Float));
  M->y     = (E_Float *)XRESIZE(M->y,     (nnew_points)  * sizeof(E_Float));
  M->z     = (E_Float *)XRESIZE(M->z,     (nnew_points)  * sizeof(E_Float));  
  //M->fc    = (E_Float *)XRESIZE(M->fc,    (3*nnew_faces) * sizeof(E_Float));
  //M->cx    = (E_Float *)XRESIZE(M->cx,    (nnew_cells)   * sizeof(E_Float));
  //M->cy    = (E_Float *)XRESIZE(M->cy,    (nnew_cells)   * sizeof(E_Float));
  //M->cz    = (E_Float *)XRESIZE(M->cz,    (nnew_cells)   * sizeof(E_Float));

  for (E_Int i = M->nfaces; i < nnew_faces; i++) {
    M->owner[i] = -1;
    M->neigh[i] = -1;
  }
}
