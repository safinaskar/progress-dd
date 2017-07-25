cmake_minimum_required (VERSION 3.1) # 3.1 adds C_STANDARD
project (progress-dd LANGUAGES C)
add_executable (progress-dd progress-dd.c)
set_property (TARGET progress-dd PROPERTY C_STANDARD 99)
target_link_libraries (progress-dd -lm)
install (TARGETS progress-dd DESTINATION bin)
