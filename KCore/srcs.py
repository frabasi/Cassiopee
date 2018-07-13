NUGA = True
METIS = True

#==============================================================================
# Fichiers sources c++ (sauf le module)
#==============================================================================
cpp_srcs = ['KCore/isNamePresent.cpp',
            'KCore/OmpMaxThreads.cpp',
            'KCore/empty.cpp',
            'KCore/tester.cpp',
            'KCore/Def/DefCplusPlusConst.cpp',
            'KCore/activation.cpp',
            'KCore/Array/cleanArrays.cpp',
            'KCore/Array/getNumberOfVariables.cpp',
            'KCore/Array/getFromArray.cpp',
            'KCore/Array/getFromArray2.cpp',
            'KCore/Array/getFromArrays.cpp',
            'KCore/Array/buildArray.cpp',
            'KCore/Array/buildArray2.cpp',
            'KCore/Array/addFieldInArray.cpp',
            'KCore/Array/getFromArrayDyn.cpp',
            'KCore/Array/buildArrayDyn.cpp',
            'KCore/Array/isNamePresent.cpp',
            'KCore/Array/extractVars.cpp',
            'KCore/Array/getPosition.cpp',
            'KCore/Array/getVarName.cpp',
            'KCore/Array/getArrayPtr.cpp',
            'KCore/Array/compareVarStrings.cpp',
            'KCore/Array/addPrefixInVarString.cpp',
            'KCore/Array/getVarStringFromArray.cpp',
            'KCore/Array/getSizeFromArray.cpp',
            'KCore/Array/getInfoFromArray.cpp',
            'KCore/Array/getFromList.cpp',
            'KCore/Numpy/getFromNumpyArray.cpp',
            'KCore/Numpy/buildNumpyArray.cpp',
            'KCore/Numpy/getNumpyPtr.cpp',
            'KCore/PyTree/getFromZone.cpp',
            'KCore/PyTree/get.cpp',
            'KCore/PyTree/set.cpp',
            'KCore/String/kstring.cpp',
            'KCore/CompGeom/bezier.cpp',
            'KCore/CompGeom/pointIn.cpp',
            'KCore/CompGeom/distanceTo.cpp',
            'KCore/CompGeom/compCartElt.cpp',
            'KCore/CompGeom/projectOrtho.cpp',
            'KCore/CompGeom/projectDir.cpp',
            'KCore/CompGeom/projectRay.cpp',
            'KCore/CompGeom/compGeom.cpp',
            'KCore/CompGeom/intersect2Segments.cpp',
            'KCore/CompGeom/circumCircle.cpp',
            'KCore/CompGeom/intersectRayTriangle.cpp',
            'KCore/CompGeom/computeParamCoord.cpp',
            'KCore/CompGeom/spline.cpp',
            'KCore/CompGeom/compCurvature.cpp',
            'KCore/CompGeom/compCurvatureAngle.cpp',
            'KCore/CompGeom/compCurvatureHeight.cpp',
            'KCore/CompGeom/Triangle.cpp',
            'KCore/CompGeom/delaunay.cpp',
            'KCore/CompGeom/nurbs.cpp',
            'KCore/CompGeom/barycenter.cpp',
            'KCore/CompGeom/UBSSurface.cpp',
            'KCore/CompGeom/trianglesIntersection.cpp',
            'KCore/CompGeom/getAlphaAngleBetweenElts.cpp',
            'KCore/CompGeom/convexity.cpp',
            'KCore/CompGeom/getEdgeLength.cpp',
            'KCore/Connect/connectEV2VE.cpp',
            'KCore/Connect/connectEV2VNbrs.cpp',
            'KCore/Connect/connectEV2EENbrs.cpp',
            'KCore/Connect/connectEV2FV.cpp',
            'KCore/Connect/connectEV2VF.cpp',
            'KCore/Connect/connectNG2FE.cpp',
            'KCore/Connect/connectNG2EV.cpp',
            'KCore/Connect/connectNG2VF.cpp',
            'KCore/Connect/connectNG2VNbrs.cpp',
            #'KCore/Connect/connectNG2ENbrs.cpp',
            'KCore/Connect/connectFE2EF.cpp',
            'KCore/Connect/connectFE2EENbrs.cpp',
            'KCore/Connect/connectFE2NFace.cpp',
            'KCore/Connect/connectMix2EV.cpp',
            'KCore/Connect/identifyFace.cpp',
            'KCore/Connect/orderBAR.cpp',
            'KCore/Connect/cleanConnectivity.cpp',
            'KCore/Connect/supIdPoints.cpp',
            'KCore/Connect/connect.cpp',
            'KCore/Connect/connectNGons.cpp',
            'KCore/Connect/reorderStruct.cpp',
            'KCore/Connect/reorderUnstruct.cpp',
            'KCore/Connect/reorderNGon.cpp',
            'KCore/Connect/createExtendedCentersMesh.cpp',
            'KCore/Connect/BARSplitter.cpp',
            'KCore/Connect/TSSplitter.cpp',
            'KCore/Connect/indiceStruct2Unstr.cpp',
            'KCore/Connect/indiceFace2Connect.cpp',
            'KCore/Connect/IdTool.cpp',
            'KCore/Connect/MeshTool.cpp',
            'KCore/Sort/sort.cpp',
            'KCore/Loc/node2Center.cpp',
            'KCore/Loc/node2ExtCenters.cpp',
            'KCore/Loc/center2ExtCenters.cpp',
            'KCore/Loc/extCenters2Node.cpp',
            'KCore/Loc/center2Node.cpp',
            'KCore/Loc/fromExtCenters2StdCenters.cpp',
            'KCore/Loc/cart2Cyl.cpp',
            'KCore/Loc/cyl2Cart.cpp',
            'KCore/Linear/solve.cpp',
            'KCore/Linear/prod.cpp',
            'KCore/Linear/inv.cpp',
            'KCore/Linear/DelaunayMath.cpp',
            'KCore/Linear/eigen.cpp',
            'KCore/Linear/cholesky.cpp',
            'KCore/MeshElement/Edge.cpp',
            'KCore/MeshElement/Triangle.cpp',
            'KCore/MeshElement/Quadrangle.cpp',
            'KCore/MeshElement/Tetrahedron.cpp',
            'KCore/MeshElement/Hexahedron.cpp',
            'KCore/MeshElement/Polygon.cpp',
            'KCore/Search/OctreeNode.cpp',
            'KCore/Noise/random.cpp',
            'KCore/Noise/perlin.cpp',
            'KCore/Metric/CompNGonVol.cpp',
            'KCore/Metric/compNGonSurf.cpp',
            'KCore/Metric/compNGonFacesSurf.cpp',
            'KCore/Metric/compVolOfStructCell2D.cpp',
            'KCore/Interp/IntTreeNode.cpp',
            'KCore/Interp/InterpAdt.cpp',
            'KCore/Interp/Interp2.cpp',
            'KCore/Interp/Interp.cpp',
            'KCore/Interp/InterpMLS.cpp',
            'KCore/Fld/ngon_unit.cpp',
            'KCore/Logger/logger.cpp',
            'KCore/Logger/log_to_std_output.cpp',
            'KCore/Logger/log_to_std_error.cpp',
            'KCore/Logger/log_to_file.cpp',
            'KCore/Logger/log_from_distributed_file.cpp',
            ]
