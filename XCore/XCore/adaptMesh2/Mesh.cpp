#include "Proto.h"
#include <cassert>

E_Int master_face(E_Int face, AMesh *M)
{
  return M->faceTree->children(face) ? face : M->faceTree->parent(face);
}

E_Int master_cell(E_Int cell, AMesh *M)
{
  if (cell == -1) return -1;
  return M->cellTree->children(cell) ? cell : M->cellTree->parent(cell);
}

const char *cell_type_to_string(E_Int type)
{
  switch (type) {
    case HEXA: return "HEXA";
    case TETRA: return "TETRA";
    case PENTA: return "PENTA";
    case PYRA: return "PYRA";
    default:
      return "UNKNOWN";
  }
}

const char *face_type_to_string(E_Int type)
{
  switch (type) {
    case HEXA: return "QUAD";
    case TRI: return "TRI";
    default:
      return "UNKNOWN";
  }
}

void ngon_print(AMesh *M)
{
  puts("");
  for (E_Int i = 0; i < M->nfaces; i++) {
    for (E_Int j = M->indPG[i]; j < M->indPG[i+1]; j++)
      printf("%d ", M->ngon[j]);
    puts("");
  }
  puts("");
}

void nface_print(AMesh *M)
{
  puts("");
  for (E_Int i = 0; i < M->ncells; i++) {
    for (E_Int j = M->indPH[i]; j < M->indPH[i+1]; j++)
      printf("%d ", M->nface[j]);
    puts("");
  }
  puts("");
}

const E_Int normalIn_T[4] = {1, 0, 1, 0};
const E_Int normalIn_Pe[5] = {1, 0, 1, 0, 1};
const E_Int normalIn_Py[5] = {1, 1, 0, 1, 0};
const E_Int normalIn_H[6] = {1, 0, 1, 0, 1, 0};

Edge::Edge()
{}

Edge::Edge(E_Int p0, E_Int p1) :
  p0_(std::min(p0, p1)), p1_(std::max(p0, p1))
{}

void Edge::set(E_Int p0, E_Int p1)
{
  p0_ = std::min(p0, p1);
  p1_ = std::max(p0, p1);
}

bool Edge::operator<(const Edge &e) const
{
  return (p0_ < e.p0_) || (p0_ == e.p0_ && p1_ < e.p1_);
}

void patch_drop(Patch *P)
{
  XFREE(P->pf);
  XFREE(P->pn);
  XFREE(P->sbuf_i);
  XFREE(P->rbuf_i);
  XFREE(P->sbuf_f);
  XFREE(P->rbuf_f);
}

AMesh::AMesh() :
  ncells(-1), nfaces(-1), npoints(-1), nif(-1), nbf(-1), npf(-1),
  x(NULL), y(NULL), z(NULL),
  nface(NULL), indPH(NULL), ngon(NULL), indPG(NULL),
  owner(NULL), neigh(NULL),
  nbc(-1), ptlists(NULL), bcsizes(NULL), bcnames(NULL),
  Tr(-1.0), Tu(-1.0), eps(-1.0), hmin(-1.0), hmax(-1.0), unrefine(-1),
  mode_2D(NULL), ref_data(NULL), ecenter(NULL),
  cellTree(NULL), faceTree(NULL),
  prev_ncells(-1), prev_nfaces(-1), prev_npoints(-1),
  pid(-1), npc(-1), nrq(-1), req(NULL),
  gcells(NULL), gfaces(NULL), gpoints(NULL),
  npatches(-1), patches(NULL),
  PT(NULL), FT(NULL), CT(NULL),
  px(NULL), py(NULL), pz(NULL), pfld(NULL), pref(NULL), plvl(NULL),
  closed_indPG(NULL), closed_ngon(NULL),
  closed_indPH(NULL), closed_nface(NULL)
{
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);
  MPI_Comm_size(MPI_COMM_WORLD, &npc);
  nrq = 0;
  req = (MPI_Request *)XMALLOC(2*npc * sizeof(MPI_Request));
}

void mesh_drop(AMesh *M)
{
  XFREE(M->x);
  XFREE(M->y);
  XFREE(M->z);

  XFREE(M->nface);
  XFREE(M->indPH);
  XFREE(M->ngon);
  XFREE(M->indPG);

  XFREE(M->owner);
  XFREE(M->neigh);

  for (E_Int i = 0; i < M->nbc; i++) {
    XFREE(M->ptlists[i]);
    XFREE(M->bcnames[i]);
  }
  XFREE(M->ptlists);
  XFREE(M->bcnames);
  XFREE(M->bcsizes);

  XFREE(M->mode_2D);
  XFREE(M->ref_data);

  delete M->ecenter;
  
  M->cellTree->drop();
  M->faceTree->drop();
  delete M->cellTree;
  delete M->faceTree;

  XFREE(M->req);
  XFREE(M->gcells);
  XFREE(M->gfaces);
  XFREE(M->gpoints);

  for (E_Int i = 0; i < M->npatches; i++)
    patch_drop(&M->patches[i]);
  XFREE(M->patches);

  delete M->PT;
  delete M->FT;
  delete M->CT;
  
  XFREE(M->closed_indPG);
  XFREE(M->closed_ngon);
  XFREE(M->closed_indPH);
  XFREE(M->closed_nface);

  delete M;
}

E_Int get_neighbour(E_Int cell, E_Int face, AMesh *M)
{
  assert(cell == M->owner[face] || cell == M->neigh[face]);
  if (cell == M->owner[face]) return M->neigh[face];
  return M->owner[face];
}

E_Int *get_face(E_Int i, E_Int &np, E_Int *ngon, E_Int *indPG)
{
  np = indPG[i+1] - indPG[i];
  return &ngon[indPG[i]];
}

E_Int *get_cell(E_Int i, E_Int &nf, E_Int *nface, E_Int *indPH)
{
  nf = indPH[i+1] - indPH[i];
  return &nface[indPH[i]];
}

E_Int get_stride(E_Int i, E_Int *indir)
{
  return indir[i+1] - indir[i];
}

E_Int *get_facets(E_Int i, E_Int *cn, E_Int *indir)
{
  return &cn[indir[i]];
}

