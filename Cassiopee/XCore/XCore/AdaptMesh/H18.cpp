#include "Hexa.h"
#include "Mesh.h"

void H18_refine(Int hexa, Mesh *M)
{
    H18_reorder(hexa, M);

    Int *cell = Mesh_get_cell(M, hexa);
    Int *crange = Mesh_get_crange(M, hexa);

    Int FACES[24];
    memcpy(FACES, cell, 24 * sizeof(Int));

    Int *BOT = FACES;
    Int *TOP = FACES + 4;
    Int *LFT = FACES + 8;
    Int *RGT = FACES + 12;
    Int *FRO = FACES + 16;
    Int *BCK = FACES + 20;

    Int NODES[18];
    for (Int i = 0; i < 18; i++) NODES[i] = -1;

    // Local variables
    Int fid, *pn, i0, reorient, local[9];

    // BOT
    assert(crange[0] == 4);

    fid = BOT[0];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
    if (reorient) std::swap(local[1], local[3]);
    NODES[0]  = local[0];
    NODES[8]  = local[1];
    NODES[12] = local[2];
    NODES[11] = local[3];

    fid = BOT[1];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[8], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[8]);
    assert(local[3] == NODES[12]);
    NODES[1] = local[1];
    NODES[9] = local[2];

    fid = BOT[2];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[12], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[12]);
    assert(local[1] == NODES[9]);
    NODES[2] = local[2];
    NODES[10] = local[3];

    fid = BOT[3];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[11], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[11]);
    assert(local[1] == NODES[12]);
    assert(local[2] == NODES[10]);
    NODES[3] = local[3];

    // LFT
    assert(crange[2] == 2);

    fid = LFT[0];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[0], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[2]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[0]);
    assert(local[1] == NODES[11]);
    NODES[13] = local[2];
    NODES[4] = local[3];

    fid = LFT[1];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[11], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[2]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[11]);
    assert(local[1] == NODES[3]);
    assert(local[3] == NODES[13]);
    NODES[7] = local[2];

    // RGT
    assert(crange[3] == 2);

    fid = RGT[0];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[1], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[3]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[1]);
    assert(local[1] == NODES[9]);
    NODES[14] = local[2];
    NODES[5] = local[3];

    fid = RGT[1];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[9], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[3]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[9]);
    assert(local[1] == NODES[2]);
    assert(local[3] == NODES[14]);
    NODES[6] = local[2];

    // FRO
    assert(crange[4] == 2);

    fid = FRO[0];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[1], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[4]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[1]);
    assert(local[1] == NODES[8]);
    assert(local[3] == NODES[5]);
    NODES[15] = local[2];

    fid = FRO[1];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[8], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[4]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[8]);
    assert(local[1] == NODES[0]);
    assert(local[2] == NODES[4]);
    assert(local[3] == NODES[15]);

    // BCK
    assert(crange[5] == 2);

    fid = BCK[0];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[2], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[5]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[2]);
    assert(local[1] == NODES[10]);
    assert(local[3] == NODES[6]);
    NODES[16] = local[2];

    fid = BCK[1];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[10], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[5]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[10]);
    assert(local[1] == NODES[3]);
    assert(local[2] == NODES[7]);
    assert(local[3] == NODES[16]);

    // TOP
    assert(crange[1] == 4);

    fid = TOP[0];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[4]);
    assert(local[1] == NODES[15]);
    assert(local[3] == NODES[13]);
    NODES[17] = local[2];

    fid = TOP[1];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[15], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[15]);
    assert(local[1] == NODES[5]);
    assert(local[2] == NODES[14]);
    assert(local[3] == NODES[17]);

    fid = TOP[2];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[17], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[17]);
    assert(local[1] == NODES[14]);
    assert(local[2] == NODES[6]);
    assert(local[3] == NODES[16]);

    fid = TOP[3];
    pn = Mesh_get_face(M, fid);
    for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
    i0 = Get_pos(NODES[13], local, 4);
    Right_shift(local, i0, 4);
    reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
    if (reorient) std::swap(local[1], local[3]);
    assert(local[0] == NODES[13]);
    assert(local[1] == NODES[17]);
    assert(local[2] == NODES[16]);
    assert(local[3] == NODES[7]);

    // Set internal faces in ngon
    Int *face = NULL;

    face = Mesh_get_face(M, M->nf);
    face[0] = NODES[8];  face[2] = NODES[12];
    face[4] = NODES[17]; face[6] = NODES[15];

    face = Mesh_get_face(M, M->nf+1);
    face[0] = NODES[9];  face[2] = NODES[12];
    face[4] = NODES[17]; face[6] = NODES[14];

    face = Mesh_get_face(M, M->nf+2);
    face[0] = NODES[12]; face[2] = NODES[10];
    face[4] = NODES[16]; face[6] = NODES[17];

    face = Mesh_get_face(M, M->nf+3);
    face[0] = NODES[12]; face[2] = NODES[11];
    face[4] = NODES[13]; face[6] = NODES[17];

    // Update internal face strides, ranges and states
    for (Int i = 0; i < 4; i++) {
        Int fid = M->nf + i;
        Int *frange = Mesh_get_frange(M, fid);
        for (Int j = 0; j < 4; j++) frange[j] = 1;
        M->fstride[fid] = 4;
        M->fref[fid] = FACE_NEW;
    }

    // Assemble children
    Int *child = NULL;

    // First child replaces hexa
    child = Mesh_get_cell(M, hexa);
    memset(child, -1, 24*sizeof(Int));
    child[0]  = BOT[0]; child[4]  = TOP[0];
    child[8]  = LFT[0]; child[12] = M->nf;
    child[16] = FRO[1]; child[20] = M->nf+3;

    // nc
    child = Mesh_get_cell(M, M->nc);
    child[0]  = BOT[1]; child[4]  = TOP[1];
    child[8]  = M->nf;  child[12] = RGT[0];
    child[16] = FRO[0]; child[20] = M->nf+1;

    // nc+1
    child = Mesh_get_cell(M, M->nc+1);
    child[0]  = BOT[2];  child[4]  = TOP[2];
    child[8]  = M->nf+2; child[12] = RGT[1];
    child[16] = M->nf+1; child[20] = BCK[0];

    // nc+2
    child = Mesh_get_cell(M, M->nc+2);
    child[0]  = BOT[3];  child[4]  = TOP[3];
    child[8]  = LFT[1];  child[12] = M->nf+2;
    child[16] = M->nf+3; child[20] = BCK[1];

    // Fix range and strides
    update_range_and_stride(M, hexa, M->nc, 3);

    // Update adaptation info
    M->clevel[hexa]++;

    for (Int i = 0; i < 3; i++) {
        M->clevel[M->nc+i] = M->clevel[hexa];
        M->ctype[M->nc+i] = M->ctype[hexa];
    }

    M->cchildren[hexa] = {hexa, M->nc, M->nc+1, M->nc+2};

    // Set shell faces owns and neis
    update_shell_pe(hexa, M);

    // Set owns and neis of internal faces
    M->owner[M->nf] = hexa;
    M->neigh[M->nf] = M->nc;

    M->owner[M->nf+1] = M->nc;
    M->neigh[M->nf+1] = M->nc+1;

    M->owner[M->nf+2] = M->nc+2;
    M->neigh[M->nf+2] = M->nc+1;

    M->owner[M->nf+3] = hexa;
    M->neigh[M->nf+3] = M->nc+2;

    // Update level/type of internal faces
    for (Int i = 0; i < 4; i++) {
        M->flevel[M->nf+i] = M->clevel[hexa];
        M->ftype[M->nf+i] = QUAD;
    }

    assert(check_canon_hexa(hexa, M) == 0);
    for (Int i = 0; i < 3; i++) assert(check_canon_hexa(M->nc+i, M) == 0);

    // Increment face/hexa count
    M->nf += 4;
    M->nc += 3;
}

