cmake_minimum_required(VERSION 3.16)
project(PlutoSDR CXX) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include_directories(${INCLUDE_DIR})

option(BUILD_GUI "Build SDL2/OpenGL/GLEW + ImGui/ImPlot executable (src/main.cpp)" ON)

# SoapySDR 
find_package(SoapySDR CONFIG QUIET)
if(NOT SoapySDR_FOUND)
  find_package(SoapySDR QUIET)
endif()

if(SoapySDR_FOUND)
  message(STATUS "Found SoapySDR via find_package")
else()
  find_path(SOAPYSDR_INCLUDE_DIR SoapySDR/Device.h
      PATHS /usr/include /usr/local/include)
  find_library(SOAPYSDR_LIBRARY SoapySDR
      PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
endif()

add_executable(new ${SRC_DIR}/new.cpp)

if(SoapySDR_FOUND)
  target_link_libraries(new PRIVATE SoapySDR::SoapySDR)
else()
  if(NOT SOAPYSDR_INCLUDE_DIR OR NOT SOAPYSDR_LIBRARY)
    message(FATAL_ERROR "SoapySDR не найден (ни find_package, ни ручной поиск)")
  endif()
  message(STATUS "Found SoapySDR: ${SOAPYSDR_LIBRARY}")
  message(STATUS "Include dir: ${SOAPYSDR_INCLUDE_DIR}")
  target_include_directories(new PRIVATE ${SOAPYSDR_INCLUDE_DIR})
  target_link_libraries(new PRIVATE ${SOAPYSDR_LIBRARY})
endif()

# GUI target (SDL2 + OpenGL + GLEW + ImGui + ImPlot) 
if(BUILD_GUI)
  # SDL2 / OpenGL / GLEW are optional: if missing, we just skip building 'main'
  find_package(SDL2 QUIET)
  find_package(OpenGL QUIET)
  find_package(GLEW QUIET)

  if(NOT SDL2_FOUND OR NOT OPENGL_FOUND OR NOT GLEW_FOUND)
    message(WARNING "GUI deps not found (need SDL2 + OpenGL + GLEW). Set -DBUILD_GUI=OFF or install dev packages. Skipping 'main' target.")
    return()
  endif()

  # ImGui / ImPlot (submodules in third_party)
  set(IMGUI_SRC
      ${THIRD_PARTY_DIR}/imgui/imgui.cpp
      ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
      ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
      ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
      ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
      ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
      ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
  )

  set(IMPLOT_SRC
      ${THIRD_PARTY_DIR}/implot/implot.cpp
      ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
      ${THIRD_PARTY_DIR}/implot/implot_items.cpp
  )

  if(EXISTS ${THIRD_PARTY_DIR}/imgui/imgui.cpp AND EXISTS ${THIRD_PARTY_DIR}/implot/implot.cpp)
    add_library(imgui STATIC ${IMGUI_SRC})
    target_include_directories(imgui PUBLIC ${THIRD_PARTY_DIR}/imgui)
    target_include_directories(imgui PRIVATE ${THIRD_PARTY_DIR}/imgui/backends ${SDL2_INCLUDE_DIRS})
    target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

    add_library(implot STATIC ${IMPLOT_SRC})
    target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
    target_link_libraries(implot PRIVATE imgui)

    if(NOT MSVC)
      target_compile_options(imgui PRIVATE -fPIC)
      target_compile_options(implot PRIVATE -fPIC)
    endif()

    add_executable(main ${SRC_DIR}/main.cpp)
    target_link_libraries(main PRIVATE imgui implot ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
  else()
    message(WARNING "ImGui/ImPlot submodules not initialized. Run: git submodule update --init --recursive. GUI target 'main' will not be built.")
  endif()
endif()