#include "machine.h"

#include <cctype>
#include <fstream>
#include <sstream>

// =====================================================================
//  Outils locaux
// =====================================================================
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

std::string binaire8(uint8_t valeur)
{
    std::string r;
    for (int i = 7; i >= 0; --i) {
        r += ((valeur >> i) & 1) ? '1' : '0';
    }
    return r;
}

bool chiffreHexa(char c, int& valeur)
{
    if (c >= '0' && c <= '9') { valeur = c - '0';        return true; }
    if (c >= 'a' && c <= 'f') { valeur = c - 'a' + 10;   return true; }
    if (c >= 'A' && c <= 'F') { valeur = c - 'A' + 10;   return true; }
    return false;
}

std::string extension(const std::string& chemin)
{
    const std::size_t p = chemin.find_last_of('.');
    if (p == std::string::npos) return "";
    std::string ext = chemin.substr(p + 1);
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}

} // namespace

const char* texteArret(Arret a)
{
    switch (a) {
    case Arret::Halt:         return "HALT";
    case Arret::PointArret:   return "point d'arret";
    case Arret::LimiteCycles: return "limite de cycles atteinte";
    case Arret::Erreur:       return "erreur d'execution";
    }
    return "?";
}

// =====================================================================
//  Construction : on branche les peripheriques sur le bus.
//  Le processeur, lui, ne voit que le bus.
// =====================================================================
CMachine::CMachine() : cpu_(bus_)
{
    bus_.ajouterPeripherique(&portSortie_);
    bus_.ajouterPeripherique(&portEntree_);
    bus_.ajouterPeripherique(&timer_);
    bus_.ajouterPeripherique(&uart_);
    reset();
}

// =====================================================================
//  Chargement
// =====================================================================

bool CMachine::chargerImage(const std::vector<uint8_t>& octets, uint8_t adresse)
{
    if (octets.size() > isa::TAILLE_MEMOIRE - adresse) {
        derniereErreur_ = "le programme depasse la memoire disponible";
        return false;
    }
    for (std::size_t i = 0; i < octets.size(); ++i) {
        bus_.ecrireBrut(static_cast<uint8_t>(adresse + i), octets[i]);
    }
    return true;
}

bool CMachine::chargerBin(const std::string& chemin)
{
    std::ifstream fichier(chemin.c_str(), std::ios::binary);
    if (!fichier) {
        derniereErreur_ = "impossible d'ouvrir '" + chemin + "'";
        return false;
    }
    std::vector<uint8_t> octets;
    char c = 0;
    while (fichier.get(c)) {
        octets.push_back(static_cast<uint8_t>(c));
    }
    if (octets.empty()) {
        derniereErreur_ = "fichier vide : '" + chemin + "'";
        return false;
    }

    bus_.effacerMemoire();
    if (!chargerImage(octets, 0)) {
        return false;
    }
    reset();
    return true;
}

// ---------------------------------------------------------------------
//  Format .hex : du texte, donc lisible et modifiable a la main.
//
//     ; commentaire jusqu'a la fin de la ligne
//     @04        place les octets suivants a partir de l'adresse 0x04
//     08 01 18 80
// ---------------------------------------------------------------------
bool CMachine::chargerHex(const std::string& chemin)
{
    std::ifstream fichier(chemin.c_str());
    if (!fichier) {
        derniereErreur_ = "impossible d'ouvrir '" + chemin + "'";
        return false;
    }

    std::vector<uint8_t> image(isa::TAILLE_MEMOIRE, 0);
    bool        quelqueChose = false;
    int         adresse      = 0;
    int         numeroLigne  = 0;
    std::string ligne;

    while (std::getline(fichier, ligne)) {
        ++numeroLigne;

        const std::size_t p = ligne.find(';');
        if (p != std::string::npos) {
            ligne = ligne.substr(0, p);
        }

        std::istringstream flux(ligne);
        std::string        jeton;
        while (flux >> jeton) {
            // ---- changement d'adresse ----
            if (jeton[0] == '@') {
                jeton = jeton.substr(1);
                int valeur = 0;
                bool ok = !jeton.empty() && jeton.size() <= 2;
                for (std::size_t i = 0; ok && i < jeton.size(); ++i) {
                    int c4 = 0;
                    if (chiffreHexa(jeton[i], c4)) {
                        valeur = valeur * 16 + c4;
                    } else {
                        ok = false;
                    }
                }
                if (!ok) {
                    derniereErreur_ = "ligne " + std::to_string(numeroLigne) +
                                      " : adresse invalide '@" + jeton + "'";
                    return false;
                }
                adresse = valeur;
                continue;
            }

            // ---- octet ----
            if (jeton.size() != 2) {
                derniereErreur_ = "ligne " + std::to_string(numeroLigne) +
                                  " : '" + jeton +
                                  "' n'est pas un octet hexadecimal";
                return false;
            }
            int hautc = 0;
            int basc  = 0;
            if (!chiffreHexa(jeton[0], hautc) || !chiffreHexa(jeton[1], basc)) {
                derniereErreur_ = "ligne " + std::to_string(numeroLigne) +
                                  " : '" + jeton +
                                  "' n'est pas un octet hexadecimal";
                return false;
            }
            if (adresse >= static_cast<int>(isa::TAILLE_MEMOIRE)) {
                derniereErreur_ = "ligne " + std::to_string(numeroLigne) +
                                  " : depassement de la memoire";
                return false;
            }
            image[adresse++] = static_cast<uint8_t>(hautc * 16 + basc);
            quelqueChose     = true;
        }
    }

    if (!quelqueChose) {
        derniereErreur_ = "aucun octet trouve dans '" + chemin + "'";
        return false;
    }

    bus_.effacerMemoire();
    if (!chargerImage(image, 0)) {
        return false;
    }
    reset();
    return true;
}

