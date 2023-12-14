cmake_minimum_required(VERSION 3.26)
project(exploration_securite C)

set(CMAKE_C_STANDARD 11)
add_executable(exploration_securite
        src/com/connexion/connexion.c
        src/main.c
        src/main.c
        src/com/com/com.c
        src/com/protocole/protocole.c)

target_compile_options(exploration_securite PRIVATE "-Wall" "-Wextra")  # Exemple d'options de compilation

find_package(OpenSSL REQUIRED)
target_link_libraries(exploration_securite PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(exploration_securite PRIVATE m rt pthread)