void H18_reorder(Int hexa, Mesh *M)
{
    Int NODES[18];
    for (Int i = 0; i < 18; i++) NODES[i] = -1;

    Int local[8], i0;

    Int *cell = Mesh_get_cell(M, hexa);
    Int *crange = Mesh_get_crange(M, hexa);

    Int FACES[24];
    for (Int i = 0; i < 24; i++) FACES[i] = cell[i];

    Int *BOT = FACES;

    if (crange[0] == 4) {
        Int first = 0;

        Int fid = BOT[first];

        Int *pn = Mesh_get_face(M, fid);

        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        Int reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
        if (reorient) std::swap(local[1], local[3]);

        // Find second, third and fourth sides of BOT

        Int second, third, fourth;
        second = third = fourth = first;

        for (Int i = 1; i < 4; i++) {
            Int side = cell[i];
            Int *pn = Mesh_get_face(M, side);
            Int common[4] = {0, 0, 0, 0};
            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                for (Int k = 0; k < 4; k++) {
                    if (local[k] == point) {
                        common[k] = 1;
                        break;
                    }
                }
            }
            if (common[1] && common[2]) second = i;
            else if (common[2] && common[3]) fourth = i;
            else third = i;
        }

        assert(second != first);
        assert(third != first);
        assert(fourth != first);

        // Fill bot nodes
        NODES[0]  = local[0];
        NODES[8]  = local[1];
        NODES[12] = local[2];
        NODES[11] = local[3];

        // Setup second face
        fid = BOT[second];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[8], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[8]);
        assert(local[3] == NODES[12]);
        NODES[1] = local[1];
        NODES[9] = local[2];

        // Setup third face
        fid = BOT[third];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[12], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[12]);
        assert(local[1] == NODES[9]);
        NODES[2]  = local[2];
        NODES[10] = local[3];

        // Setup fourth face
        fid = BOT[fourth];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[11], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[0]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[11]);
        assert(local[1] == NODES[12]);
        assert(local[2] == NODES[10]);
        NODES[3] = local[3];
        
        Int tmp[4] = {BOT[first], BOT[second], BOT[third], BOT[fourth]};
        for (Int i = 0; i < 4; i++) BOT[i] = tmp[i];
    } else {
        assert(crange[0] == 1);
        assert(BOT[1] == -1);
        assert(BOT[2] == -1);
        assert(BOT[3] == -1);
        Int *pn = Mesh_get_face(M, BOT[0]);
        for (Int i = 0; i < 8; i++) local[i] = pn[i];
        Int reorient = Mesh_get_reorient(M, BOT[0], hexa, normalIn_H[0]);
        if (reorient) std::reverse(local+1, local+8);
        NODES[0]  = local[0];
        NODES[1]  = local[2];
        NODES[2]  = local[4];
        NODES[3]  = local[6];
        NODES[8]  = local[1];
        NODES[9]  = local[3];
        NODES[10] = local[5];
        NODES[11] = local[7];
    }

    for (Int i = 0; i < 4; i++) cell[i] = BOT[i];

    BOT = cell;
    Int *TOP = cell + 4;
    Int *LFT = cell + 8;
    Int *RGT = cell + 12;
    Int *FRO = cell + 16;
    Int *BCK = cell + 20;
    
    // Find TOP, LFT, RGT, FRO, BCK

    Int tmp_crange[6] = {crange[0], -1, -1, -1, -1, -1};

    for (Int i = 1; i < 6; i++) {
        Int *SIDE = FACES + 4*i;

        std::set<Int> points;

        for (Int j = 0; j < crange[i]; j++) {
            Int fid = SIDE[j];
            assert(fid != -1);
            Int *pn = Mesh_get_face(M, fid);
            for (Int k = 0; k < 8; k += 2) points.insert(pn[k]);
        }

        Int common[4] = {0, 0, 0, 0};

        for (Int point : points) {
            for (Int j = 0; j < 4; j++) {
                if (NODES[j] == point) {
                    common[j] = 1;
                    break;
                }
            }
        }

        if      (common[0] && common[3]) {
            tmp_crange[2] = crange[i];
            for (Int j = 0; j < 4; j++) LFT[j] = SIDE[j];
        }
        else if (common[1] && common[2]) {
            tmp_crange[3] = crange[i];
            for (Int j = 0; j < 4; j++) RGT[j] = SIDE[j];
        }
        else if (common[0] && common[1]) {
            tmp_crange[4] = crange[i];
            for (Int j = 0; j < 4; j++) FRO[j] = SIDE[j];
        }
        else if (common[2] && common[3]) {
            tmp_crange[5] = crange[i];
            for (Int j = 0; j < 4; j++) BCK[j] = SIDE[j];
        }
        else                             {
            tmp_crange[1] = crange[i];
            for (Int j = 0; j < 4; j++) TOP[j] = SIDE[j];
        }
    }

    for (Int i = 0; i < 6; i++) assert(tmp_crange[i] != -1);

    for (Int i = 0; i < 6; i++) crange[i] = tmp_crange[i];

    // Reorder LFT sides

    if (crange[2] == 2) {
        // First face must share NODES[0]
        
        Int first = -1;

        for (Int i = 0; i < 2 && first == -1; i++) {
            Int fid = LFT[i];
            Int *pn = Mesh_get_face(M, fid);
            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                if (point == NODES[0]) {
                    first = i;
                    break;
                }
            }
        }

        assert(first != -1);

        Int second = (first+1)%2;

        // Setup first face
        Int fid = LFT[first];
        Int *pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        Int i0 = Get_pos(NODES[0], local, 4);
        Right_shift(local, i0, 4);
        Int reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[2]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[0]);
        assert(local[1] == NODES[11]);
        NODES[13] = local[2];
        NODES[4] = local[3];

        // Setup second face
        fid = LFT[second];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[11], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[2]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[11]);
        assert(local[1] == NODES[3]);
        assert(local[3] == NODES[13]);
        NODES[7] = local[2];

        Int tmp[2] = {LFT[first], LFT[second]};
        for (Int i = 0; i < 2; i++) LFT[i] = tmp[i];
    } else {
        assert(crange[2] == 1);
        assert(LFT[1] == -1);
        assert(LFT[2] == -1);
        assert(LFT[3] == -1);
        Int *pn = Mesh_get_face(M, LFT[0]);
        for (Int i = 0; i < 8; i++) local[i] = pn[i];
        i0 = Get_pos(NODES[0], local, 8);
        Right_shift(local, i0, 8);
        Int reorient = Mesh_get_reorient(M, LFT[0], hexa, normalIn_H[2]);
        if (reorient) std::reverse(local+1, local+8);
        assert(local[0] == NODES[0]);
        assert(local[1] == NODES[11]);
        assert(local[2] == NODES[3]);
        NODES[7]  = local[4];
        NODES[13] = local[5];
        NODES[4]  = local[6];
    }

    // Reorder RGT sides

    if (crange[3] == 2) {
        // First face must share NODES[1]
        
        Int first = -1;

        for (Int i = 0; i < 2 && first == -1; i++) {
            Int fid = RGT[i];
            Int *pn = Mesh_get_face(M, fid);
            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                if (point == NODES[1]) {
                    first = i;
                    break;
                }
            }
        }

        assert(first != -1);

        Int second = (first+1)%2;

        // Setup first face
        Int fid = RGT[first];
        Int *pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        Int i0 = Get_pos(NODES[1], local, 4);
        Right_shift(local, i0, 4);
        Int reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[3]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[1]);
        assert(local[1] == NODES[9]);
        NODES[14] = local[2];
        NODES[5] = local[3];

        // Setup second face
        fid = RGT[second];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[9], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[3]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[9]);
        assert(local[1] == NODES[2]);
        assert(local[3] == NODES[14]);
        NODES[6] = local[2];

        Int tmp[2] = {RGT[first], RGT[second]};
        for (Int i = 0; i < 2; i++) RGT[i] = tmp[i];
    } else {
        assert(crange[3] == 1);
        assert(RGT[1] == -1);
        assert(RGT[2] == -1);
        assert(RGT[3] == -1);
        Int *pn = Mesh_get_face(M, RGT[0]);
        for (Int i = 0; i < 8; i++) local[i] = pn[i];
        i0 = Get_pos(NODES[1], local, 8);
        Right_shift(local, i0, 8);
        Int reorient = Mesh_get_reorient(M, RGT[0], hexa, normalIn_H[3]);
        if (reorient) std::reverse(local+1, local+8);
        assert(local[0] == NODES[0]);
        assert(local[1] == NODES[9]);
        assert(local[2] == NODES[2]);
        NODES[6]  = local[4];
        NODES[14] = local[5];
        NODES[5]  = local[6];
    }

    // Reorder FRO sides

    if (crange[4] == 2) {
        // First face must share NODES[1]
        
        Int first = -1;

        for (Int i = 0; i < 2 && first == -1; i++) {
            Int fid = FRO[i];
            Int *pn = Mesh_get_face(M, fid);
            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                if (point == NODES[1]) {
                    first = i;
                    break;
                }
            }
        }

        assert(first != -1);

        Int second = (first+1)%2;

        // Setup first face
        Int fid = FRO[first];
        Int *pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        Int i0 = Get_pos(NODES[1], local, 4);
        Right_shift(local, i0, 4);
        Int reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[4]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[1]);
        assert(local[1] == NODES[8]);
        assert(local[3] == NODES[5]);
        NODES[15] = local[2];

        // Setup second face
        fid = FRO[second];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[8], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[4]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[8]);
        assert(local[1] == NODES[0]);
        assert(local[2] == NODES[4]);
        assert(local[3] == NODES[15]);

        Int tmp[2] = {FRO[first], FRO[second]};
        for (Int i = 0; i < 2; i++) FRO[i] = tmp[i];
    } else {
        assert(crange[4] == 1);
        assert(FRO[1] == -1);
        assert(FRO[2] == -1);
        assert(FRO[3] == -1);
        Int *pn = Mesh_get_face(M, FRO[0]);
        for (Int i = 0; i < 8; i++) local[i] = pn[i];
        i0 = Get_pos(NODES[1], local, 8);
        Right_shift(local, i0, 8);
        Int reorient = Mesh_get_reorient(M, FRO[0], hexa, normalIn_H[4]);
        if (reorient) std::reverse(local+1, local+8);
        assert(local[0] == NODES[1]);
        assert(local[1] == NODES[8]);
        assert(local[2] == NODES[0]);
        assert(local[4] == NODES[4]);
        assert(local[6] == NODES[5]);
        NODES[15] = local[5];
    }

    // Reorder BCK sides

    if (crange[5] == 2) {
        // First face must share NODES[2]

        Int nn0, nn1, pn0[8], pn1[8];
        Mesh_get_fpoints(M, BCK[0], nn0, pn0);
        Mesh_get_fpoints(M, BCK[1], nn1, pn1);
        
        Int first = -1;

        for (Int i = 0; i < 2 && first == -1; i++) {
            Int fid = BCK[i];
            Int *pn = Mesh_get_face(M, fid);
            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                if (point == NODES[2]) {
                    first = i;
                    break;
                }
            }
        }

        assert(first != -1);

        Int second = (first+1)%2;

        // Setup first face
        Int fid = BCK[first];
        Int *pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        Int i0 = Get_pos(NODES[2], local, 4);
        Right_shift(local, i0, 4);
        Int reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[5]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[2]);
        assert(local[1] == NODES[10]);
        assert(local[3] == NODES[6]);
        NODES[16] = local[2];
        assert(NODES[16] != -1);

        // Setup second face
        fid = BCK[second];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[10], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[5]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[10]);
        assert(local[1] == NODES[3]);
        assert(local[2] == NODES[7]);
        assert(local[3] == NODES[16]);

        Int tmp[2] = {BCK[first], BCK[second]};
        for (Int i = 0; i < 2; i++) BCK[i] = tmp[i];
    } else {
        assert(crange[5] == 1);
        assert(BCK[1] == -1);
        assert(BCK[2] == -1);
        assert(BCK[3] == -1);
        Int *pn = Mesh_get_face(M, BCK[0]);
        for (Int i = 0; i < 8; i++) local[i] = pn[i];
        i0 = Get_pos(NODES[2], local, 8);
        Right_shift(local, i0, 8);
        Int reorient = Mesh_get_reorient(M, BCK[0], hexa, normalIn_H[5]);
        if (reorient) std::reverse(local+1, local+8);
        assert(local[0] == NODES[2]);
        assert(local[1] == NODES[10]);
        assert(local[2] == NODES[3]);
        assert(local[4] == NODES[7]);
        assert(local[6] == NODES[6]);
        NODES[16] = local[5];
    }

    // Reorder TOP sides

    if (crange[1] == 4) {
        // First face must share NODES[4]

        Int first = -1;

        for (Int i = 0; i < 4 && first == -1; i++) {
            Int fid = TOP[i];
            Int *pn = Mesh_get_face(M, fid);
            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                if (point == NODES[4]) {
                    first = i;
                    break;
                }
            }
        }

        assert(first != -1);

        // Setup first face
        Int fid = TOP[first];
        Int *pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        Int i0 = Get_pos(NODES[4], local, 4);
        Right_shift(local, i0, 4);
        Int reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[4]);
        assert(local[1] == NODES[15]);
        assert(local[3] == NODES[13]);
        NODES[17] = local[2];

        // Get second, third and fourth sides
        Int second, third, fourth;
        second = third = fourth = -1;

        for (Int i = 0; i < 4; i++) {
            if (i == first) continue;
            
            Int fid = TOP[i];
            Int *pn = Mesh_get_face(M, fid);

            Int common[4] = {0, 0, 0, 0};

            for (Int j = 0; j < 4; j++) {
                Int point = pn[2*j];
                for (Int k = 0; k < 4; k++) {
                    if (local[k] == point) {
                        common[k] = 1;
                        break;
                    }
                }
            }

            if (common[1] && common[2]) second = i;
            else if (common[2] && common[3]) fourth = i;
            else third = i;
        }

        assert(second != -1);
        assert(third != -1);
        assert(fourth != -1);

        // Setup second face
        fid = TOP[second];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[15], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[15]);
        assert(local[1] == NODES[5]);
        assert(local[2] == NODES[14]);
        assert(local[3] == NODES[17]);

        // Setup third face
        fid = TOP[third];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[17], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[17]);
        assert(local[1] == NODES[14]);
        assert(local[2] == NODES[6]);
        assert(local[3] == NODES[16]);

        // Setup fourth face
        fid = TOP[fourth];
        pn = Mesh_get_face(M, fid);
        for (Int i = 0; i < 4; i++) local[i] = pn[2*i];
        i0 = Get_pos(NODES[13], local, 4);
        Right_shift(local, i0, 4);
        reorient = Mesh_get_reorient(M, fid, hexa, normalIn_H[1]);
        if (reorient) std::swap(local[1], local[3]);
        assert(local[0] == NODES[13]);
        assert(local[1] == NODES[17]);
        assert(local[2] == NODES[16]);
        assert(local[3] == NODES[7]);

        Int tmp[4] = {TOP[first], TOP[second], TOP[third], TOP[fourth]};
        for (Int i = 0; i < 4; i++) TOP[i] = tmp[i];
    } else {
        assert(crange[1] == 1);
        assert(TOP[1] == -1);
        assert(TOP[2] == -1);
        assert(TOP[3] == -1);
        Int *pn = Mesh_get_face(M, TOP[0]);
        for (Int i = 0; i < 8; i++) local[i] = pn[i];
        i0 = Get_pos(NODES[4], local, 8);
        Right_shift(local, i0, 8);
        Int reorient = Mesh_get_reorient(M, TOP[0], hexa, normalIn_H[1]);
        if (reorient) std::reverse(local+1, local+8);
        assert(local[0] == NODES[4]);
        assert(local[1] == NODES[15]);
        assert(local[2] == NODES[5]);
        assert(local[3] == NODES[14]);
        assert(local[4] == NODES[6]);
        assert(local[5] == NODES[16]);
        assert(local[6] == NODES[7]);
        assert(local[7] == NODES[13]);
    }
}