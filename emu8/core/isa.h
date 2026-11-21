// =====================================================================
//  isa.h  --  Definition du jeu d'instructions et de la carte memoire
//
//  CONTRAT COMMUN AUX DEUX GROUPES.
//  Ce fichier est la reference partagee entre l'assembleur et le
//  processeur. Toute modification doit etre validee par les deux
//  groupes, sinon le code machine produit d'un cote ne sera pas
//  interprete correctement de l'autre.
// =====================================================================

#ifndef ISA_H
#define ISA_H

#include <cstdint>
#include <cstddef>

namespace isa {

// ---------------------------------------------------------------------
//  Memoire
// ---------------------------------------------------------------------
constexpr std::size_t TAILLE_MEMOIRE = 256;

constexpr int VECTEUR_RESET        = 0x00;  // le PC demarre ici
constexpr int VECTEUR_INTERRUPTION = 0x02;  // saut sur interruption
constexpr int FIN_ZONE_PROGRAMME   = 0x7F;
constexpr int ADR_PORT_SORTIE      = 0x80;  // 8 LED
constexpr int ADR_PORT_ENTREE      = 0x81;  // 8 interrupteurs
constexpr int ADR_TIMER_VALEUR     = 0x82;
constexpr int ADR_TIMER_CONTROLE   = 0x83;  // bit0 = marche, bit1 = IT autorisee
constexpr int ADR_TIMER_PERIODE    = 0x84;  // N cycles
constexpr int ADR_UART_DONNEE      = 0x85;  // ecriture = emission d'un caractere
constexpr int ADR_UART_ETAT        = 0x86;  // bit0 = pret a emettre
constexpr int DEBUT_RAM            = 0x90;
constexpr int DEBUT_PILE           = 0xF0;
constexpr int SP_INITIAL           = 0xFF;  // la pile descend

// ---------------------------------------------------------------------
//  Registre d'etat
// ---------------------------------------------------------------------
constexpr uint8_t FLAG_Z = 0x01;  // resultat nul
constexpr uint8_t FLAG_C = 0x02;  // retenue / emprunt
constexpr uint8_t FLAG_N = 0x04;  // bit 7 du resultat
constexpr uint8_t FLAG_I = 0x08;  // interruptions autorisees

// ---------------------------------------------------------------------
//  Encodage
//
//  Octet 1 : OOOOO rrr   (5 bits d'opcode, 3 bits de registre)
//  Octet 2 : donnee immediate, adresse, ou numero de registre source
//
//  Toutes les instructions font donc 1 ou 2 octets.
// ---------------------------------------------------------------------
constexpr uint8_t MASQUE_OPCODE   = 0xF8;
constexpr uint8_t MASQUE_REGISTRE = 0x07;

enum class Format {
    Aucun,    // NOP, RET, RETI, HALT          -> 0 operande
    RegImm,   // LDI Rd, k                     -> 2 operandes
    RegAdr,   // LD Rd, adr   /  ST Rs, adr    -> 2 operandes
    RegReg,   // MOV/ADD/SUB/AND/OR/CMP Rd, Rs -> 2 operandes
    Adr       // JMP/JZ/JNZ/CALL adr           -> 1 operande
};

struct DescInstruction {
    const char* mnemonique;
    uint8_t     opcode;   // base de l'opcode (bits 7..3)
    Format      format;
    uint8_t     taille;   // en octets
    uint8_t     cycles;   // duree d'execution
};

inline constexpr DescInstruction TABLE[] = {
    //  mnemo   opcode  format            taille cycles
    { "NOP",    0x00, Format::Aucun,  1, 1 },
    { "LDI",    0x08, Format::RegImm, 2, 1 },
    { "LD",     0x10, Format::RegAdr, 2, 2 },
    { "ST",     0x18, Format::RegAdr, 2, 2 },
    { "MOV",    0x20, Format::RegReg, 2, 1 },
    { "ADD",    0x28, Format::RegReg, 2, 1 },
    { "SUB",    0x30, Format::RegReg, 2, 1 },
    { "AND",    0x38, Format::RegReg, 2, 1 },
    { "OR",     0x40, Format::RegReg, 2, 1 },
    { "CMP",    0x48, Format::RegReg, 2, 1 },
    { "JMP",    0x50, Format::Adr,    2, 2 },
    { "JZ",     0x58, Format::Adr,    2, 2 },
    { "JNZ",    0x60, Format::Adr,    2, 2 },
    { "CALL",   0x68, Format::Adr,    2, 3 },
    { "RET",    0x70, Format::Aucun,  1, 3 },
    { "RETI",   0x78, Format::Aucun,  1, 3 },
    { "JC",     0x80, Format::Adr,    2, 2 },   // version 2 du contrat
    { "JN",     0x88, Format::Adr,    2, 2 },   // version 2 du contrat
    { "HALT",   0xF8, Format::Aucun,  1, 1 },
};

constexpr std::size_t NB_INSTRUCTIONS = sizeof(TABLE) / sizeof(TABLE[0]);

// Recherche par mnemonique (insensible a la casse). nullptr si inconnu.
const DescInstruction* trouverParMnemonique(const char* mnemo);

// Recherche par octet lu en memoire (les 3 bits de poids faible sont
// ignores). nullptr si l'opcode ne correspond a aucune instruction.
const DescInstruction* trouverParOpcode(uint8_t octet);

// Nombre d'operandes attendus pour un format donne.
constexpr std::size_t nbOperandes(Format f) {
    return (f == Format::Aucun) ? 0u : ((f == Format::Adr) ? 1u : 2u);
}

} // namespace isa

#endif // ISA_H