void compute_face_center_and_area(E_Int id, E_Int stride,
  E_Int *pn, E_Float *x, E_Float *y, E_Float *z, E_Float *fc, E_Float *fa)
{
  // Init
  fa[0] = fa[1] = fa[2] = 0.0;
  fc[0] = fc[1] = fc[2] = 0.0;

  // Approximate face center
  E_Float fcenter[3] = {0,0,0};

  for (E_Int i = 0; i < stride; i++) {
    E_Int point = pn[i];
    fcenter[0] += x[point];
    fcenter[1] += y[point];
    fcenter[2] += z[point];
  }

  for (E_Int i = 0; i < 3; i++) fcenter[i] /= stride;

  // Sum of triangle area vectors
  E_Float sumN[3] = {0,0,0};
  // Sum of triangle areas
  E_Float sumA = 0;
  // Sum of area-weighted triangle centers
  E_Float sumAc[3] = {0,0,0};

  // Compute area vector and center of stride-2 triangles
  // formed by p0p1p2, p0p2p3, ... p0p(stride-2)p(stride-1)
  E_Int p0 = pn[0];
  for (E_Int i = 1; i < stride-1; i++) {
    E_Int p1 = pn[i];
    E_Int p2 = pn[i+1];

    // Triangle center
    E_Float tc[3];
    tc[0] = x[p0] + x[p1] + x[p2];
    tc[1] = y[p0] + y[p1] + y[p2];
    tc[2] = z[p0] + z[p1] + z[p2];

    // Area vector
    E_Float n[3];
    E_Float v10[3] = {x[p1]-x[p0], y[p1]-y[p0], z[p1]-z[p0]};
    E_Float v20[3] = {x[p2]-x[p0], y[p2]-y[p0], z[p2]-z[p0]};
    K_MATH::cross(v10, v20, n);

    // Area
    E_Float a = K_MATH::norm(n, 3);

    for (E_Int j = 0; j < 3; j++) {
      sumN[j] += n[j];
      sumAc[j] += a*tc[j];
    }
    sumA += a;
  }

  // Deal with zero-area faces
  if (sumA < K_MATH::SMALL) {
    fprintf(stderr, "compute_face_area_and_center(): "
      "Warning: Face: %d - Area: %f - Tol: %.2e\n", id, sumA, K_MATH::SMALL);
    for (E_Int i = 0; i < 3; i++) {
      fc[i] = fcenter[i];
      fa[i] = 0.0;
    }
  } else {
    for (E_Int i = 0; i < 3; i++) {
      fc[i] = sumAc[i]/(3.0*sumA);
      fa[i] = 0.5*sumN[i];
    }
  }
}

E_Int get_reorient(E_Int face, E_Int cell, E_Int normalIn, AMesh *M)
{
  assert(M->owner[face] == cell || M->neigh[face] == cell);
  if (M->neigh[face] == cell && normalIn == 1) return 0;
  else if (M->owner[face] == cell && normalIn == 0) return 0;
  else return 1;
}

static
void reorder_tetra(E_Int i, E_Int nf, E_Int *pf, AMesh *M)
{
  E_Int common[3], map[3];
  E_Int bot = pf[0];
  E_Int *pn = get_facets(bot, M->ngon, M->indPG);

  for (E_Int j = 0; j < 3; j++) map[j] = pn[j];
  E_Int reorient = get_reorient(bot, i, normalIn_T[0], M);
  if (reorient) std::swap(map[1], map[2]);

  E_Int lft, rgt, fro;
  lft = rgt = fro = -1;

  for (E_Int j = 1; j < 4; j++) {
    E_Int face = pf[j];

    for (E_Int k = 0; k < 3; k++) common[k] = 0;

    E_Int *pnn = get_facets(face, M->ngon, M->indPG);

    for (E_Int k = 0; k < 3; k++) {
      E_Int point = pnn[k];

      for (E_Int l = 0; l < 3; l++) {
        if (map[l] == point) {
          common[l] = 1;
          break;
        }
      }
    }

    if      (common[0] && common[2]) lft = face;
    else if (common[1] && common[2]) rgt = face;
    else if (common[1] && common[0]) fro = face;
    else assert(0);
  }
  assert(lft != -1 && rgt != -1 && fro != -1);

  pf[1] = lft;
  pf[2] = rgt;
  pf[3] = fro;
}

static
void reorder_penta(E_Int i, E_Int nf, E_Int *pf, AMesh *M)
{
  // First tri is bottom
  E_Int bot = -1;
  for (E_Int i = 0; i < nf; i++) {
    if (get_stride(pf[i], M->indPG) == 3) {
      bot = pf[i];
      Right_shift(pf, i, 5);
      break;
    }
  }
  assert(bot != -1);

  E_Int common[3], map[3];
  E_Int *pn = get_facets(bot, M->ngon, M->indPG);
  
  for (E_Int i = 0; i < 3; i++) map[i] = pn[i];
  E_Int reorient = get_reorient(bot, i, normalIn_Pe[0], M);
  if (reorient) std::swap(map[1], map[2]);

  E_Int top, lft, rgt, bck;
  top = lft = rgt = bck = -1;

  for (E_Int j = 1; j < 5; j++) {
    for (E_Int k = 0; k < 3; k++) common[k] = 0;

    E_Int face = pf[j];
    E_Int stride = get_stride(face, M->indPG);
    E_Int *pn = get_facets(face, M->ngon, M->indPG);

    for (E_Int k = 0; k < stride; k++) {
      E_Int point = pn[k];

      for (E_Int l = 0; l < 3; l++) {
        if (map[l] == point) {
          common[l] = 1;
          break;
        }
      }
    }

    if      (common[0] && common[2]) lft = face;
    else if (common[0] && common[1]) rgt = face;
    else if (common[1] && common[2]) bck = face;
    else                             top = face;
  }
  assert(top != -1);
  assert(lft != -1);
  assert(rgt != -1);
  assert(bck != -1);

  pf[1] = top;
  pf[2] = lft;
  pf[3] = rgt;
  pf[4] = bck;
}

static
void reorder_pyra(E_Int i, E_Int nf, E_Int *pf, AMesh *M)
{
  E_Int common[4], map[4];
  E_Int bot = pf[0];
  E_Int *pn = get_facets(bot, M->ngon, M->indPG);
  
  for (E_Int i = 0; i < 4; i++) map[i] = pn[i];
  E_Int reorient = get_reorient(bot, i, normalIn_Py[0], M);
  if (reorient) std::swap(map[1], map[3]);

  E_Int lft, rgt, fro, bck;
  lft = rgt = fro = bck = -1;

  for (E_Int j = 1; j < 5; j++) {
    for (E_Int k = 0; k < 4; k++) common[k] = 0;

    E_Int face = pf[j];
    E_Int *pn = get_facets(face, M->ngon, M->indPG);

    for (E_Int k = 0; k < 3; k++) {
      E_Int point = pn[k];

      for (E_Int l = 0; l < 4; l++) {
        if (map[l] == point) {
          common[l] = 1;
          break;
        }
      }
    }

    if      (common[0] && common[3]) lft = face;
    else if (common[1] && common[2]) rgt = face;
    else if (common[1] && common[0]) fro = face;
    else                             bck = face;
  }

  assert(lft != -1);
  assert(rgt != -1);
  assert(fro != -1);
  assert(bck != -1);

  pf[1] = lft;
  pf[2] = rgt;
  pf[3] = fro;
  pf[4] = bck;
}

