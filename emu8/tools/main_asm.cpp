// =====================================================================
//  main_asm.cpp  --  Outil console d'assemblage
//
//  Usage :
//     emu8asm fichier.asm [-o sortie.bin] [-s]
//
//     -o <fichier>   ecrit l'image memoire brute (256 octets)
//     -s             affiche la table des symboles
//
//  Sans erreur, affiche le listing : adresse, octets produits, source,
//  puis la relecture desassemblee (verification aller-retour).
// =====================================================================

#include "assembleur.h"
#include "desassembleur.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void afficherUsage()
{
    std::cout << "usage : emu8asm fichier.asm [-o sortie.bin] [-s]\n";
}

std::string hexa(int valeur, int chiffres)
{
    static const char* table = "0123456789ABCDEF";
    std::string r;
    for (int i = chiffres - 1; i >= 0; --i) {
        r += table[(valeur >> (4 * i)) & 0x0F];
    }
    return r;
}

} // namespace

int main(int argc, char* argv[])
{
    std::string source;
    std::string sortie;
    bool        afficherSymboles = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            sortie = argv[++i];
        } else if (arg == "-s") {
            afficherSymboles = true;
        } else if (arg == "-h" || arg == "--help") {
            afficherUsage();
            return 0;
        } else if (source.empty()) {
            source = arg;
        } else {
            std::cerr << "argument inattendu : " << arg << "\n";
            return 2;
        }
    }

    if (source.empty()) {
        afficherUsage();
        return 2;
    }

    CAssembleur        assembleur;
    ResultatAssemblage res = assembleur.assemblerFichier(source);

    // ---- erreurs ----
    if (!res.ok()) {
        for (std::size_t i = 0; i < res.erreurs.size(); ++i) {
            std::cerr << source << ":" << res.erreurs[i].ligne
                      << ": erreur: " << res.erreurs[i].message << "\n";
        }
        std::cerr << res.erreurs.size() << " erreur(s)\n";
        return 1;
    }

    // ---- listing ----
    std::cout << "=== Listing ===\n";
    for (std::size_t i = 0; i < res.listing.size(); ++i) {
        const LigneListing& l = res.listing[i];
        std::string octets;
        for (int k = 0; k < l.nbOctets; ++k) {
            if (k > 0) octets += " ";
            octets += hexa(res.code[l.adresse + k], 2);
        }
        while (octets.size() < 8) octets += " ";

        std::cout << hexa(l.adresse, 4) << "  " << octets << "  "
                  << std::setw(4) << l.ligne << " | " << l.texte << "\n";
    }

    // ---- symboles ----
    if (afficherSymboles) {
        std::cout << "\n=== Symboles ===\n";
        for (std::map<std::string, uint8_t>::const_iterator it =
                 res.symboles.begin();
             it != res.symboles.end(); ++it) {
            std::cout << "  " << std::left << std::setw(16) << it->first
                      << " = 0x" << hexa(it->second, 2) << "\n";
        }
    }

    // ---- relecture desassemblee ----
    std::cout << "\n=== Relecture desassemblee ===\n";
    CDesassembleur desassembleur;
    int adresse = 0;
    while (adresse < static_cast<int>(res.code.size())) {
        if (!res.occupe[adresse]) {
            ++adresse;
            continue;
        }
        int nb = 1;
        std::cout << desassembleur.ligneComplete(res.code, adresse, nb) << "\n";
        adresse += (nb > 0 ? nb : 1);
    }

    // ---- image memoire ----
    if (!sortie.empty()) {
        std::ofstream fichier(sortie.c_str(), std::ios::binary);
        if (!fichier) {
            std::cerr << "impossible d'ecrire '" << sortie << "'\n";
            return 1;
        }
        fichier.write(reinterpret_cast<const char*>(res.code.data()),
                      static_cast<std::streamsize>(res.code.size()));
        std::cout << "\nimage memoire ecrite dans " << sortie << " ("
                  << res.code.size() << " octets)\n";
    }

    return 0;
}
