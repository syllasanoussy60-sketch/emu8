#include "desassembleur.h"

namespace {

std::string hexa(int valeur, int chiffres)
{
    static const char* table = "0123456789ABCDEF";
    std::string r;
    for (int i = chiffres - 1; i >= 0; --i) {
        r += table[(valeur >> (4 * i)) & 0x0F];
    }
    return r;
}

std::string registre(uint8_t n)
{
    return std::string("R") + static_cast<char>('0' + (n & 0x07));
}

} // namespace

std::string CDesassembleur::desassembler(const std::vector<uint8_t>& memoire,
                                         int adresse, int& nbOctets) const
{
    nbOctets = 1;
    if (adresse < 0 || adresse >= static_cast<int>(memoire.size())) {
        return "???";
    }

    const uint8_t octet1 = memoire[adresse];
    const isa::DescInstruction* d = isa::trouverParOpcode(octet1);
    if (d == nullptr) {
        return "??? (0x" + hexa(octet1, 2) + ")";
    }

    nbOctets = d->taille;
    const uint8_t reg = static_cast<uint8_t>(octet1 & isa::MASQUE_REGISTRE);

    uint8_t octet2 = 0;
    if (d->taille == 2) {
        if (adresse + 1 >= static_cast<int>(memoire.size())) {
            nbOctets = 1;
            return "??? (instruction tronquee)";
        }
        octet2 = memoire[adresse + 1];
    }

    std::string texte = d->mnemonique;
    switch (d->format) {
    case isa::Format::Aucun:
        break;
    case isa::Format::RegImm:
        texte += " " + registre(reg) + ", 0x" + hexa(octet2, 2);
        break;
    case isa::Format::RegAdr:
        texte += " " + registre(reg) + ", [0x" + hexa(octet2, 2) + "]";
        break;
    case isa::Format::RegReg:
        texte += " " + registre(reg) + ", " + registre(octet2);
        break;
    case isa::Format::Adr:
        texte += " 0x" + hexa(octet2, 2);
        break;
    }
    return texte;
}

std::string CDesassembleur::ligneComplete(const std::vector<uint8_t>& memoire,
                                          int adresse, int& nbOctets) const
{
    const std::string texte = desassembler(memoire, adresse, nbOctets);

    std::string octets;
    for (int i = 0; i < nbOctets; ++i) {
        if (adresse + i < static_cast<int>(memoire.size())) {
            if (i > 0) octets += " ";
            octets += hexa(memoire[adresse + i], 2);
        }
    }
    while (octets.size() < 6) {
        octets += " ";
    }

    return hexa(adresse, 4) + "  " + octets + "  " + texte;
}