static
void reorder_hexa(E_Int i, E_Int nf, E_Int *pf, AMesh *M)
{
  E_Int common[4], map[4];
  E_Int bot = pf[0];
  E_Int *pn = get_facets(bot, M->ngon, M->indPG);
  
  for (E_Int i = 0; i < 4; i++) map[i] = pn[i];
  E_Int reorient = get_reorient(bot, i, normalIn_H[0], M);
  if (reorient) std::swap(map[1], map[3]);

  E_Int top, lft, rgt, fro, bck;
  top = lft = rgt = fro = bck = -1;

  for (E_Int j = 1; j < 6; j++) {
    E_Int face = pf[j];

    for (E_Int k = 0; k < 4; k++) common[k] = 0;

    E_Int *pn = get_facets(face, M->ngon, M->indPG);

    for (E_Int k = 0; k < 4; k++) {
      E_Int point = pn[k];

      for (E_Int l = 0; l < 4; l++) {
        if (map[l] == point) {
          common[l] = 1;
          break;
        }
      }
    }

    if      (common[0] && common[3]) lft = face;
    else if (common[0] && common[1]) fro = face;
    else if (common[1] && common[2]) rgt = face;
    else if (common[3] && common[2]) bck = face;
    else                             top = face;
  }
  assert(top != -1);
  assert(lft != -1);
  assert(rgt != -1);
  assert(fro != -1);
  assert(bck != -1);

  pf[1] = top;
  pf[2] = lft;
  pf[3] = rgt;
  pf[4] = fro;
  pf[5] = bck;
}

void reorder_cells(AMesh *M)
{
  if (M->mode_2D)
    set_cells_for_2D(M);

  for (E_Int i = 0; i < M->ncells; i++) {
    E_Int nf = -1;
    E_Int *pf = get_cell(i, nf, M->nface, M->indPH);
    E_Int ctype = M->cellTree->type_[i];

    switch (ctype) {
      case TETRA:
        reorder_tetra(i, nf, pf, M);
        break;
      case PENTA:
        reorder_penta(i, nf, pf, M);
        break;
      case PYRA:
        reorder_pyra(i, nf, pf, M);
        break;
      case HEXA:
        reorder_hexa(i, nf, pf, M);
        break;
      default:
        assert(0);
        break;
    }
  }
}

E_Int check_canon_cells(AMesh *M)
{
  for (E_Int i = 0; i < M->ncells; i++) {
    E_Int type = M->cellTree->type_[i];
    switch (type) {
      case HEXA:
        assert(check_canon_hexa(i, M));
        break;
      case TETRA:
        assert(check_canon_tetra(i, M));
        break;
      case PENTA:
        assert(check_canon_penta(i, M));
        break;
      case PYRA:
        assert(check_canon_pyra(i, M));
        break;
      default:
        assert(0);
        break;
    }
  }

  return 1;
}

void Order_tri(E_Int *local, E_Int *pn, E_Int reorient, E_Int i0)
{
  for (E_Int i = 0; i < 3; i++) local[i] = pn[i];
  Right_shift(local, i0, 3);
  if (reorient) std::swap(local[1], local[2]);
}

void Order_quad(E_Int *local, E_Int *pn, E_Int reorient, E_Int i0)
{
  for (E_Int i = 0; i < 4; i++) local[i] = pn[i];
  Right_shift(local, i0, 4);
  if (reorient) std::swap(local[1], local[3]);
}

E_Int check_canon_tetra(E_Int cell, AMesh *M)
{
  E_Int NODES[4] = {-1, -1, -1, -1};

  E_Int *pf = get_facets(cell, M->nface, M->indPH);

  E_Int face, i0, reorient, *pn, local[3];

  // BOT (In)
  face = pf[0];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = 0;
  reorient = get_reorient(face, cell, normalIn_T[0], M);
  Order_tri(local, pn, reorient, i0);
  for (E_Int i = 0; i < 3; i++) NODES[i] = local[i];

  // LFT (Out)
  face = pf[1];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 3);
  reorient = get_reorient(face, cell, normalIn_T[1], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[0]);
  assert(local[2] == NODES[2]);
  NODES[3] = local[1];

  // RGT (In)
  face = pf[2];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 3);
  reorient = get_reorient(face, cell, normalIn_T[2], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[3]);
  assert(local[2] == NODES[2]);

  // FRO (Out)
  face = pf[3];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 3);
  reorient = get_reorient(face, cell, normalIn_T[3], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[1]);
  assert(local[2] == NODES[3]);

  return 1;
}

E_Int check_canon_hexa(E_Int cell, AMesh *M)
{
  E_Int NODES[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

  E_Int *pf = get_facets(cell, M->nface, M->indPH);

  E_Int face, i0, reorient, *pn, local[4];

  // BOT
  face = pf[0];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = 0;
  reorient = get_reorient(face, cell, normalIn_H[0], M);
  Order_quad(local, pn, reorient, i0);
  for (E_Int i = 0; i < 4; i++) NODES[i] = local[i];

  // LFT
  face = pf[2];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[2], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[3]);
  NODES[7] = local[2];
  NODES[4] = local[3];

  // RGT
  face = pf[3];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[3], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[2]);
  NODES[6] = local[2];
  NODES[5] = local[3];

  // FRO
  face = pf[4];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[4], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[0]);
  assert(local[2] == NODES[4]);
  assert(local[3] == NODES[5]);

  // BCK
  face = pf[5];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[2], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[5], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[2]);
  assert(local[1] == NODES[3]);
  assert(local[2] == NODES[7]);
  assert(local[3] == NODES[6]);

  // TOP
  face = pf[1];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[4], pn, 4);
  reorient = get_reorient(face, cell, normalIn_H[1], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[4]);
  assert(local[1] == NODES[5]);
  assert(local[2] == NODES[6]);
  assert(local[3] == NODES[7]);

  return 1;
}

E_Int check_canon_penta(E_Int cell, AMesh *M)
{
  E_Int NODES[6] = {-1, -1, -1, -1, -1, -1};

  E_Int *pf = get_facets(cell, M->nface, M->indPH);

  E_Int face, i0, reorient, *pn, local[4];

  // BOT
  face = pf[0];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = 0;
  reorient = get_reorient(face, cell, normalIn_Pe[0], M);
  Order_tri(local, pn, reorient, i0);
  for (E_Int i = 0; i < 3; i++) NODES[i] = local[i];

  // LFT (in)
  face = pf[2];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 4);
  assert(i0 != -1);
  reorient = get_reorient(face, cell, normalIn_Pe[2], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[2]);
  NODES[5] = local[2];
  NODES[3] = local[3];

  // RGT (out)
  face = pf[3];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 4);
  reorient = get_reorient(face, cell, normalIn_Pe[3], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[1]);
  assert(local[3] == NODES[3]);
  NODES[4] = local[2];

  // BCK (in)
  face = pf[4];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[2], pn, 4);
  reorient = get_reorient(face, cell, normalIn_Pe[4], M);
  Order_quad(local, pn, reorient, i0);
  assert(local[0] == NODES[2]);
  assert(local[1] == NODES[1]);
  assert(local[2] == NODES[4]);
  assert(local[3] == NODES[5]);

  // TOP
  face = pf[1];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[3], pn, 3);
  reorient = get_reorient(face, cell, normalIn_Pe[1], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[3]);
  assert(local[1] == NODES[4]);
  assert(local[2] == NODES[5]);

  return 1;
}

