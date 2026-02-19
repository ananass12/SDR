cmake_minimum_required(VERSION 3.16)
project(PlutoSDR CXX) 

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/10-11/third_party)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/10-11/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/10-11/include)


# Прямой поиск библиотек
find_path(SOAPYSDR_INCLUDE_DIR SoapySDR/Device.h
    PATHS /usr/include /usr/local/include)

find_library(SOAPYSDR_LIBRARY SoapySDR
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

if(NOT SOAPYSDR_INCLUDE_DIR OR NOT SOAPYSDR_LIBRARY)
    message(FATAL_ERROR "SoapySDR не найден")
else()
    message(STATUS "Found SoapySDR: ${SOAPYSDR_LIBRARY}")
    message(STATUS "Include dir: ${SOAPYSDR_INCLUDE_DIR}")
endif()

# Поиск математической библиотеки
find_library(M_LIB m)

# SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# OpenGL/GLEW
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${INCLUDE_DIR})

add_library(imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${THIRD_PARTY_DIR}/imgui)
target_compile_options(imgui PRIVATE -fPIC)

add_library(implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)

target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)

add_executable(new ${SRC_DIR}/new.cpp)

#add_executable(main main.cpp)  # Оставляем .cpp расширение
target_include_directories(new  PRIVATE ${SOAPYSDR_INCLUDE_DIR})
target_link_libraries(new PRIVATE 
    imgui 
    implot 
    ${SDL2_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    ${GLEW_LIBRARIES}
    SoapySDR::SoapySDR
    ${M_LIB}
)
target_compile_options(new  PRIVATE -g -Wall)

# Для C++ может потребоваться явно указать использование C++11
target_compile_features(new PRIVATE cxx_std_11)
