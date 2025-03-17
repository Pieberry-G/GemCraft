cmake_minimum_required(VERSION 3.10.0)
 
add_subdirectory(polyscope)
add_subdirectory(spdlog)
add_definitions(-DTINYGLTF_NO_INCLUDE_STB_IMAGE)
add_definitions(-DTINYGLTF_NO_STB_IMAGE)
add_definitions(-DTINYGLTF_NO_STB_IMAGE_WRITE)
add_subdirectory(tinygltf)
add_subdirectory(tinyobjloader)

set_target_properties(glad PROPERTIES FOLDER "Dependencies")
set_target_properties(imgui PROPERTIES FOLDER "Dependencies")
set_target_properties(polyscope PROPERTIES FOLDER "Dependencies")
set_target_properties(spdlog PROPERTIES FOLDER "Dependencies")
set_target_properties(stb PROPERTIES FOLDER "Dependencies")
set_target_properties(tinygltf PROPERTIES FOLDER "Dependencies")
set_target_properties(loader_example PROPERTIES FOLDER "Dependencies")
set_target_properties(tinyobjloader PROPERTIES FOLDER "Dependencies")
set_target_properties(uninstall PROPERTIES FOLDER "Dependencies")

### PCL
add_library(pcl_deps INTERFACE)
target_link_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/lib")
target_link_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/Boost/lib")
target_link_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/FLANN/lib")
target_link_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/OpenNI2/Lib")
target_link_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/Qhull/lib")
target_link_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/VTK/lib")

target_link_libraries(pcl_deps INTERFACE
    OpenNI2.lib
    pcl_common.lib
    pcl_features.lib
    pcl_filters.lib
    pcl_io.lib
    pcl_io_ply.lib
    pcl_kdtree.lib
    pcl_keypoints.lib
    pcl_ml.lib
    pcl_octree.lib
    pcl_outofcore.lib
    pcl_people.lib
    pcl_recognition.lib
    pcl_registration.lib
    pcl_sample_consensus.lib
    pcl_search.lib
    pcl_segmentation.lib
    pcl_stereo.lib
    pcl_surface.lib
    pcl_tracking.lib
    pcl_visualization.lib
    vtkcgns-9.3.lib
    vtkChartsCore-9.3.lib
    vtkCommonColor-9.3.lib
    vtkCommonComputationalGeometry-9.3.lib
    vtkCommonCore-9.3.lib
    vtkCommonDataModel-9.3.lib
    vtkCommonExecutionModel-9.3.lib
    vtkCommonMath-9.3.lib
    vtkCommonMisc-9.3.lib
    vtkCommonSystem-9.3.lib
    vtkCommonTransforms-9.3.lib
    vtkDICOMParser-9.3.lib
    vtkDomainsChemistry-9.3.lib
    vtkDomainsChemistryOpenGL2-9.3.lib
    vtkdoubleconversion-9.3.lib
    vtkexodusII-9.3.lib
    vtkexpat-9.3.lib
    vtkFiltersAMR-9.3.lib
    vtkFiltersCellGrid-9.3.lib
    vtkFiltersCore-9.3.lib
    vtkFiltersExtraction-9.3.lib
    vtkFiltersFlowPaths-9.3.lib
    vtkFiltersGeneral-9.3.lib
    vtkFiltersGeneric-9.3.lib
    vtkFiltersGeometry-9.3.lib
    vtkFiltersGeometryPreview-9.3.lib
    vtkFiltersHybrid-9.3.lib
    vtkFiltersHyperTree-9.3.lib
    vtkFiltersImaging-9.3.lib
    vtkFiltersModeling-9.3.lib
    vtkFiltersParallel-9.3.lib
    vtkFiltersParallelImaging-9.3.lib
    vtkFiltersPoints-9.3.lib
    vtkFiltersProgrammable-9.3.lib
    vtkFiltersReduction-9.3.lib
    vtkFiltersSelection-9.3.lib
    vtkFiltersSMP-9.3.lib
    vtkFiltersSources-9.3.lib
    vtkFiltersStatistics-9.3.lib
    vtkFiltersTensor-9.3.lib
    vtkFiltersTexture-9.3.lib
    vtkFiltersTopology-9.3.lib
    vtkFiltersVerdict-9.3.lib
    vtkfmt-9.3.lib
    vtkfreetype-9.3.lib
    vtkGeovisCore-9.3.lib
    vtkgl2ps-9.3.lib
    vtkglew-9.3.lib
    vtkhdf5-9.3.lib
    vtkhdf5_hl-9.3.lib
    vtkImagingColor-9.3.lib
    vtkImagingCore-9.3.lib
    vtkImagingFourier-9.3.lib
    vtkImagingGeneral-9.3.lib
    vtkImagingHybrid-9.3.lib
    vtkImagingMath-9.3.lib
    vtkImagingMorphological-9.3.lib
    vtkImagingSources-9.3.lib
    vtkImagingStatistics-9.3.lib
    vtkImagingStencil-9.3.lib
    vtkInfovisCore-9.3.lib
    vtkInfovisLayout-9.3.lib
    vtkInteractionImage-9.3.lib
    vtkInteractionStyle-9.3.lib
    vtkInteractionWidgets-9.3.lib
    vtkIOAMR-9.3.lib
    vtkIOAsynchronous-9.3.lib
    vtkIOCellGrid-9.3.lib
    vtkIOCesium3DTiles-9.3.lib
    vtkIOCGNSReader-9.3.lib
    vtkIOChemistry-9.3.lib
    vtkIOCityGML-9.3.lib
    vtkIOCONVERGECFD-9.3.lib
    vtkIOCore-9.3.lib
    vtkIOEnSight-9.3.lib
    vtkIOExodus-9.3.lib
    vtkIOExport-9.3.lib
    vtkIOExportGL2PS-9.3.lib
    vtkIOExportPDF-9.3.lib
    vtkIOFLUENTCFF-9.3.lib
    vtkIOGeometry-9.3.lib
    vtkIOHDF-9.3.lib
    vtkIOImage-9.3.lib
    vtkIOImport-9.3.lib
    vtkIOInfovis-9.3.lib
    vtkIOIOSS-9.3.lib
    vtkIOLegacy-9.3.lib
    vtkIOLSDyna-9.3.lib
    vtkIOMINC-9.3.lib
    vtkIOMotionFX-9.3.lib
    vtkIOMovie-9.3.lib
    vtkIONetCDF-9.3.lib
    vtkIOOggTheora-9.3.lib
    vtkIOParallel-9.3.lib
    vtkIOParallelXML-9.3.lib
    vtkIOPLY-9.3.lib
    vtkIOSegY-9.3.lib
    vtkIOSQL-9.3.lib
    vtkioss-9.3.lib
    vtkIOTecplotTable-9.3.lib
    vtkIOVeraOut-9.3.lib
    vtkIOVideo-9.3.lib
    vtkIOXML-9.3.lib
    vtkIOXMLParser-9.3.lib
    vtkjpeg-9.3.lib
    vtkjsoncpp-9.3.lib
    vtkkissfft-9.3.lib
    vtklibharu-9.3.lib
    vtklibproj-9.3.lib
    vtklibxml2-9.3.lib
    vtkloguru-9.3.lib
    vtklz4-9.3.lib
    vtklzma-9.3.lib
    vtkmetaio-9.3.lib
    vtknetcdf-9.3.lib
    vtkogg-9.3.lib
    vtkParallelCore-9.3.lib
    vtkParallelDIY-9.3.lib
    vtkpng-9.3.lib
    vtkpugixml-9.3.lib
    vtkRenderingAnnotation-9.3.lib
    vtkRenderingCellGrid-9.3.lib
    vtkRenderingContext2D-9.3.lib
    vtkRenderingContextOpenGL2-9.3.lib
    vtkRenderingCore-9.3.lib
    vtkRenderingFreeType-9.3.lib
    vtkRenderingGL2PSOpenGL2-9.3.lib
    vtkRenderingHyperTreeGrid-9.3.lib
    vtkRenderingImage-9.3.lib
    vtkRenderingLabel-9.3.lib
    vtkRenderingLICOpenGL2-9.3.lib
    vtkRenderingLOD-9.3.lib
    vtkRenderingOpenGL2-9.3.lib
    vtkRenderingSceneGraph-9.3.lib
    vtkRenderingUI-9.3.lib
    vtkRenderingVolume-9.3.lib
    vtkRenderingVolumeOpenGL2-9.3.lib
    vtkRenderingVtkJS-9.3.lib
    vtksqlite-9.3.lib
    vtksys-9.3.lib
    vtkTestingRendering-9.3.lib
    vtktheora-9.3.lib
    vtktiff-9.3.lib
    vtkverdict-9.3.lib
    vtkViewsContext2D-9.3.lib
    vtkViewsCore-9.3.lib
    vtkViewsInfovis-9.3.lib
    vtkWrappingTools-9.3.lib
    vtkzlib-9.3.lib
)