E_Int check_canon_pyra(E_Int cell, AMesh *M)
{
  E_Int NODES[5] = {-1, -1, -1, -1, -1};

  E_Int *pf = get_facets(cell, M->nface, M->indPH);

  E_Int face, i0, reorient, *pn, local[4];

  // BOT (in)
  face = pf[0];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = 0;
  reorient = get_reorient(face, cell, normalIn_Py[0], M);
  Order_quad(local, pn, reorient, i0);
  for (E_Int i = 0; i < 4; i++) NODES[i] = local[i];

  // LFT (in)
  face = pf[1];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[0], pn, 3);
  reorient = get_reorient(face, cell, normalIn_Py[1], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[0]);
  assert(local[1] == NODES[3]);
  NODES[4] = local[2];

  // RGT (out)
  face = pf[2];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 3);
  reorient = get_reorient(face, cell, normalIn_Py[2], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[2]);
  assert(local[2] == NODES[4]);

  // FRO (in)
  face = pf[3];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[1], pn, 3);
  reorient = get_reorient(face, cell, normalIn_Py[3], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[1]);
  assert(local[1] == NODES[0]);
  assert(local[2] == NODES[4]);

  // BCK (out)
  face = pf[4];
  pn = get_facets(face, M->ngon, M->indPG);
  i0 = Get_pos(NODES[2], pn, 3);
  reorient = get_reorient(face, cell, normalIn_Py[4], M);
  Order_tri(local, pn, reorient, i0);
  assert(local[0] == NODES[2]);
  assert(local[1] == NODES[3]);
  assert(local[2] == NODES[4]);

  return 1;
}

E_Int is_internal_face(E_Int face, AMesh *M)
{
  return M->neigh[face] != -1;
}

AMesh *init_mesh(K_FLD::FldArrayI &cn, E_Float *px, E_Float *py,
  E_Float *pz, E_Int npts)
{
  E_Int *ngon = cn.getNGon();
  E_Int *indPG = cn.getIndPG();
  E_Int *nface = cn.getNFace();
  E_Int *indPH = cn.getIndPH();
  E_Int ncells = cn.getNElts();
  E_Int nfaces = cn.getNFaces();

  AMesh *M = new AMesh;

  M->npoints = npts;
  M->nfaces = nfaces;
  M->ncells = ncells;

  M->x = (E_Float *)XMALLOC(M->npoints * sizeof(E_Float));
  M->y = (E_Float *)XMALLOC(M->npoints * sizeof(E_Float));
  M->z = (E_Float *)XMALLOC(M->npoints * sizeof(E_Float));

  memcpy(M->x, px, M->npoints * sizeof(E_Float));
  memcpy(M->y, py, M->npoints * sizeof(E_Float));
  memcpy(M->z, pz, M->npoints * sizeof(E_Float));

  M->indPH = (E_Int *)XMALLOC((M->ncells+1) * sizeof(E_Int));
  M->indPG = (E_Int *)XMALLOC((M->nfaces+1) * sizeof(E_Int));

  memcpy(M->indPH, indPH, (M->ncells+1) * sizeof(E_Int));
  memcpy(M->indPG, indPG, (M->nfaces+1) * sizeof(E_Int));

  M->nface = (E_Int *)XMALLOC(M->indPH[M->ncells] * sizeof(E_Int));
  M->ngon  = (E_Int *)XMALLOC(M->indPG[M->nfaces] * sizeof(E_Int));

  E_Int *ptr = M->nface;

  for (E_Int i = 0; i < M->ncells; i++) {
    E_Int nf = -1;
    E_Int *pf = cn.getElt(i, nf, nface, indPH);
    for (E_Int j = 0; j < nf; j++)
      *ptr++ = pf[j]-1;
  }

  ptr = M->ngon;

  for (E_Int i = 0; i < M->nfaces; i++) {
    E_Int np = -1;
    E_Int *pn = cn.getFace(i, np, ngon, indPG);
    for (E_Int j = 0; j < np; j++)
      *ptr++ = pn[j]-1;
  }

  M->ecenter = new std::map<Edge, E_Int>;

  M->CT = new std::unordered_map<E_Int, E_Int>;
  M->FT = new std::unordered_map<E_Int, E_Int>;
  M->PT = new std::unordered_map<E_Int, E_Int>;
 
  return M;
}

void get_full_cell(E_Int cell, AMesh *M, E_Int &nf, E_Int *pf)
{
  E_Int stride = -1;
  E_Int *FACES = get_cell(cell, stride, M->nface, M->indPH);
  nf = 0;

  for (E_Int i = 0; i < stride; i++) {
    E_Int face = FACES[i];

    E_Int flvl = M->faceTree->level(face);
    E_Int clvl = M->cellTree->level(cell);

    assert(flvl >= clvl);

    if (flvl == clvl) {
      pf[nf++] = face;
    } else {
      assert(master_face(face, M) == face);
      Children *children = M->faceTree->children(face);
      for (E_Int j = 0; j < children->n; j++)
        pf[nf++] = children->pc[j];
    }
  }
}

// TODO(Imad): update comm patch faces

// Keep refined faces and their children, untouched faces and unrefined
// faces

void update_boundary_faces(AMesh *M)
{
  M->nbf = 0;
  for (E_Int i = 0; i < M->nbc; i++) {
    E_Int *ptlist = M->ptlists[i];
    
    // How many faces on current boundary have been refined/unrefined?
    E_Int new_bcsize = 0;
    for (E_Int j = 0; j < M->bcsizes[i]; j++) {
      E_Int face = ptlist[j];
      E_Int state = M->faceTree->state(face);

      if (state == UNTOUCHED)
        new_bcsize += 1;
      else if (state == REFINED)
        new_bcsize += M->faceTree->children(face)->n;
      else if (state == UNREFINED)
        new_bcsize += 1;
    }

    M->nbf += new_bcsize;

    E_Int *new_ptlist = (E_Int *)XMALLOC(new_bcsize * sizeof(E_Int));
    E_Int *ptr = new_ptlist;

    for (E_Int j = 0; j < M->bcsizes[i]; j++) {
      E_Int face = ptlist[j];
      E_Int state = M->faceTree->state(face);
      
      if (state == UNTOUCHED || state == UNREFINED) {
        *ptr++ = face;
      } else if (state == REFINED) {
        Children *children = M->faceTree->children(face);
        for (E_Int k = 0; k < children->n; k++) *ptr++ = children->pc[k];
      }
    }

    XFREE(M->ptlists[i]);

    M->ptlists[i] = new_ptlist;
    M->bcsizes[i] = new_bcsize;
  }

  M->nif = M->nfaces - M->nbf;
}

