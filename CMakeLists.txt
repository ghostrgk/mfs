cmake_minimum_required(VERSION 3.10)

project(mfs)

# https://clang.llvm.org/docs/DiagnosticsReference.html
add_compile_options(-Werror -Wall -Wextra -Wpedantic -g -fsanitize=address,leak,undefined)
add_link_options(-Wall -Wextra -Wpedantic -g -fsanitize=address,leak,undefined)
#add_compile_options(-Wall -Wextra -Wpedantic -g)
#add_compile_options(-Werror -Wall -Wextra -Wpedantic -g)

# need to think about coverage and memsan

add_subdirectory(libs)
add_subdirectory(local_app)


add_subdirectory(client)
add_subdirectory(simple_server)