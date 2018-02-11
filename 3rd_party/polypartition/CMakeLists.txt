
FILE(GLOB _SOURCES *.cpp)
FILE(GLOB _HEADERS *.hpp)
SOURCE_GROUP("Header Files" FILES ${_HEADERS})

ADD_LIBRARY(extern_polypartition ${_SOURCES} ${_HEADERS})