void update_global_cells_after_ref(AMesh *M)
{
  E_Int nnew_cells = M->ncells - M->prev_ncells;
  
  E_Int first_new_cell;

  MPI_Scan(&nnew_cells, &first_new_cell, 1, XMPI_INT, MPI_SUM, MPI_COMM_WORLD);

  E_Int gncells;
  MPI_Allreduce(&M->prev_ncells, &gncells, 1, XMPI_INT, MPI_SUM, MPI_COMM_WORLD);

  if (nnew_cells > 0) {
    M->gcells = (E_Int *)XRESIZE(M->gcells, M->ncells * sizeof(E_Int));

    for (E_Int i = 0; i < nnew_cells; i++)
      M->gcells[M->prev_ncells + i] = gncells + first_new_cell - nnew_cells + i;
  }
}

void update_global_faces_after_ref(AMesh *M)
{
  E_Int nnew_faces = M->nfaces - M->prev_nfaces;
  E_Int first_new_face;

  MPI_Scan(&nnew_faces, &first_new_face, 1, XMPI_INT, MPI_SUM, MPI_COMM_WORLD);

  E_Int gnfaces;
  MPI_Allreduce(&M->prev_nfaces, &gnfaces, 1, XMPI_INT, MPI_SUM, MPI_COMM_WORLD);

  if (nnew_faces == 0) return;

  M->gfaces = (E_Int *)XRESIZE(M->gfaces, M->nfaces * sizeof(E_Int));

  // Init
  for (E_Int i = M->prev_nfaces; i < M->nfaces; i++)
    M->gfaces[i] = -1;

  // Internal faces first
  E_Int incr = first_new_face - nnew_faces;

  for (E_Int i = 0; i < nnew_faces; i++) {
    E_Int face = M->prev_nfaces + i;
    if (M->neigh[face] == -1) continue;

    M->gfaces[face] = gnfaces + incr++;
    M->FT->insert({M->gfaces[face], face});
  }

  // Boundary faces next
  for (E_Int i = 0; i < M->nbc; i++) {
    E_Int *ptlist = M->ptlists[i];
    
    for (E_Int j = 0; j < M->bcsizes[i]; j++) {
      E_Int face = ptlist[j];

      if (M->gfaces[face] == -1) {
        assert(face >= M->prev_nfaces);
        M->gfaces[face] = gnfaces + incr++;
        M->FT->insert({M->gfaces[face], face});
      }
    }
  }

  // Patch faces last as they might be renumbered and we wouldn't want to
  // create holes in global face numbering

  for (E_Int i = 0; i < M->npatches; i++) {
    Patch *P = &M->patches[i];

    for (E_Int j = 0; j < P->nf; j++) {
      E_Int face = P->pf[j];

      if (M->gfaces[face] == -1) {
        M->gfaces[face] = gnfaces + incr++;
        M->FT->insert({M->gfaces[face], face});
      }
    }
  }
}

void update_global_points_after_ref(AMesh *M)
{
  E_Int nnew_points = M->npoints - M->prev_npoints;
  E_Int first_new_point;

  MPI_Scan(&nnew_points, &first_new_point, 1, XMPI_INT, MPI_SUM, MPI_COMM_WORLD);

  E_Int gnpoints;
  MPI_Allreduce(&M->prev_npoints, &gnpoints, 1, XMPI_INT, MPI_SUM, MPI_COMM_WORLD);

  if (nnew_points == 0) return;

  M->gpoints = (E_Int *)XRESIZE(M->gpoints, M->npoints * sizeof(E_Int));

  // Init
  for (E_Int i = M->prev_npoints; i < M->npoints; i++)
    M->gpoints[i] = -1;

  // Freeze points belonging to pfaces
  E_Int *fpts = (E_Int *)XCALLOC(M->npoints, sizeof(E_Int));
  
  for (E_Int i = 0; i < M->npatches; i++) {
    Patch *P = &M->patches[i];

    for (E_Int j = 0; j < P->nf; j++) {
      E_Int pface = P->pf[j];
      assert(pface >= 0 && pface < M->nfaces);

      E_Int np = -1;
      E_Int *pn = get_face(pface, np, M->ngon, M->indPG);    
  
      for (E_Int k = 0; k < np; k++)
        fpts[pn[k]] = 1;
    }
  }

  E_Int incr = gnpoints + first_new_point - nnew_points;

  // First pass: skip ppoints
  for (E_Int i = 0; i < nnew_points; i++) {
    E_Int point = M->prev_npoints + i;
    if (fpts[point]) continue;
    
    M->gpoints[point] = incr++;
    M->PT->insert({M->gpoints[point], point});
  }

  // Second pass: do ppoints
  for (E_Int i = 0; i < nnew_points; i++) {
    E_Int point = M->prev_npoints + i;
    if (!fpts[point]) continue;
    
    M->gpoints[point] = incr++;
    M->PT->insert({M->gpoints[point], point});
  }

  for (E_Int i = 0; i < M->npoints; i++)
    assert(M->gpoints[i] != -1);
 
  XFREE(fpts);
}

static
void resize_for_synchro(AMesh *M, E_Int nrf)
{
  // Resize
  E_Int nnew_faces = M->nfaces + nrf;
  E_Int nnew_points = M->npoints + nrf*5; // Max 5 new points per refined face

  M->faceTree->resize(nnew_faces);
  M->ngon  = (E_Int *)  XRESIZE(M->ngon,  (4*nnew_faces) *sizeof(E_Int));
  M->indPG = (E_Int *)  XRESIZE(M->indPG, (nnew_faces+1) *sizeof(E_Int));
  M->owner = (E_Int *)  XRESIZE(M->owner, (nnew_faces)   *sizeof(E_Int));
  M->neigh = (E_Int *)  XRESIZE(M->neigh, (nnew_faces)   *sizeof(E_Int));
  M->x     = (E_Float *)XRESIZE(M->x,     (nnew_points)  *sizeof(E_Float));
  M->y     = (E_Float *)XRESIZE(M->y,     (nnew_points)  *sizeof(E_Float));
  M->z     = (E_Float *)XRESIZE(M->z,     (nnew_points)  *sizeof(E_Float));

  for (E_Int i = M->nfaces; i < nnew_faces; i++) {
    M->owner[i] = -1;
    M->neigh[i] = -1;
  }
}

