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
#pragma once

#include <vector>
#include <cstddef>

#include "common/common.h"
#include "queue.h"

struct Vertex;
struct Hedge;
struct Face;
struct Segment;
struct Smesh;
struct Cycle;

struct Dcel {
    std::vector<Vertex *> V;
    std::vector<Hedge *> H;
    std::vector<Face *> F;
    std::vector<Cycle *> C;

    Queue Q; // Filters out duplicate vertices

    Face *f_unbounded[2];

    static Int RED;
    static Int BLACK;
    static Int NO_IDEA;

    Dcel(const Smesh &M0, const Smesh &M1);
    ~Dcel();
    
    void init_vertices(const Smesh &M0, const Smesh &M1);

    void init_hedges_and_faces(const Smesh &M, Int color);

    static Int check_hedges(const std::vector<Hedge *> &H);

    static Int check_faces(const std::vector<Hedge *> &H,
        const std::vector<Face *> &F);

    void find_intersections();

    static void resolve(Vertex *p, const std::vector<Segment *> &L,
        const std::vector<Segment *> &C, const std::vector<Segment *> &U,
        std::vector<Hedge *> &H);
    
    void make_cycles();

    void set_face_labels(std::vector<Face *> &F);

    Hedge *get_hedge_of_color(Face *f, Int color);

    std::vector<Face *> make_cycle_faces(const std::vector<Cycle *> &C);

    void update_hedge_faces(const std::vector<Face *> &F);

    void set_cycles_inout();

    std::vector<Int> extract_indices_of_type(Int inout);
    
    std::vector<Face *> extract_faces_of_indices(
        const std::vector<Int> &indices);

    void write_ngon(const char *fname, const std::vector<Face *> &faces) const;

    void write_degen_faces(const char *fname);
    
    void write_outer_faces(const char *fname);
    
    void write_inner_faces(const char *fname);

    static std::vector<Vertex *> get_face_vertices(Face *f);
};
