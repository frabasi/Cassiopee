# paradigma source path
PPATH = 'XCore/paradigma23'

#==============================================================================
# Fichiers c++
#==============================================================================
cpp_srcs = ['ext_wrapper/pdm_ext_wrapper.c', 'io/pdm_error.c', 'io/pdm_logging.c', 'io/pdm_printf.c', 'io/pdm_vtk.c', 'mesh/pdm_cellface_orient.c', 'mesh/pdm_closest_points.c', 'mesh/pdm_dconnectivity_transform.c', 'mesh/pdm_dgeom_elem.c', 'mesh/pdm_dist_cloud_surf.c', 'mesh/pdm_dmesh_nodal.c', 'mesh/pdm_dmesh_nodal_elements_utils.c', 'mesh/pdm_dmesh_nodal_elmts.c', 'mesh/pdm_dmesh_nodal_to_dmesh.c', 'mesh/pdm_dmesh_to_dmesh_nodal.c', 'mesh/pdm_domain_interface.c', 'mesh/pdm_edges_intersect.c', 'mesh/pdm_elt_parent_find.c', 'mesh/pdm_field_cell_to_vtx.c', 'mesh/pdm_geom_elem.c', 'mesh/pdm_graph_bound.c', 'mesh/pdm_ho_basis.c', 'mesh/pdm_ho_bezier.c', 'mesh/pdm_ho_bezier_basis.c', 'mesh/pdm_ho_location.c', 'mesh/pdm_ho_ordering.c', 'mesh/pdm_ho_seg_intersect.c', 'mesh/pdm_lagrange_to_bezier.c', 'mesh/pdm_line.c', 'mesh/pdm_mean_values.c', 'mesh/pdm_mesh_check.c', 'mesh/pdm_mesh_intersection.c', 'mesh/pdm_mesh_intersection_surf_surf_atomic.c', 'mesh/pdm_mesh_intersection_vol_vol_atomic.c', 'mesh/pdm_mesh_location.c', 'mesh/pdm_mesh_nodal.c', 'mesh/pdm_overlay.c', 'mesh/pdm_part_bound.c', 'mesh/pdm_part_connectivity_transform.c', 'mesh/pdm_part_domain_interface.c', 'mesh/pdm_part_mesh_nodal.c', 'mesh/pdm_part_mesh_nodal_elmts.c', 'mesh/pdm_part_mesh_nodal_elmts_utils.c', 'mesh/pdm_part_mesh_nodal_to_pmesh.c', 'mesh/pdm_plane.c', 'mesh/pdm_points_merge.c', 'mesh/pdm_point_location.c', 'mesh/pdm_polygon.c', 'mesh/pdm_poly_clipp.c', 'mesh/pdm_predicate.c', 'mesh/pdm_surf_mesh.c', 'mesh/pdm_surf_part.c', 'mesh/pdm_tetrahedron.c', 'mesh/pdm_triangle.c', 'mesh/pdm_triangulate.c', 'meshgen/pdm_box_gen.c', 'meshgen/pdm_dcube_gen.c', 'meshgen/pdm_dcube_nodal_gen.c', 'meshgen/pdm_generate_mesh.c', 'meshgen/pdm_point_cloud_gen.c', 'meshgen/pdm_poly_surf_gen.c', 'meshgen/pdm_poly_vol_gen.c', 'meshgen/pdm_sphere_surf_gen.c', 'meshgen/pdm_sphere_vol_gen.c', 'pario/pdm_file_par.c', 'pario/pdm_file_seq.c', 'pario/pdm_io.c', 'pario/pdm_io_tab.c', 'pario/pdm_reader_gamma.c', 'pario/pdm_reader_stl.c', 'pario/pdm_writer.c', 'pario/pdm_writer_ensight.c', 'pario/pdm_writer_ensight_case.c', 'pdm.c', 'ppart/pdm_dmesh_partitioning.c', 'ppart/pdm_hilbert.c', 'ppart/pdm_multipart.c', 'ppart/pdm_para_graph_dual.c', 'ppart/pdm_part.c', 'ppart/pdm_partitioning_algorithm.c', 'ppart/pdm_partitioning_nodal_algorithm.c', 'ppart/pdm_part_coarse_mesh.c', 'ppart/pdm_part_extension.c', 'ppart/pdm_part_geom.c', 'ppart/pdm_part_renum.c', 'struct/pdm_binary_search.c', 'struct/pdm_block_to_block.c', 'struct/pdm_block_to_part.c', 'struct/pdm_box.c', 'struct/pdm_box_tree.c', 'struct/pdm_compare_operator.c', 'struct/pdm_cuthill.c', 'struct/pdm_dbbtree.c', 'struct/pdm_distant_neighbor.c', 'struct/pdm_distrib.c', 'struct/pdm_dmesh.c', 'struct/pdm_dmesh_extract.c', 'struct/pdm_doctree.c', 'struct/pdm_doctree_algorithm.c', 'struct/pdm_equal_operator.c', 'struct/pdm_extract_part.c', 'struct/pdm_global_mean.c', 'struct/pdm_global_reduce.c', 'struct/pdm_gnum.c', 'struct/pdm_gnum_from_hash_values.c', 'struct/pdm_gnum_location.c', 'struct/pdm_handles.c', 'struct/pdm_hash_tab.c', 'struct/pdm_interpolate_from_mesh_location.c', 'struct/pdm_kdtree_seq.c', 'struct/pdm_linear_programming.c', 'struct/pdm_morton.c', 'struct/pdm_multi_block_merge.c', 'struct/pdm_multi_block_to_part.c', 'struct/pdm_octree.c', 'struct/pdm_octree_seq.c', 'struct/pdm_order.c', 'struct/pdm_para_octree.c', 'struct/pdm_part_graph.c', 'struct/pdm_part_mesh.c', 'struct/pdm_part_to_block.c', 'struct/pdm_part_to_part.c', 'struct/pdm_point_tree_seq.c', 'struct/pdm_quick_sort.c', 'struct/pdm_radix_sort.c', 'struct/pdm_sort.c', 'struct/pdm_unique.c', 'util/pdm_array.c', 'util/pdm_fortran_to_c_string.c', 'util/pdm_hkey.c', 'util/pdm_linear_algebra.c', 'util/pdm_memory_stats.c', 'util/pdm_mpi_node_first_rank.c', 'util/pdm_remove_blank.c', 'util/pdm_timer.c', 'util/pdm_version.c', 'mpi_wrapper/no_mpi/pdm_no_mpi.c', 'mpi_wrapper/no_mpi/pdm_no_mpi_ext_dependencies.c', 'mpi_wrapper/mpi/pdm_mpi.c', 'mpi_wrapper/mpi/pdm_mpi_ext_dependencies.c']

# Fichiers pyx
pyx_srcs = ['XCore/paradigma23/Cython/Pypdm.pyx']
