add_library(ParseC STATIC parsec.c utf8.c)
install(TARGETS ParseC DESTINATION lib)
