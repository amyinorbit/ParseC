add_library(ParseC STATIC parsec.c utf8.c)
target_include_directories(ParseC INTERFACE ${PROJECT_SOURCE_DIR}/include)
install(TARGETS ParseC DESTINATION lib)
