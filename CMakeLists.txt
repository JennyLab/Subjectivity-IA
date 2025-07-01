cmake_minimum_required(VERSION 3.10)
project(SubjectivityIA CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# Busca todos los .cpp en src/
file(GLOB SOURCES "Subjectivity.test001.cpp")

add_executable(main ${SOURCES})

# Si tienes dependencias externas, agrégalas aquí, por ejemplo:
# find_package(OpenCV REQUIRED)
# target_link_libraries(main PRIVATE ${OpenCV_LIBS})
