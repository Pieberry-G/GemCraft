cmake_minimum_required(VERSION 3.10.0)

add_subdirectory(geometry-central)
add_subdirectory(polyscope)
add_subdirectory(spdlog)

set_target_properties(geometry-central PROPERTIES FOLDER "Dependencies")
set_target_properties(glad PROPERTIES FOLDER "Dependencies")
set_target_properties(imgui PROPERTIES FOLDER "Dependencies")
set_target_properties(polyscope PROPERTIES FOLDER "Dependencies")
set_target_properties(spdlog PROPERTIES FOLDER "Dependencies")
set_target_properties(stb PROPERTIES FOLDER "Dependencies")