if NUGA:
    cpp_srcs += ["KCore/Nuga/Delaunay/MeshUtils1D.cpp",
                 "KCore/Nuga/Delaunay/Triangulator.cpp",
                 #"KCore/Nuga/Delaunay/iodata.cpp",
                 "KCore/Nuga/GapFixer/Imprinter.cpp",
                 "KCore/Nuga/GapFixer/GapFixer.cpp",
                 "KCore/Nuga/GapFixer/FittingBox.cpp",
                 "KCore/Nuga/GapFixer/Plaster.cpp",
                 "KCore/Nuga/GapFixer/GapsManager.cpp",
                 "KCore/Nuga/GapFixer/Intersector.cpp",
                 "KCore/Nuga/GapFixer/NodeAssociator.cpp",
                 "KCore/Nuga/GapFixer/PostNodeAssociator.cpp",
                 "KCore/Nuga/GapFixer/PatchMaker.cpp",
                 "KCore/Nuga/GapFixer/Zipper.cpp",
                 "KCore/Nuga/GapFixer/MergingZipper.cpp",
                 "KCore/Nuga/Boolean/BooleanOperator.cpp",
                 "KCore/Nuga/Boolean/TRI_BooleanOperator.cpp",
                 "KCore/Nuga/Boolean/BAR_BooleanOperator.cpp",
                 "KCore/Nuga/Boolean/SwapperT3.cpp"]
