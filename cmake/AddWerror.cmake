function(add_werror target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-implicit-fallthrough -Werror)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # target_compile_options(${target} PRIVATE /W4 /WX) #TODO
  endif()
endfunction()
