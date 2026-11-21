cmake_minimum_required(VERSION 3.16)
project(emu8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------------
#  core : le coeur du projet.
#
#  AUCUNE dependance a MFC / Qt / wxWidgets / SDL ici. Si cette
#  bibliotheque compile sans bibliotheque graphique, le critere de
#  validation de l'architecture demande par le sujet est rempli.
# ---------------------------------------------------------------------
add_library(core STATIC
    core/isa.cpp
    core/assembleur.cpp
    core/desassembleur.cpp
    core/processeur.cpp
    core/machine.cpp
)
target_include_directories(core PUBLIC core)

# ---------------------------------------------------------------------
#  emu8asm : assembleur en mode console (texte -> code machine)
# ---------------------------------------------------------------------
add_executable(emu8asm tools/main_asm.cpp)
target_link_libraries(emu8asm PRIVATE core)

# ---------------------------------------------------------------------
#  emu8 : emulateur en mode console (code machine -> execution)
# ---------------------------------------------------------------------
add_executable(emu8 tools/main_console.cpp)
target_link_libraries(emu8 PRIVATE core)

# ---------------------------------------------------------------------
#  emu8gui : interface graphique (a creer avec l'assistant Visual Studio)
#
#  L'application MFC se cree avec l'assistant de Visual Studio, qui
#  genere son propre projet .vcxproj. Il suffit alors d'ajouter les
#  fichiers de core/ a ce projet : aucune ligne du coeur n'est a
#  modifier.
# ---------------------------------------------------------------------