void resize_data_for_synchronisation(AMesh *M)
{
  E_Int nrf = 0; // Total of remotely refined patch children

  int *rcount = (int *)XMALLOC(M->npatches * sizeof(int));

  for (E_Int i = 0; i < M->npatches; i++) {
    Patch *P = &M->patches[i];

    int scount = 0;

    for (E_Int j = 0; j < P->nf; j++) {
      E_Int face = P->pf[j];
      E_Int state = M->faceTree->state(face);

      if (state == REFINED) {
        scount += M->faceTree->children(face)->n - 1; // do not count face
      }
    }

    MPI_Irecv(rcount+i, 1, MPI_INT, P->nei, 0, MPI_COMM_WORLD, &M->req[M->nrq++]);
    MPI_Isend(&scount, 1, MPI_INT, P->nei, 0, MPI_COMM_WORLD, &M->req[M->nrq++]);
  }

  Comm_waitall(M);

  for (E_Int i = 0; i < M->npatches; i++)
    nrf += rcount[i];

  resize_for_synchro(M, nrf);

  XFREE(rcount);
}

void update_patch_faces_after_ref(AMesh *M)
{
  for (E_Int i = 0; i < M->npatches; i++) {
    Patch *P = &M->patches[i];
    E_Int new_nf = 0;
    
    for (E_Int j = 0; j < P->nf; j++) {
      E_Int face = P->pf[j];
      E_Int state = M->faceTree->state(face);

      if (state == UNTOUCHED)
        new_nf += 1;
      else if (state == REFINED)
        new_nf += M->faceTree->children(face)->n;
      else if (state == UNREFINED)
        new_nf += 1;
    }

    E_Int *new_pf = (E_Int *)XMALLOC(new_nf * sizeof(E_Int));
    E_Int *new_pn = (E_Int *)XMALLOC(new_nf * sizeof(E_Int));
    E_Int *pf = new_pf;
    E_Int *pn = new_pn;

    // Children faces inherit pn of parent, will be updated later

    for (E_Int j = 0; j < P->nf; j++) {
      E_Int face = P->pf[j];
      E_Int nei = P->pn[j];
      E_Int state = M->faceTree->state(face);

      if (state == UNTOUCHED || state == UNREFINED) {
        *pf++ = face;
        *pn++ = nei;
      } else if (state == REFINED) {
        Children *children = M->faceTree->children(face);
        
        for (E_Int k = 0; k < children->n; k++) {
          *pf++ = children->pc[k];
          *pn++ = nei;
        }
      }
    }

    XFREE(M->patches[i].pf);
    XFREE(M->patches[i].pn);
    M->patches[i].pf = new_pf;
    M->patches[i].pn = new_pn;
    M->patches[i].nf = new_nf;
  }

  if (M->mode_2D) return;

  // Swap first and third children if iso refinement
  for (E_Int i = 0; i < M->npatches; i++) {
    Patch *P = &M->patches[i];

    if (M->pid < P->nei) continue;

    for (E_Int j = 0; j < P->nf; ) {
      E_Int face = P->pf[j++];
      E_Int state = M->faceTree->state(face);
      if (state != REFINED) continue;

      assert(M->faceTree->children(face)->n == 4);

      std::swap(P->pf[j], P->pf[j+2]);
      j += 3;
    }
  }
}

