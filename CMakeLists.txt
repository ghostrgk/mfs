cmake_minimum_required(VERSION 3.10)
project(minifs)

# https://clang.llvm.org/docs/DiagnosticsReference.html
add_compile_options(-Werror -Wall -Wextra -Wpedantic -g -fsanitize=address,leak,undefined)
add_link_options(-Wall -Wextra -Wpedantic -g -fsanitize=address,leak,undefined)
#add_compile_options(-Wall -Wextra -Wpedantic -g)
#add_compile_options(-Werror -Wall -Wextra -Wpedantic -g)

# need to think about coverage and memsan

add_subdirectory(fs++)
add_subdirectory(server)
add_subdirectory(local_app)