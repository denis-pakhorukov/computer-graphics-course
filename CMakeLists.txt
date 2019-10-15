cmake_minimum_required(VERSION 3.14)
project(computer_graphics)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(lab1_task1 lab1/task1.cpp)
add_executable(lab1_task2 lab1/task2.cpp)
add_executable(lab1_task3 lab1/task3.cpp)
add_executable(lab1_task4 lab1/task4.cpp)

include_directories(${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS})

target_link_libraries(lab1_task1 ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(lab1_task2 ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(lab1_task3 ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(lab1_task4 ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