void synchronise_patches_after_ref(AMesh *M)
{
  // Goal: eliminate duplicate global patch face numbering
  // Choice : lesser rank numbering wins

  // Procs exchange their latest patch state
  // Both procs replace refined faces with their global children number

  for (E_Int i = 0; i < M->npatches; i++) {
    Patch *P = &M->patches[i];

    int scount = 0;

    for (E_Int j = 0; j < P->nf;) {
      E_Int face = P->pf[j++];
      E_Int state = M->faceTree->state(face);

      if (state == REFINED) {
        // face | nc | c0 | n0 ...
        E_Int np = M->indPG[face+1] - M->indPG[face];
        E_Int nc = M->faceTree->children(face)->n;
        
        scount += 2; // face + nc

        scount += nc * (np + 1); // (id + np) per children
        
        j += M->faceTree->children(face)->n-1; // skip children
      } else {
        scount += 2;
      }
    }

    int rcount;

    MPI_Send(&scount, 1, MPI_INT, P->nei, 0, MPI_COMM_WORLD);
    MPI_Recv(&rcount, 1, MPI_INT, P->nei, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    M->patches[i].sbuf_i = (E_Int *)
      XRESIZE(M->patches[i].sbuf_i, scount*sizeof(E_Int));
    memset(M->patches[i].sbuf_i, -1, scount*sizeof(E_Int));

    E_Int *f_it = M->patches[i].sbuf_i; // face iterator

    // f0 | 2 | c0 | pn ... | c1 ... | f1 | 0 | f2 ...

    for (E_Int j = 0; j < P->nf;) {
      E_Int face = P->pf[j++];
      E_Int state = M->faceTree->state(face);

      *f_it++ = M->gfaces[face];

      if (state == REFINED) {
        Children *children = M->faceTree->children(face);

        *f_it++ = children->n;

        for (E_Int k = 0; k < children->n; k++) {
          *f_it++ = M->gfaces[children->pc[k]];

          E_Int np = -1;
          E_Int *pn = get_face(children->pc[k], np, M->ngon, M->indPG);

          for (E_Int l = 0; l < np; l++)
            *f_it++ = M->gpoints[pn[l]];

        }

        j += children->n-1;
      } else {
        *f_it++ = 0;
      }
    }

    assert(f_it - M->patches[i].sbuf_i == scount);
    
    M->patches[i].rbuf_i = (E_Int *)
      XRESIZE(M->patches[i].rbuf_i, rcount*sizeof(E_Int));
    
    MPI_Send(M->patches[i].sbuf_i, scount, XMPI_INT, P->nei, 0, MPI_COMM_WORLD);
    MPI_Recv(M->patches[i].rbuf_i, rcount, XMPI_INT, P->nei, 0, MPI_COMM_WORLD,
      MPI_STATUS_IGNORE);

    
    // Deduce synchronized patch size
    // Iterate through received and current states simultaneously

    E_Int *r_it = M->patches[i].rbuf_i;
    f_it = M->patches[i].sbuf_i;

    // Count the synchronised patch total number of faces
    E_Int N = 0;
    
    // f0 | 2 | c0 | pn ... | own | c1 ... | f1 | 0 | f2 ...
    
    for (;;) {

      if (f_it - M->patches[i].sbuf_i == scount) break;

      E_Int glf = *f_it++;
      E_Int grf = *r_it++;

      assert(glf == grf); // This face is not new: its gid should be the same

      E_Int nl = *f_it++;
      E_Int nr = *r_it++;

      E_Int lf = (*(M->FT))[glf];
      E_Int np = get_stride(lf, M->indPG);
      assert(np == 3 || np == 4);

      if (nl == nr) {
        if (nl == 0) {
          // Face was not refined
          N += 1;
        } else {
          // Face was refined locally and remotely
          N += nl;
          f_it += (1+np)*nl;
          r_it += (1+np)*nl;
        }
      } else if (nl > nr) {
        // Only the local face was refined
        N += nl;
        f_it += (1+np)*nl;
      } else if (nr > nl) {
        // Only the remote face was refined
        // I need the data of its children
        N += nr;
        r_it += (1+np)*nr;
      }
    }

    assert(r_it - M->patches[i].rbuf_i == rcount);
    assert(f_it - M->patches[i].sbuf_i == scount);
    
    E_Int *new_pf = (E_Int *)XMALLOC(N * sizeof(E_Int));

    // Fill
    f_it = M->patches[i].sbuf_i;
    r_it = M->patches[i].rbuf_i;
    E_Int *p_it = new_pf;

    if (M->pid == 1) {
      printf("sbuf: ");
      for (E_Int j = 0; j < scount; j++)
        printf("%d ", M->patches[i].sbuf_i[j]);
      puts("");
      printf("rbuf: ");
      for (E_Int j = 0; j < rcount; j++)
        printf("%d ", M->patches[i].rbuf_i[j]);
      puts("");
      fflush(stdout);
    }

    for (;;) {
      if (f_it - M->patches[i].sbuf_i == scount) break;
      E_Int glf = *f_it++;
      E_Int grf = *r_it++;

      if (M->pid == 1) printf("glf : %d\n", glf);

      assert(glf == grf);

      E_Int nl = *f_it++;
      E_Int nr = *r_it++;

      if (M->pid == 1) printf("nl: %d - nr: %d\n", nl, nr);

      E_Int lf = (*(M->FT))[glf];
      E_Int np = get_stride(lf, M->indPG);
      assert(np == 3 || np == 4);

      if (nl == nr) {
        if (nl == 0) {

          if (M->pid == 1) puts("no refinement");
          
          // No refinement took place for this place
          *p_it++ = glf;
        
        } else {
          // Replace with lesser rank global numbering

          // c0 | pn ...

          if (M->pid == 1) puts("local and remote refinement");

          for (E_Int k = 0; k < nl; k++) {

            // Iterate through the ids
            E_Int glc = *f_it++;
            E_Int grc = *r_it++;

            if (M->pid == 1) printf("glc: %d - grc: %d\n", glc, grc);

            E_Int gc = M->pid < P->nei ? glc : grc;
           
            // Insert in new patch
            *p_it++ = gc;

            // Replace gface with kept label
            //auto search = M->FT->find(glc);
            //assert(search != M->FT->end());
            //E_Int lc = search->second;
            //assert(lc >= 0 && lc < M->nfaces);

            // Replace child and its points with lesser rank labels
            //if (gc != glc) {
            //  M->gfaces[lc] = gc;
            //  M->FT->erase(search);
            //  M->FT->insert({gc, lc});
            //}
            
            for (E_Int l = 0; l < np; l++) {
                E_Int glp = *f_it++;
                E_Int grp = *r_it++;
            
                if (M->pid == 1) printf("glp: %d - grp: %d\n", glp, grp);

                /*E_Int gp = M->pid < P->nei ? glp : grp;

                // Local id of current point
                auto search = M->PT->find(glp); 
                assert(search != M->PT->end());
                E_Int lp = search->second;

                M->gpoints[lp] = gp;
                M->PT->erase(search);
                M->PT->insert({gp, lp});
                */
            }    
          } // for child in children
        } // nl > 0
      } else if (nl > nr) {

        // Face was refined only locally, all data should be okay

        for (E_Int k = 0; k < nl; k++) {
          *p_it++ = *f_it++;
          f_it += np; // skip children points
        }

      } else if (nr > nl) {
        
        // c0 | pn ...
        
        // Insert this face in faceTree
        /*
        Children *new_children = (Children *)XMALLOC(sizeof(Children));
        new_children->n = nr;
    
        new_children->next = M->faceTree->children_[lf];
        M->faceTree->children_[lf] = new_children;

        M->faceTree->level_[lf] += 1;
        M->faceTree->state_[lf] = REFINED;
        */
        
        // First pass: fill new patch + face siblings

        //E_Int *t_it = r_it; // tmp iterator
        
        for (E_Int k = 0; k < nr; k++) {
          // id
          E_Int cid = *r_it++;

          *p_it++ = cid;
          
          //new_children->pc[k] = cid;

          // skip points
          r_it += np;
        }
        
        //assert(new_children->pc[0] == glf);

        // c0 | pn ... | own
        
        // Second pass: fill indPG / ngon
        
        // First child is face, treat it separately
        /*{
          E_Int cid = *r_it++;
          assert(cid == glf);

          E_Int *pn = get_facets(lf, M->ngon, M->indPG);

          for (E_Int k = 0; k < np; k++) {
            E_Int gp = *r_it++;
            
            auto search = M->PT->find(gp);

            if (search != M->PT->end()) {
              pn[k] == search->second;
              assert(pn[k] >= 0 && pn[k] < M->npoints);
            } else {
              pn[k] = M->npoints;
              M->gpoints[M->npoints] = gp;
              M->PT->insert({gp, M->npoints});
              M->npoints++;
            } 
          }
        }
        */

        // Do the rest of the children
        /*
        for (E_Int k = 1; k < nr; k++) {
          E_Int cid = *r_it++;

          // This child should new to me
          assert(M->FT->find(cid) == M->FT->end());

          // Register it in mesh
          M->gfaces[M->nfaces] = cid;
          M->FT->insert({cid, M->nfaces});

          M->indPG[M->nfaces+1] = np + M->indPG[M->nfaces];
          E_Int *pn = &M->ngon[M->indPG[M->nfaces]];

          for (E_Int l = 0; l < np; l++) {
            E_Int gp = *r_it++;

            auto search = M->PT->find(gp);

            if (search != M->PT->end()) {
              pn[l] = search->second;
            } else {

              // New point, register it

              M->gpoints[M->npoints] = gp;
              M->PT->insert({gp, M->npoints});

              pn[l] = M->npoints;
  
              M->npoints++;
            }
          }

          // Register it in face tree

          M->faceTree->state_[M->nfaces] = UNTOUCHED;
          M->faceTree->level_[M->nfaces] = M->faceTree->level(lf);
          M->faceTree->type_[M->nfaces] = M->faceTree->type(lf);
          M->faceTree->parent_[M->nfaces] = lf;
          M->nfaces++;
        }
        */
      }
    }

    assert(p_it - new_pf == N);
  
    /*
    M->patches[i].nf = N;
    XFREE(M->patches[i].pf);
    M->patches[i].pf = new_pf;

    for (E_Int j = 0; j < P->nf; j++) {
      P->pf[j] = (*(M->FT))[P->pf[j]];
    }

    // UPDATE OWNER AND NEIGH
    for (E_Int j = 0; j < P->nf; j++) {
      E_Int face = P->pf[j];
      if (M->owner[face] == -1) {
        assert(M->neigh[face] == -1);
        M->owner[face] = M->owner[M->faceTree->parent(face)];
      }
    }
    */
  }

  EXIT;
}

void renumber_mesh(AMesh *M, const std::vector<E_Int> &new_cells,
  const std::vector<E_Int> &new_faces, E_Int new_ncells, E_Int new_nfaces,
  E_Int sizeNFace, E_Int sizeNGon)
{
  // ngon
  E_Int *indPG = (E_Int *)XMALLOC((new_nfaces+1) * sizeof(E_Int));
  indPG[0] = 0;
  
  for (E_Int i = 0; i < M->nfaces; i++) {
    E_Int new_face = new_faces[i];
    if (new_face == -1) continue;

    indPG[new_face+1] = get_stride(i, M->indPG);
  }

  for (E_Int i = 0; i < new_nfaces; i++) indPG[i+1] += indPG[i];
  assert(indPG[new_nfaces] == sizeNGon);

  E_Int *ngon = (E_Int *)XMALLOC(sizeNGon * sizeof(E_Int));
  
  for (E_Int i = 0; i < M->nfaces; i++) {
    E_Int new_face = new_faces[i];
    if (new_face == -1) continue;

    E_Int np = -1;
    E_Int *pn = get_face(i, np, M->ngon, M->indPG);

    E_Int *ptr = &ngon[indPG[new_face]];

    for (E_Int j = 0; j < np; j++)
      *ptr++ = pn[j];
  }



  // nface
  E_Int *indPH = (E_Int *)XMALLOC((new_ncells+1) * sizeof(E_Int));
  indPH[0] = 0;
  
  for (E_Int i = 0; i < M->ncells; i++) {
    E_Int new_cell = new_cells[i];
    if (new_cell == -1) continue;

    indPH[new_cell+1] = get_stride(i, M->indPH);
  }

  for (E_Int i = 0; i < new_ncells; i++) indPH[i+1] += indPH[i];
  assert(indPH[new_ncells] == sizeNFace);

  E_Int *nface = (E_Int *)XMALLOC(sizeNFace * sizeof(E_Int));
  
  for (E_Int i = 0; i < M->ncells; i++) {
    E_Int new_cell = new_cells[i];
    if (new_cell == -1) {
      continue;
    }

    E_Int nf = -1;
    E_Int *pf = get_cell(i, nf, M->nface, M->indPH);

    E_Int *ptr = &nface[indPH[new_cell]];

    for (E_Int j = 0; j < nf; j++) {
      *ptr++ = new_faces[pf[j]];
    }
  }

  // points
  E_Int new_npoints = 0;
  std::vector<E_Int> new_points(M->npoints, -1);
  for (E_Int i = 0; i < sizeNGon; i++) {
    E_Int point = ngon[i];
    if (new_points[point] == -1)
      new_points[point] = new_npoints++;
  }

  // ecenter
  Edge E;
  std::map<Edge, E_Int> *ecenter = new std::map<Edge, E_Int>;

  for (auto& e : *(M->ecenter)) {
    E_Int ec = e.second;
    
    // Delete GONE edge centers
    if (new_points[ec] == -1)
      continue;
    
    E_Int n0 = e.first.p0_;
    E_Int n1 = e.first.p1_;
    assert(new_points[n0] != -1);
    assert(new_points[n1] != -1);
    
    E.set(new_points[n0], new_points[n1]);

    ecenter->insert({E, new_points[ec]});
  }

  for (E_Int i = 0; i < sizeNGon; i++) {
    ngon[i] = new_points[ngon[i]];
  }

  // xyz
  E_Float *x = (E_Float *)XMALLOC(new_npoints * sizeof(E_Float));
  E_Float *y = (E_Float *)XMALLOC(new_npoints * sizeof(E_Float));
  E_Float *z = (E_Float *)XMALLOC(new_npoints * sizeof(E_Float));

  for (E_Int i = 0; i < M->npoints; i++) {
    if (new_points[i] == -1) continue;
    x[new_points[i]] = M->x[i];
    y[new_points[i]] = M->y[i];
    z[new_points[i]] = M->z[i];
  }

  // owner and neigh
  E_Int *owner = (E_Int *)XMALLOC(new_nfaces * sizeof(E_Int));
  E_Int *neigh = (E_Int *)XMALLOC(new_nfaces * sizeof(E_Int));
  
  for (E_Int i = 0; i < M->nfaces; i++) {
    if (new_faces[i] == -1) continue;

    owner[new_faces[i]] = new_cells[M->owner[i]];

    if (M->neigh[i] == -1) neigh[new_faces[i]] = -1;
    else {
      neigh[new_faces[i]] = new_cells[M->neigh[i]];
    }
  }

  // patches
  for (E_Int i = 0; i < M->nbc; i++) {
    E_Int *ptlist = M->ptlists[i];

    // Replace
    for (E_Int j = 0; j < M->bcsizes[i]; j++) {
      ptlist[j] = new_faces[ptlist[j]];
    }
  }

  // ref_data
  //puts("Warning: ref data is not renumbered");
  int *ref_data = (E_Int *)XMALLOC(new_ncells * sizeof(int));
  for (E_Int i = 0; i < M->ncells; i++) {
    if (new_cells[i] != -1)
      ref_data[new_cells[i]] = M->ref_data[i];
  }

  // Free and replace
  XFREE(M->x);
  XFREE(M->y);
  XFREE(M->z);
  XFREE(M->nface);
  XFREE(M->indPH);
  XFREE(M->ngon);
  XFREE(M->indPG);
  XFREE(M->owner);
  XFREE(M->neigh);
  XFREE(M->ref_data); 
  delete M->ecenter;

  M->ncells = new_ncells;
  M->nfaces = new_nfaces;
  M->npoints = new_npoints;
  M->x = x;
  M->y = y;
  M->z = z;
  M->nface = nface;
  M->indPH = indPH;
  M->ngon = ngon;
  M->indPG = indPG;
  M->owner = owner;
  M->neigh = neigh;
  M->ref_data = ref_data;
  M->ecenter = ecenter;
}

void compress_mesh(AMesh *M, const std::vector<E_Int> &new_cells,
  const std::vector<E_Int> &new_faces, E_Int nc, E_Int nf,
  E_Int sizeNFace, E_Int sizeNGon)
{
  renumber_mesh(M, new_cells, new_faces, nc, nf, sizeNFace, sizeNGon);

  M->cellTree->compress(new_cells, nc);
    
  M->faceTree->compress(new_faces, nf);
}
