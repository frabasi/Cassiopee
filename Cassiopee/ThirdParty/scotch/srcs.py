import KCore.Dist as Dist
from KCore.config import *
#==============================================================================
# Fichiers c++
#==============================================================================
cpp_srcs = ['src/libscotch/kdgraph_map_st.c', 'src/libscotch/bgraph_bipart_fm.c', 'src/libscotch/dgraph_band.c', 'src/libscotch/arch_cmpltw.c', 'src/libscotch/library_dgraph_order_io_f.c', 'src/libscotch/arch_hcub.c', 'src/libscotch/hgraph_induce.c', 'src/libscotch/library_graph_io_habo.c', 'src/libscotch/bdgraph_bipart_df.c', 'src/libscotch/library_mesh_io_habo_f.c', 'src/libscotch/library_graph_base_f.c', 'src/libscotch/library_dgraph_gather_f.c', 'src/libscotch/graph_list.c', 'src/libscotch/vgraph_separate_ml.c', 'src/libscotch/library_arch_f.c', 'src/libscotch/library_dgraph_check.c', 'src/libscotch/vgraph_separate_vw.c', 'src/libscotch/library_graph_io_scot_f.c', 'src/libscotch/bdgraph_bipart_ex.c', 'src/libscotch/hdgraph_check.c', 'src/libscotch/library_memory_f.c', 'src/libscotch/library_graph_map_f.c', 'src/libscotch/hmesh_order_st.c', 'src/libscotch/hmesh_order_nd.c', 'src/libscotch/hall_order_hx.c', 'src/libscotch/hmesh_order_bl.c', 'src/libscotch/vdgraph_separate_st.c', 'src/libscotch/graph_check.c', 'src/libscotch/library_dgraph_scatter.c', 'src/libscotch/bgraph_check.c', 'src/libscotch/kdgraph_map_rb.c', 'src/libscotch/library_version_f.c', 'src/libscotch/kgraph_map_rb.c', 'src/libscotch/graph.c', 'src/libscotch/hdgraph_order_nd.c', 'src/libscotch/library_mesh_io_habo.c', 'src/libscotch/bgraph.c', 'src/libscotch/arch_sub.c', 'src/libscotch/vgraph_separate_zr.c', 'src/libscotch/vmesh_separate_zr.c', 'src/libscotch/dummysizes.c', 'src/libscotch/dgraph_match_sync_coll.c', 'src/libscotch/mesh.c', 'src/libscotch/vgraph_separate_th.c', 'src/libscotch/vmesh_separate_gr.c', 'src/libscotch/dgraph_view.c', 'src/libscotch/library_dgraph_order_io_block_f.c', 'src/libscotch/library_dgraph_halo_f.c', 'src/libscotch/library_dgraph_order_io.c', 'src/libscotch/kdgraph.c', 'src/libscotch/comm.c', 'src/libscotch/dgraph_halo.c', 'src/libscotch/vmesh_separate_fm.c', 'src/libscotch/hgraph_order_hd.c', 'src/libscotch/dgraph_induce.c', 'src/libscotch/hdgraph_fold.c', 'src/libscotch/library_dgraph_order_perm.c', 'src/libscotch/arch.c', 'src/libscotch/mesh_induce_sepa.c', 'src/libscotch/kdgraph_gather.c', 'src/libscotch/library_dgraph_stat_f.c', 'src/libscotch/arch_cmplt.c', 'src/libscotch/library_graph_io_scot.c', 'src/libscotch/common_file.c', 'src/libscotch/library_memory.c', 'src/libscotch/fibo.c', 'src/libscotch/mesh_coarsen.c', 'src/libscotch/library_mesh_graph_f.c', 'src/libscotch/mapping_io.c', 'src/libscotch/hdgraph_order_sq.c', 'src/libscotch/library_graph_io_mmkt.c', 'src/libscotch/hmesh_order_hx.c', 'src/libscotch/arch_dist.c', 'src/libscotch/library_dgraph.c', 'src/libscotch/geom.c', 'src/libscotch/library_graph_io_habo_f.c', 'src/libscotch/library_mesh_graph.c', 'src/libscotch/library_random_f.c', 'src/libscotch/hgraph_induce_edge.c', 'src/libscotch/dmapping.c', 'src/libscotch/library_graph_map_io.c', 'src/libscotch/bgraph_bipart_gp.c', 'src/libscotch/dgraph_allreduce.c', 'src/libscotch/dgraph_match_scan.c', 'src/libscotch/mesh_io.c', 'src/libscotch/graph_diam.c', 'src/libscotch/library_dgraph_io_save.c', 'src/libscotch/vgraph.c', 'src/libscotch/vgraph_check.c', 'src/libscotch/common_thread.c', 'src/libscotch/library_mesh_f.c', 'src/libscotch/hgraph_order_hx.c', 'src/libscotch/vdgraph_gather_all.c', 'src/libscotch/kgraph_map_df_loop.c', 'src/libscotch/library_error_exit.c', 'src/libscotch/gain.c', 'src/libscotch/library_dgraph_f.c', 'src/libscotch/kgraph_map_rb_part.c', 'src/libscotch/order_io.c', 'src/libscotch/kgraph_check.c', 'src/libscotch/vdgraph_separate_sq.c', 'src/libscotch/dorder_io_tree.c', 'src/libscotch/vmesh_separate_ml.c', 'src/libscotch/library_common_f.c', 'src/libscotch/library_dgraph_check_f.c', 'src/libscotch/library_dgraph_redist_f.c', 'src/libscotch/dgraph_gather.c', 'src/libscotch/graph_ielo.c', 'src/libscotch/vgraph_separate_df.c', 'src/libscotch/vdgraph_check.c', 'src/libscotch/library_version.c', 'src/libscotch/common_memory.c', 'src/libscotch/library_graph_map.c', 'src/libscotch/library_dgraph_band.c', 'src/libscotch/library_graph_check.c', 'src/libscotch/kdgraph_map_rb_map.c', 'src/libscotch/library_mesh_io_scot.c', 'src/libscotch/library_graph_diam.c', 'src/libscotch/dgraph_halo_fill.c', 'src/libscotch/graph_base.c', 'src/libscotch/hgraph_check.c', 'src/libscotch/library_dgraph_induce_f.c', 'src/libscotch/graph_clone.c', 'src/libscotch/library_mesh_io_scot_f.c', 'src/libscotch/hmesh_order_si.c', 'src/libscotch/library_dgraph_order_io_block.c', 'src/libscotch/graph_induce.c', 'src/libscotch/graph_io.c', 'src/libscotch/library_graph_io_chac.c', 'src/libscotch/kgraph_map_df.c', 'src/libscotch/dorder_io.c', 'src/libscotch/library_dgraph_coarsen_f.c', 'src/libscotch/library_graph_coarsen.c', 'src/libscotch/dorder_gather.c', 'src/libscotch/hmesh_mesh.c', 'src/libscotch/mapping.c', 'src/libscotch/library_order.c', 'src/libscotch/library_graph_color_f.c', 'src/libscotch/vdgraph_separate_ml.c', 'src/libscotch/hdgraph_gather.c', 'src/libscotch/hmesh_hgraph.c', 'src/libscotch/bdgraph.c', 'src/libscotch/wgraph_part_gg.c', 'src/libscotch/mesh_io_habo.c', 'src/libscotch/arch_vhcub.c', 'src/libscotch/vmesh_separate_gg.c', 'src/libscotch/library_dgraph_gather.c', 'src/libscotch/bgraph_bipart_gg.c', 'src/libscotch/hgraph_order_cp.c', 'src/libscotch/library_graph_io_mmkt_f.c', 'src/libscotch/bgraph_bipart_ml.c', 'src/libscotch/hgraph_order_nd.c', 'src/libscotch/hdgraph_order_si.c', 'src/libscotch/bgraph_bipart_st.c', 'src/libscotch/library_dgraph_band_f.c', 'src/libscotch/library_graph_map_io_f.c', 'src/libscotch/library_dgraph_coarsen.c', 'src/libscotch/library_graph_diam_f.c', 'src/libscotch/library_dgraph_order_f.c', 'src/libscotch/vgraph_separate_st.c', 'src/libscotch/library_graph.c', 'src/libscotch/hdgraph.c', 'src/libscotch/library_graph_io_chac_f.c', 'src/libscotch/library_graph_order.c', 'src/libscotch/hmesh_order_hd.c', 'src/libscotch/graph_match.c', 'src/libscotch/dgraph_match.c', 'src/libscotch/vdgraph_separate_zr.c', 'src/libscotch/library_mesh_order.c', 'src/libscotch/arch_tleaf.c', 'src/libscotch/bdgraph_bipart_st.c', 'src/libscotch/library_dgraph_order.c', 'src/libscotch/dgraph_gather_all.c', 'src/libscotch/graph_io_scot.c', 'src/libscotch/library_dgraph_map_view_f.c', 'src/libscotch/kgraph_map_bd.c', 'src/libscotch/library_dgraph_io_load.c', 'src/libscotch/vgraph_separate_gp.c', 'src/libscotch/library_parser_f.c', 'src/libscotch/graph_io_habo.c', 'src/libscotch/wgraph_part_fm.c', 'src/libscotch/parser.c', 'src/libscotch/graph_match_scan.c', 'src/libscotch/library_graph_part_ovl_f.c', 'src/libscotch/kgraph_map_ex.c', 'src/libscotch/common_file_compress.c', 'src/libscotch/hdgraph_induce.c', 'src/libscotch/order_check.c', 'src/libscotch/graph_io_mmkt.c', 'src/libscotch/library_dgraph_map_f.c', 'src/libscotch/hgraph_order_si.c', 'src/libscotch/vdgraph_separate_bd.c', 'src/libscotch/library_geom_f.c', 'src/libscotch/hmesh_induce.c', 'src/libscotch/dgraph_fold_comm.c', 'src/libscotch/library_graph_color.c', 'src/libscotch/hmesh_order_cp.c', 'src/libscotch/dgraph_io_save.c', 'src/libscotch/wgraph_part_rb.c', 'src/libscotch/bgraph_bipart_ex.c', 'src/libscotch/bdgraph_bipart_sq.c', 'src/libscotch/arch_deco.c', 'src/libscotch/bgraph_bipart_df.c', 'src/libscotch/vgraph_separate_gg.c', 'src/libscotch/common_integer.c', 'src/libscotch/library_dgraph_halo.c', 'src/libscotch/library_errcom.c', 'src/libscotch/bgraph_bipart_df_loop.c', 'src/libscotch/vgraph_separate_es.c', 'src/libscotch/hgraph_order_bl.c', 'src/libscotch/library_dgraph_grow.c', 'src/libscotch/graph_coarsen.c', 'src/libscotch/dorder_io_block.c', 'src/libscotch/hgraph_order_kp.c', 'src/libscotch/vmesh_separate_st.c', 'src/libscotch/kgraph_map_ml.c', 'src/libscotch/hgraph_order_st.c', 'src/libscotch/arch_torus.c', 'src/libscotch/hdgraph_order_st.c', 'src/libscotch/dgraph_scatter.c', 'src/libscotch/bdgraph_bipart_bd.c', 'src/libscotch/hmesh_order_gr.c', 'src/libscotch/hmesh_order_hf.c', 'src/libscotch/library_mesh_order_f.c', 'src/libscotch/bgraph_bipart_bd.c', 'src/libscotch/wgraph_part_zr.c', 'src/libscotch/library_dgraph_order_tree_dist_f.c', 'src/libscotch/library_graph_part_ovl.c', 'src/libscotch/dorder.c', 'src/libscotch/hgraph_order_hf.c', 'src/libscotch/bgraph_bipart_zr.c', 'src/libscotch/dgraph_ghst.c', 'src/libscotch/hgraph.c', 'src/libscotch/library_dgraph_io_save_f.c', 'src/libscotch/dgraph_match_check.c', 'src/libscotch/common_string.c', 'src/libscotch/vgraph_separate_fm.c', 'src/libscotch/vmesh_check.c', 'src/libscotch/library_graph_check_f.c', 'src/libscotch/library_error.c', 'src/libscotch/bdgraph_gather_all.c', 'src/libscotch/kdgraph_map_rb_part.c', 'src/libscotch/dgraph_redist.c', 'src/libscotch/wgraph_check.c', 'src/libscotch/vmesh_store.c', 'src/libscotch/hgraph_order_gp.c', 'src/libscotch/kgraph_map_cp.c', 'src/libscotch/arch_deco2.c', 'src/libscotch/dgraph_fold.c', 'src/libscotch/graph_coarsen_edge.c', 'src/libscotch/bdgraph_store.c', 'src/libscotch/library_dorder.c', 'src/libscotch/dgraph_match_sync_ptop.c', 'src/libscotch/hmesh_order_gp.c', 'src/libscotch/arch_vcmplt.c', 'src/libscotch/library_graph_coarsen_f.c', 'src/libscotch/library_dgraph_induce.c', 'src/libscotch/vgraph_separate_bd.c', 'src/libscotch/library_mapping.c', 'src/libscotch/vdgraph_store.c', 'src/libscotch/library_parser.c', 'src/libscotch/library_graph_map_view_f.c', 'src/libscotch/mesh_check.c', 'src/libscotch/dorder_perm.c', 'src/libscotch/library_geom.c', 'src/libscotch/library_dgraph_redist.c', 'src/libscotch/library_strat.c', 'src/libscotch/dgraph_fold_dup.c', 'src/libscotch/common.c', 'src/libscotch/library_graph_induce.c', 'src/libscotch/library_dgraph_order_gather_f.c', 'src/libscotch/mesh_io_scot.c', 'src/libscotch/dmapping_io.c', 'src/libscotch/library_dgraph_stat.c', 'src/libscotch/graph_band.c', 'src/libscotch/kgraph_map_fm.c', 'src/libscotch/library_graph_f.c', 'src/libscotch/wgraph_part_gp.c', 'src/libscotch/kgraph_store.c', 'src/libscotch/library_dgraph_order_gather.c', 'src/libscotch/vdgraph_separate_df.c', 'src/libscotch/bdgraph_bipart_ml.c', 'src/libscotch/library_random.c', 'src/libscotch/library_mesh.c', 'src/libscotch/bgraph_store.c', 'src/libscotch/library_graph_induce_f.c', 'src/libscotch/library_dmapping.c', 'src/libscotch/kgraph.c', 'src/libscotch/hall_order_hd.c', 'src/libscotch/bdgraph_bipart_zr.c', 'src/libscotch/library_graph_map_view.c', 'src/libscotch/wgraph_part_ml.c', 'src/libscotch/vmesh.c', 'src/libscotch/graph_io_chac.c', 'src/libscotch/common_stub.c', 'src/libscotch/hall_order_hf.c', 'src/libscotch/library_dgraph_scatter_f.c', 'src/libscotch/wgraph.c', 'src/libscotch/mesh_graph.c', 'src/libscotch/kgraph_band.c', 'src/libscotch/common_file_decompress.c', 'src/libscotch/library_graph_order_f.c', 'src/libscotch/dgraph_band_grow.c', 'src/libscotch/library_dgraph_map_view.c', 'src/libscotch/dgraph_io_load.c', 'src/libscotch/library_dgraph_order_perm_f.c', 'src/libscotch/bdgraph_check.c', 'src/libscotch/arch_mesh.c', 'src/libscotch/dgraph_check.c', 'src/libscotch/wgraph_part_st.c', 'src/libscotch/dorder_tree_dist.c', 'src/libscotch/kgraph_map_rb_map.c', 'src/libscotch/order.c', 'src/libscotch/hgraph_order_cc.c', 'src/libscotch/common_error.c', 'src/libscotch/dgraph_coarsen.c', 'src/libscotch/library_dgraph_io_load_f.c', 'src/libscotch/wgraph_store.c', 'src/libscotch/library_graph_base.c', 'src/libscotch/library_dgraph_order_tree_dist.c', 'src/libscotch/hmesh_check.c', 'src/libscotch/library_dgraph_map.c', 'src/libscotch/dgraph.c', 'src/libscotch/vgraph_store.c', 'src/libscotch/kgraph_map_st.c', 'src/libscotch/common_sort.c', 'src/libscotch/library_arch.c', 'src/libscotch/vdgraph.c', 'src/libscotch/hmesh.c', 'src/libscotch/parser_ll.c', 'src/libscotch/parser_yy.c', 'src/libscotch/last_resort/parser_yy.c', 'src/libscotch/last_resort/parser_ll.c']
#==============================================================================