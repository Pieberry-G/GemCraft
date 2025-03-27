cmake_minimum_required(VERSION 3.13.0)

if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

### Project
set(PROJECT_TARGET GemCraft)
project(${PROJECT_TARGET})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SRCS "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_executable(${PROJECT_TARGET} "${SRCS}" "${HEADERS}")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_TARGET})
target_precompile_headers(${PROJECT_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h)

target_compile_definitions(${PROJECT_TARGET} PRIVATE GC_PLATFORM_WINDOWS)
target_compile_definitions(${PROJECT_TARGET} PRIVATE "$<$<CONFIG:DEBUG>:GC_DEBUG>")
target_compile_definitions(${PROJECT_TARGET} PRIVATE "$<$<CONFIG:RELEASE>:GC_NDEBUG>")
 
add_subdirectory(deps)
target_link_libraries(${PROJECT_TARGET} polyscope spdlog tinygltf tinyobjloader)
target_link_libraries(${PROJECT_TARGET} pcl_deps cgal_deps)
target_include_directories(${PROJECT_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/glad/include")
target_include_directories(${PROJECT_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/stb")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/config/.polyscope.ini" DESTINATION "${CMAKE_BINARY_DIR}")
