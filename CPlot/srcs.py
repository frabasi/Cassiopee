import KCore.Dist as Dist
from KCore.config import *

MPEG = False

if MPEG:
    (mpeg, mpegIncDir, mpegLib) = Dist.checkMpeg(additionalLibPaths,
                                                additionalIncludePaths)
else: mpeg = False

# SHADERS=2 (glsl2.0) =4 (glsl4.0)
if CPlotOffScreen >= 1:
    # Il semble qu'OSMesa n'implemente en general pas le glsl4
    SHADERS = 2
else:
    SHADERS = 4
#==============================================================================
# Fichiers C++
#==============================================================================
cpp_srcs = ['CPlot/render.cpp',
            'CPlot/delete.cpp',
            'CPlot/add.cpp',
            'CPlot/replace.cpp',
            'CPlot/createDL.cpp',
            'CPlot/configure.cpp',
            'CPlot/DataInstance.cpp',
            'CPlot/DataDL.cpp',
            'CPlot/displayNew.cpp',
            'CPlot/displayNewFBO.cpp',
            'CPlot/displayNewOSMesa.cpp',
            'CPlot/displayAgain.cpp',
            'CPlot/displayAgainFBO.cpp',
            'CPlot/displayAgainOSMesa.cpp',
            'CPlot/fitView.cpp',
            'CPlot/finalizeExport.cpp',
            'CPlot/getStringsFromPyObj.cpp',
            'CPlot/pressKey.cpp',
            'CPlot/getAllVars.cpp',
            'CPlot/get.cpp',
            'CPlot/set.cpp',
            'CPlot/setFileName.cpp',
            'CPlot/getFileExt.cpp',
            'CPlot/Data.cpp',
            'CPlot/initZoneData.cpp',
            'CPlot/initZoneDataDL.cpp',
            'CPlot/createZones.cpp',
            'CPlot/codeFromRenderTag.cpp',
            'CPlot/StructZone.cpp',
            'CPlot/UnstructZone.cpp',
            'CPlot/Zone.cpp',
            'CPlot/ZoneImplDL.cpp',
            'CPlot/findMinMax.cpp',
            'CPlot/compSNorm.cpp',
            'CPlot/compUNorm.cpp',
            'CPlot/openGfx.cpp',
            'CPlot/keyboard.cpp',
            'CPlot/mouse.cpp',
            'CPlot/hide.cpp',
            'CPlot/Slot1D.cpp',
            'CPlot/Zone1D.cpp',
            'CPlot/display1D.cpp',
            'CPlot/Textures/noise.cpp',
            'CPlot/Textures/createTextures.cpp',
            'CPlot/Textures/createVoxelTexture.cpp',
            'CPlot/Textures/createColormapTexture.cpp',
            'CPlot/Display/display.cpp',
            'CPlot/Display/reshape.cpp',
            'CPlot/Display/displayBB.cpp',
            'CPlot/Display/fog.cpp',
            'CPlot/Display/setCursor.cpp',
            'CPlot/Display/light.cpp',
            'CPlot/Display/roll.cpp',
            'CPlot/Display/dist2BB.cpp',
            'CPlot/Display/computeSteps.cpp',
            'CPlot/Display/frustum.cpp',
            'CPlot/Display/displayActivePoint.cpp',
            'CPlot/Display/displayFrameTex.cpp',
            'CPlot/Display/displayAnaglyph.cpp',
            'CPlot/Display/activateZone.cpp',
            'CPlot/Display/displaySEdges.cpp',
            'CPlot/Display/displayUEdges.cpp',
            'CPlot/Display/displaySMesh.cpp',
            'CPlot/Display/displaySMeshZone.cpp',
            'CPlot/Display/displayUMesh.cpp',
            'CPlot/Display/displayUMeshZone.cpp',
            'CPlot/Display/displayUMeshHOZone.cpp',            
            'CPlot/Display/displaySSolid.cpp',
            'CPlot/Display/displaySSolidZone.cpp',
            'CPlot/Display/displayUSolid.cpp',
            'CPlot/Display/displayUSolidZone.cpp',
            'CPlot/Display/displayUSolidHOZone.cpp',
            'CPlot/Display/displayText.cpp',
            'CPlot/Display/displaySIsoSolid.cpp',
            'CPlot/Display/displaySIsoSolidZone.cpp',
            'CPlot/Display/displayUIsoSolid.cpp',
            'CPlot/Display/displayUIsoSolidZone.cpp',
            'CPlot/Display/createDLSMeshZone.cpp',
            'CPlot/Display/renderDLSMeshZone.cpp',
            'CPlot/Display/createDLUMeshZone.cpp',
            'CPlot/Display/createDLUMeshHOZone.cpp',
            'CPlot/Display/renderDLUMeshZone.cpp',
            'CPlot/Display/createDLSSolidZone.cpp',
            'CPlot/Display/renderDLSSolidZone.cpp',
            'CPlot/Display/createDLUSolidZone.cpp',
            'CPlot/Display/createDLUSolidHOZone.cpp',
            'CPlot/Display/renderDLUSolidZone.cpp',
            'CPlot/Display/renderDLUSolidHOZone.cpp',
            'CPlot/Display/createDLSIsoSolidZone.cpp',
            'CPlot/Display/renderDLSIsoSolidZone.cpp',
            'CPlot/Display/createDLUIsoSolidZone.cpp',
            'CPlot/Display/renderDLUIsoSolidZone.cpp',
            'CPlot/Display/displayPlot.cpp',
            'CPlot/Display/menu.cpp',
            'CPlot/Display/displayIsoLegend.cpp',
            'CPlot/Display/displayAxis.cpp',
            'CPlot/Display/displayNodes.cpp',
            'CPlot/Display/displayBillBoards.cpp',
            'CPlot/Display/displayAllBillBoards.cpp',
            'CPlot/Plugins/loadPlugins.cpp',
            'CPlot/Plugins/checkVariable.cpp',
            'CPlot/Plugins/findBlankedZones.cpp',
            'CPlot/Plugins/blanking.cpp',
            'CPlot/Plugins/colormaps.cpp',
            'CPlot/Plugins/select.cpp',
            'CPlot/Plugins/lookfor.cpp',
            'CPlot/Plugins/gl2ps.cpp',
            'CPlot/Plugins/screenDump.cpp',
            'CPlot/Plugins/writePPMFile.cpp',
            'CPlot/Plugins/imagePost.cpp',
            'CPlot/Plugins/mouseClick.cpp',

            'CPlot/Fonts/OpenGLText.cpp',
            
            'CPlot/GLEW/glew.c',
            'CPlot/GLUT/freeglut_callbacks.c',
            'CPlot/GLUT/freeglut_cursor.c',
            'CPlot/GLUT/freeglut_display.c',
            'CPlot/GLUT/freeglut_ext.c',
            'CPlot/GLUT/freeglut_font.c',
            'CPlot/GLUT/freeglut_font_data.c',
            'CPlot/GLUT/freeglut_gamemode.c',
            'CPlot/GLUT/freeglut_geometry.c',
            'CPlot/GLUT/freeglut_glutfont_definitions.c',
            'CPlot/GLUT/freeglut_init.c',
            'CPlot/GLUT/freeglut_input_devices.c',
            'CPlot/GLUT/freeglut_joystick.c',
            'CPlot/GLUT/freeglut_main.c',
            'CPlot/GLUT/freeglut_menu.c',
            'CPlot/GLUT/freeglut_misc.c',
            'CPlot/GLUT/freeglut_overlay.c',
            'CPlot/GLUT/freeglut_spaceball.c',
            'CPlot/GLUT/freeglut_state.c',
            'CPlot/GLUT/freeglut_stroke_mono_roman.c',
            'CPlot/GLUT/freeglut_stroke_roman.c',
            'CPlot/GLUT/freeglut_structure.c',
            'CPlot/GLUT/freeglut_teapot.c',
            'CPlot/GLUT/freeglut_videoresize.c',
            'CPlot/GLUT/freeglut_window.c',
            'CPlot/GLUT/freeglut_xinput.c']