target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/include/pcl-1.14")
target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/Boost/include/boost-1_84")
target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/Eigen3/include/eigen3")
target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/FLANN/include")
target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/OpenNI2/Include")
target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/Qhull/include")
target_include_directories(pcl_deps INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/VTK/include/vtk-9.3")

set(VTK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/VTK/lib/cmake/vtk-9.3")
find_package(VTK REQUIRED)
target_link_libraries(pcl_deps INTERFACE ${VTK_LIBRARIES})
target_include_directories(pcl_deps INTERFACE ${VTK_INCLUDE_DIRS})

set(PCL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/cmake")
find_package(PCL REQUIRED)
target_link_libraries(pcl_deps INTERFACE ${PCL_LIBRARIES})
target_include_directories(pcl_deps INTERFACE ${PCL_INCLUDE_DIRS})

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/OpenNI2/Redist/OpenNI2.dll" DESTINATION "${CMAKE_BINARY_DIR}/bin/release")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/PCL 1.14.0/3rdParty/OpenNI2/Redist/OpenNI2.dll" DESTINATION "${CMAKE_BINARY_DIR}/bin/debug")

### CGAL
set(BOOST_INCLUDEDIR "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/boost/include")
set(BOOST_LIBRARYDIR "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/boost/lib")
set(GMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/auxiliary/gmp/include")
set(GMP_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/auxiliary/gmp/lib/gmp.lib")
set(MPFR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/auxiliary/gmp/include")
set(MPFR_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/auxiliary/gmp/lib/mpfr.lib")

set(GMP_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/auxiliary/gmp/bin/gmp-10.dll")
file(COPY ${GMP_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
file(COPY ${GMP_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(MPFR_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cgal-deps/auxiliary/gmp/bin/mpfr-6.dll")
file(COPY ${MPFR_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
file(COPY ${MPFR_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)

set(CGAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cgal")
find_package(CGAL REQUIRED)
add_library(cgal_deps INTERFACE)
target_link_libraries(cgal_deps INTERFACE ${CGAL_LIBRARIES})
target_include_directories(cgal_deps INTERFACE ${CGAL_INCLUDE_DIRS})