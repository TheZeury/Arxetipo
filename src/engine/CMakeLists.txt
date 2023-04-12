file (GLOB_RECURSE HPPS *.hpp)
file (GLOB_RECURSE CPPS *.cpp)

add_library (ArxetipoEngine ${HPPS} ${CPPS})