if SHADERS==2:
    cpp_srcs += ['CPlot/Shaders2.0/triggerShader.cpp',
                 'CPlot/Shaders2.0/FragmentShader.cpp',
                 'CPlot/Shaders2.0/VertexShader.cpp',
                 'CPlot/Shaders2.0/GeomShader.cpp',
                 'CPlot/Shaders2.0/ShaderObject.cpp',
                 'CPlot/Shaders2.0/Shader.cpp',
                 'CPlot/Shaders2.0/ShaderManager.cpp']
else:
    cpp_srcs += ['CPlot/Shaders/TesselationShaderManager.cpp',
                 'CPlot/Shaders/TesselationControlShader.cpp',
                 'CPlot/Shaders/TesselationEvaluationShader.cpp',
                 'CPlot/Shaders/triggerShader.cpp',
                 'CPlot/Shaders/FragmentShader.cpp',
                 'CPlot/Shaders/VertexShader.cpp',
                 'CPlot/Shaders/GeomShader.cpp',
                 'CPlot/Shaders/ShaderObject.cpp',
                 'CPlot/Shaders/Shader.cpp',
                 'CPlot/Shaders/ShaderManager.cpp']
# png
cpp_srcs += ["CPlot/Plugins/writePNGFile.cpp",
             "CPlot/Textures/createPngTexture.cpp"]

if mpeg:
    cpp_srcs += ["CPlot/Plugins/writeMPEGFrame.cpp"]
else:
    cpp_srcs += ["CPlot/Plugins/writeMPEGFrame_stub.cpp"]

#==============================================================================
# Fichiers fortran
#==============================================================================
for_srcs = []