bool CMachine::charger(const std::string& chemin)
{
    derniereErreur_.clear();
    const std::string ext = extension(chemin);
    if (ext == "hex" || ext == "txt") {
        return chargerHex(chemin);
    }
    return chargerBin(chemin);
}

// =====================================================================
//  Execution
// =====================================================================

void CMachine::reset()
{
    bus_.reset();
    cpu_.reset();
    dernierEtat_ = Etat::OK;
}

Etat CMachine::pas()
{
    dernierEtat_ = cpu_.pas();
    return dernierEtat_;
}

Arret CMachine::executer(uint64_t maxCycles)
{
    const uint64_t debut = cpu_.cycles();

    for (;;) {
        dernierEtat_ = cpu_.pas();

        if (dernierEtat_ == Etat::Halt) {
            return Arret::Halt;
        }
        if (dernierEtat_ != Etat::OK) {
            return Arret::Erreur;
        }
        // Point d'arret teste sur l'instruction SUIVANTE, pour que
        // l'utilisateur voie l'etat juste avant son execution.
        if (estPointArret(cpu_.pc())) {
            return Arret::PointArret;
        }
        if (maxCycles != 0 && cpu_.cycles() - debut >= maxCycles) {
            return Arret::LimiteCycles;
        }
    }
}

void CMachine::basculePointArret(uint8_t adresse)
{
    if (estPointArret(adresse)) {
        pointsArret_.erase(adresse);
    } else {
        pointsArret_.insert(adresse);
    }
}

// =====================================================================
//  Affichage (chaines de caracteres uniquement : aucune dependance
//  graphique, la console et l'interface les utilisent telles quelles)
// =====================================================================

std::string CMachine::desassembler(uint8_t adresse, int& nbOctets) const
{
    return desassembleur_.desassembler(bus_.memoire(), adresse, nbOctets);
}

std::vector<std::string> CMachine::listingAutourDuPc(int nbLignes) const
{
    std::vector<std::string> lignes;
    int adresse = cpu_.pc();

    for (int i = 0; i < nbLignes && adresse < static_cast<int>(isa::TAILLE_MEMOIRE);
         ++i) {
        int nb = 1;
        const std::string texte =
            desassembleur_.ligneComplete(bus_.memoire(), adresse, nb);

        std::string prefixe = "   ";
        if (adresse == cpu_.pc())            prefixe = "=> ";
        if (estPointArret(static_cast<uint8_t>(adresse))) {
            prefixe[0] = '*';
        }

        lignes.push_back(prefixe + texte);
        adresse += (nb > 0 ? nb : 1);
    }
    return lignes;
}

std::vector<std::string> CMachine::dumpMemoire(uint8_t debut, uint8_t fin) const
{
    std::vector<std::string> lignes;
    int d = debut & 0xF0;                 // on aligne sur 16 octets
    const int f = fin;

    while (d <= f) {
        std::string ligne = hexa(d, 4) + " |";
        std::string texte = " ";
        for (int i = 0; i < 16; ++i) {
            const int adresse = d + i;
            if (adresse >= static_cast<int>(isa::TAILLE_MEMOIRE)) break;
            const uint8_t v = bus_.lireBrut(static_cast<uint8_t>(adresse));
            ligne += " " + hexa(v, 2);
            texte += (v >= 32 && v < 127) ? static_cast<char>(v) : '.';
        }
        lignes.push_back(ligne + " |" + texte);
        d += 16;
    }
    return lignes;
}

// Fonction utilitaire exposee pour la console.
std::string texteBinaire8(uint8_t valeur) { return binaire8(valeur); }