if METIS:
    cpp_srcs += ['KCore/Metis/auxapi.c',
                 'KCore/Metis/b64.c',
                 'KCore/Metis/balance.c',
                 'KCore/Metis/blas.c',
                 'KCore/Metis/bucketsort.c',
                 'KCore/Metis/checkgraph.c',
                 'KCore/Metis/coarsen.c',
                 'KCore/Metis/compress.c',
                 'KCore/Metis/contig.c',
                 'KCore/Metis/csr.c',
                 'KCore/Metis/debug.c',
                 'KCore/Metis/error.c',
                 'KCore/Metis/evaluate.c',
                 'KCore/Metis/fkvkselect.c',
                 'KCore/Metis/fm.c',
                 'KCore/Metis/fortran.c',
                 'KCore/Metis/frename.c',
                 'KCore/Metis/fs.c',
                 #'KCore/Metis/getopt.c',
                 'KCore/Metis/gklib.c',
                 #'KCore/Metis/gkregex.c',
                 'KCore/Metis/gkgraph.c',
                 'KCore/Metis/gkutil.c',
                 'KCore/Metis/graph.c',
                 'KCore/Metis/htable.c',
                 'KCore/Metis/initpart.c',
                 'KCore/Metis/io.c',
                 'KCore/Metis/itemsets.c',
                 'KCore/Metis/kmetis.c',
                 'KCore/Metis/kwayfm.c',
                 'KCore/Metis/kwayrefine.c',
                 'KCore/Metis/mcore.c',
                 'KCore/Metis/mcutil.c',
                 'KCore/Metis/memory.c',
                 'KCore/Metis/mesh.c',
                 'KCore/Metis/meshpart.c',
                 'KCore/Metis/minconn.c',
                 'KCore/Metis/mincover.c',
                 'KCore/Metis/mmd.c',
                 'KCore/Metis/ometis.c',
                 'KCore/Metis/omp.c',
                 'KCore/Metis/options.c',
                 'KCore/Metis/parmetis.c',
                 'KCore/Metis/pdb.c',
                 'KCore/Metis/pmetis.c',
                 'KCore/Metis/pqueue.c',
                 'KCore/Metis/random.c',
                 'KCore/Metis/refine.c',
                 'KCore/Metis/rw.c',
                 'KCore/Metis/separator.c',
                 'KCore/Metis/seq.c',
                 'KCore/Metis/sfm.c',
                 'KCore/Metis/sort.c',
                 'KCore/Metis/srefine.c',
                 'KCore/Metis/stat.c',
                 'KCore/Metis/string.c',
                 'KCore/Metis/timers.c',
                 'KCore/Metis/timing.c',
                 'KCore/Metis/tokenizer.c',
                 'KCore/Metis/util.c',
                 'KCore/Metis/wspace.c'
                 ]

#==============================================================================
# Fichiers fortrans
#==============================================================================
for_srcs = ['KCore/CompGeom/RotateMeshF.for',
            'KCore/CompGeom/CompBBoxOfCellF.for',
            'KCore/CompGeom/CompBoundingBox2F.for',
            'KCore/CompGeom/CompBoundingBoxUnstr2F.for',
            'KCore/CompGeom/CompBoundingBoxUnstrF.for',
            'KCore/CompGeom/CompBoundingBoxF.for',
            'KCore/CompGeom/CompCEBoxF.for',
            'KCore/CompGeom/CompMinDistF.for',
            'KCore/CompGeom/RectifyNormalsF.for',
            'KCore/CompGeom/SlopeF.for',
            'KCore/CompGeom/ParamF.for',
            'KCore/CompGeom/OnedmapF.for',
            'KCore/CompGeom/InterpF.for',
            'KCore/Fld/FldCopyFromF.for',
            'KCore/Fld/FldSetAllBValuesAtF.for',
            'KCore/Fld/FldIntCopyFromF.for',
            'KCore/Fld/FldFortranVecF.for',
            'KCore/Fld/FldAddArrayPartF.for',
            'KCore/Fld/FldSqrtF.for',
            'KCore/Interp/Coord_in_ref_frame_3dF.for',
            'KCore/Interp/Cp_coord_in_stable_frame_3dF.for',
            'KCore/Interp/CompInterpolatedPtInRefElementF.for',
            'KCore/Linear/GaussjF.for',
            'KCore/Loc/Conv2CenterF.for',
            'KCore/Loc/Conv2Center2DF.for',
            'KCore/Loc/Conv2Center1DF.for',
            'KCore/Loc/Conv2NodeF.for',
            'KCore/Loc/Conv2Node2DF.for',
            'KCore/Loc/Conv2Node1DF.for',
            'KCore/Metric/CompTetraCellCenterF.for',
            'KCore/Metric/CompStructCellCenterF.for',
            'KCore/Metric/CompVolOfStructCellF.for',
            'KCore/Metric/CompVolOfTetraCellF.for',
            'KCore/Metric/CompMeanLengthOfCellF.for',
            'KCore/Metric/CompMinLengthOfCellF.for',
            'KCore/Metric/CompIntSurfOfCellF.for',
            'KCore/Metric/CompIntSurfF.for',
            'KCore/Metric/CompStructSurfF.for',
            'KCore/Metric/CompCenterInterfaceF.for',
            'KCore/Metric/CompStructMetricF.for',
            'KCore/Metric/CompUnstrSurfF.for',
            'KCore/Metric/CompUnstrCenterIntF.for',
            'KCore/Metric/CompUnstrMetricF.for',
            'KCore/Metric/CompNormStructSurfF.for',
            'KCore/Metric/CompNormUnstrSurfF.for']
