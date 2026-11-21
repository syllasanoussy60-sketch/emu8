// =====================================================================
//  desassembleur.h  --  Code machine vers texte lisible
//
//  Utilise par l'interface graphique pour afficher le contenu de la
//  memoire, et par l'outil console pour verifier l'assemblage.
// =====================================================================

#ifndef DESASSEMBLEUR_H
#define DESASSEMBLEUR_H

#include "isa.h"

#include <cstdint>
#include <string>
#include <vector>

class CDesassembleur {
public:
    // Desassemble l'instruction situee a l'adresse indiquee.
    // 'nbOctets' recoit la taille de l'instruction (toujours 1 ou 2).
    // Un opcode inconnu produit "???" et consomme un octet.
    std::string desassembler(const std::vector<uint8_t>& memoire,
                             int adresse,
                             int& nbOctets) const;

    // Ligne complete prete a afficher :
    //   "0004  08 01    LDI R0, 0x01"
    std::string ligneComplete(const std::vector<uint8_t>& memoire,
                              int adresse,
                              int& nbOctets) const;
};

#endif // DESASSEMBLEUR_H
