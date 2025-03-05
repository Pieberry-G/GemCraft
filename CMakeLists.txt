cmake_minimum_required(VERSION 3.10.0)

### Project
project(GemCraft)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SRCS "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_executable(GemCraft "${SRCS}" "${HEADERS}")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GemCraft)
target_precompile_headers(GemCraft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h)

target_compile_definitions(GemCraft PRIVATE GC_PLATFORM_WINDOWS)
target_compile_definitions(GemCraft PRIVATE "$<$<CONFIG:DEBUG>:GC_DEBUG>")
target_compile_definitions(GemCraft PRIVATE "$<$<CONFIG:RELEASE>:GC_NDEBUG>")

### Dependencies
add_subdirectory(deps)
target_link_libraries(GemCraft geometry-central polyscope spdlog tinygltf tinyobjloader)
target_include_directories(GemCraft PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(GemCraft PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/glad/include")
target_include_directories(GemCraft PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/stb")

### CGAL
set(BOOST_INCLUDEDIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/boost/include")
set(BOOST_LIBRARYDIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/boost/lib")
set(GMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/auxiliary/gmp/include")
set(GMP_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/auxiliary/gmp/lib/gmp.lib")
set(MPFR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/auxiliary/gmp/include")
set(MPFR_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/auxiliary/gmp/lib/mpfr.lib")

set(GMP_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/auxiliary/gmp/bin/gmp-10.dll")
file(COPY ${GMP_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
file(COPY ${GMP_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(MPFR_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal-deps/auxiliary/gmp/bin/mpfr-6.dll")
file(COPY ${MPFR_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
file(COPY ${MPFR_DLL_PATH} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)

set(CGAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/cgal")
find_package(CGAL REQUIRED)
target_link_libraries(GemCraft ${CGAL_LIBRARIES})
target_include_directories(GemCraft PUBLIC ${CGAL_INCLUDE_DIRS})

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/config/.polyscope.ini" DESTINATION "${CMAKE_BINARY_DIR}")
