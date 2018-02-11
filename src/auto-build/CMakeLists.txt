
add_custom_command(
    OUTPUT gitversion.cpp
    PRE_BUILD
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/script.sh
)

ADD_LIBRARY(auto-build gitversion.cpp)
