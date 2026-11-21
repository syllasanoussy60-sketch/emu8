#include "assembleur.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

// =====================================================================
//  Petites fonctions utilitaires, locales a ce fichier
// =====================================================================
namespace {

std::string trim(const std::string& s)
{
    std::size_t debut = 0;
    while (debut < s.size() &&
           std::isspace(static_cast<unsigned char>(s[debut]))) {
        ++debut;
    }
    std::size_t fin = s.size();
    while (fin > debut &&
           std::isspace(static_cast<unsigned char>(s[fin - 1]))) {
        --fin;
    }
    return s.substr(debut, fin - debut);
}

std::string majuscules(const std::string& s)
{
    std::string r = s;
    for (char& c : r) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return r;
}

std::vector<std::string> decouper(const std::string& s, char separateur)
{
    std::vector<std::string> morceaux;
    std::string courant;
    for (char c : s) {
        if (c == separateur) {
            morceaux.push_back(trim(courant));
            courant.clear();
        } else {
            courant += c;
        }
    }
    morceaux.push_back(trim(courant));
    return morceaux;
}

bool nomValide(const std::string& s)
{
    if (s.empty()) return false;
    if (!std::isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_') {
        return false;
    }
    for (std::size_t i = 1; i < s.size(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(s[i])) && s[i] != '_') {
            return false;
        }
    }
    return true;
}

std::string enHexa(int valeur, int chiffres)
{
    static const char* table = "0123456789ABCDEF";
    std::string r;
    for (int i = chiffres - 1; i >= 0; --i) {
        r += table[(valeur >> (4 * i)) & 0x0F];
    }
    return "0x" + r;
}

} // namespace

// =====================================================================
//  Gestion des erreurs et de l'ecriture memoire
// =====================================================================

void CAssembleur::erreur(int ligne, const std::string& message)
{
    res_->erreurs.push_back(Erreur{ligne, message});
}

void CAssembleur::ecrire(int adresse, uint8_t valeur, int ligne)
{
    if (adresse < 0 || adresse >= static_cast<int>(isa::TAILLE_MEMOIRE)) {
        erreur(ligne, "adresse " + enHexa(adresse, 4) + " hors de la memoire");
        return;
    }
    if (adresse >= isa::ADR_PORT_SORTIE && adresse < isa::DEBUT_RAM) {
        erreur(ligne, "adresse " + enHexa(adresse, 2) +
                          " : zone reservee aux peripheriques");
        return;
    }
    if (res_->occupe[adresse]) {
        erreur(ligne, "chevauchement de code a l'adresse " + enHexa(adresse, 2));
        return;
    }
    res_->code[adresse]        = valeur;
    res_->occupe[adresse]      = true;
    res_->ligneSource[adresse] = ligne;
}

// =====================================================================
//  Lecture des operandes
// =====================================================================

bool CAssembleur::lireRegistre(const std::string& jeton, int ligne,
                               uint8_t& registre)
{
    if (jeton.empty()) {
        erreur(ligne, "operande manquante (registre attendu)");
        return false;
    }
    const std::string t = majuscules(jeton);
    if (t.size() == 2 && t[0] == 'R' && t[1] >= '0' && t[1] <= '7') {
        registre = static_cast<uint8_t>(t[1] - '0');
        return true;
    }
    erreur(ligne, "registre invalide : '" + jeton + "' (attendu R0 a R7)");
    return false;
}

bool CAssembleur::lireValeur(const std::string& jeton, int ligne,
                             uint8_t& valeur)
{
    if (jeton.empty()) {
        erreur(ligne, "operande manquante (valeur ou etiquette attendue)");
        return false;
    }

    // Caractere : 'A'
    if (jeton.size() == 3 && jeton.front() == '\'' && jeton.back() == '\'') {
        valeur = static_cast<uint8_t>(jeton[1]);
        return true;
    }

    // Symbole (etiquette ou constante .equ)
    if (std::isalpha(static_cast<unsigned char>(jeton[0])) || jeton[0] == '_') {
        std::map<std::string, uint8_t>::const_iterator it =
            res_->symboles.find(jeton);
        if (it != res_->symboles.end()) {
            valeur = it->second;
            return true;
        }
        erreur(ligne, "symbole non defini : '" + jeton + "'");
        return false;
    }

    // Nombre
    long v    = 0;
    bool signe = false;
    std::string t = jeton;
    if (t[0] == '-') {
        signe = true;
        t     = t.substr(1);
    }
    if (t.empty()) {
        erreur(ligne, "nombre invalide : '" + jeton + "'");
        return false;
    }

    const std::string prefixe = majuscules(t.substr(0, 2));
    char* fin = nullptr;
    if (prefixe == "0X") {
        v = std::strtol(t.c_str() + 2, &fin, 16);
    } else if (prefixe == "0B") {
        v = std::strtol(t.c_str() + 2, &fin, 2);
    } else {
        v = std::strtol(t.c_str(), &fin, 10);
    }

    if (fin == nullptr || *fin != '\0' || fin == t.c_str() ||
        (prefixe == "0X" && fin == t.c_str() + 2) ||
        (prefixe == "0B" && fin == t.c_str() + 2)) {
        erreur(ligne, "nombre invalide : '" + jeton + "'");
        return false;
    }

    if (signe) v = -v;
    if (v < -128 || v > 255) {
        erreur(ligne, "valeur hors intervalle 8 bits : '" + jeton + "'");
        return false;
    }
    valeur = static_cast<uint8_t>(v & 0xFF);
    return true;
}

// =====================================================================
//  Analyse d'une ligne
// =====================================================================

CAssembleur::LigneAnalysee CAssembleur::analyserLigne(const std::string& brut,
                                                     int numero) const
{
    LigneAnalysee la;
    la.numero = numero;

    std::string s = brut;

    // Suppression du commentaire
    const std::size_t pc = s.find(';');
    if (pc != std::string::npos) {
        s = s.substr(0, pc);
    }
    s        = trim(s);
    la.texte = s;
    if (s.empty()) {
        return la; // vide == true
    }

    // Etiquette eventuelle
    const std::size_t pd = s.find(':');
    if (pd != std::string::npos) {
        la.etiquette = trim(s.substr(0, pd));
        s            = trim(s.substr(pd + 1));
    }

    // Mnemonique puis operandes
    if (!s.empty()) {
        const std::size_t pe = s.find_first_of(" \t");
        if (pe == std::string::npos) {
            la.mnemonique = s;
            s.clear();
        } else {
            la.mnemonique = s.substr(0, pe);
            s             = trim(s.substr(pe));
        }
        if (!s.empty()) {
            la.operandes = decouper(s, ',');
        }
    }

    la.vide = la.etiquette.empty() && la.mnemonique.empty();
    return la;
}

// =====================================================================
//  Assemblage
// =====================================================================

ResultatAssemblage CAssembleur::assemblerFichier(const std::string& chemin)
{
    std::ifstream fichier(chemin.c_str());
    if (!fichier) {
        ResultatAssemblage res;
        res.erreurs.push_back(Erreur{0, "impossible d'ouvrir '" + chemin + "'"});
        return res;
    }
    std::ostringstream tampon;
    tampon << fichier.rdbuf();
    return assembler(tampon.str());
}

ResultatAssemblage CAssembleur::assembler(const std::string& source)
{
    ResultatAssemblage res;
    res_ = &res;

    // Decoupage en lignes
    std::vector<std::string> lignes;
    {
        std::istringstream flux(source);
        std::string        l;
        while (std::getline(flux, l)) {
            if (!l.empty() && l.back() == '\r') {
                l.pop_back(); // fichier cree sous Windows
            }
            lignes.push_back(l);
        }
    }

    // -----------------------------------------------------------------
    //  PASSE 1 : adresses et table des symboles
    // -----------------------------------------------------------------
    std::vector<LigneAnalysee> analysees;
    int  adresse       = 0;
    bool debordSignale = false;

    for (std::size_t i = 0; i < lignes.size(); ++i) {
        LigneAnalysee la =
            analyserLigne(lignes[i], static_cast<int>(i) + 1);
        if (la.vide) {
            continue;
        }

        // ---- etiquette ----
        if (!la.etiquette.empty()) {
            if (!nomValide(la.etiquette)) {
                erreur(la.numero,
                       "nom d'etiquette invalide : '" + la.etiquette + "'");
            } else if (res.symboles.count(la.etiquette) != 0) {
                erreur(la.numero,
                       "etiquette deja definie : '" + la.etiquette + "'");
            } else {
                res.symboles[la.etiquette] = static_cast<uint8_t>(adresse & 0xFF);
            }
        }

        if (la.mnemonique.empty()) {
            continue; // ligne ne contenant qu'une etiquette
        }

        const std::string m = majuscules(la.mnemonique);

        // ---- directives ----
        if (m == ".ORG") {
            if (la.operandes.size() != 1) {
                erreur(la.numero, ".org attend une adresse");
            } else {
                uint8_t v = 0;
                if (lireValeur(la.operandes[0], la.numero, v)) {
                    adresse = v;
                }
            }
            continue;
        }

        if (m == ".EQU") {
            if (la.operandes.size() != 2) {
                erreur(la.numero, ".equ attend un nom et une valeur");
            } else if (!nomValide(la.operandes[0])) {
                erreur(la.numero,
                       "nom de constante invalide : '" + la.operandes[0] + "'");
            } else if (res.symboles.count(la.operandes[0]) != 0) {
                erreur(la.numero,
                       "symbole deja defini : '" + la.operandes[0] + "'");
            } else {
                uint8_t v = 0;
                if (lireValeur(la.operandes[1], la.numero, v)) {
                    res.symboles[la.operandes[0]] = v;
                }
            }
            continue;
        }

        if (m == ".DB") {
            if (la.operandes.empty()) {
                erreur(la.numero, ".db attend au moins un octet");
                continue;
            }
            la.adresse = adresse;
            adresse += static_cast<int>(la.operandes.size());
            analysees.push_back(la);
        } else {
            // ---- instruction ----
            const isa::DescInstruction* d =
                isa::trouverParMnemonique(la.mnemonique.c_str());
            if (d == nullptr) {
                erreur(la.numero,
                       "instruction inconnue : '" + la.mnemonique + "'");
                continue;
            }
            la.adresse = adresse;
            adresse += d->taille;
            analysees.push_back(la);
        }

        if (adresse > static_cast<int>(isa::TAILLE_MEMOIRE) && !debordSignale) {
            erreur(la.numero, "le programme depasse la taille de la memoire");
            debordSignale = true;
        }
    }

    // -----------------------------------------------------------------
    //  PASSE 2 : encodage
    // -----------------------------------------------------------------
    for (std::size_t i = 0; i < analysees.size(); ++i) {
        const LigneAnalysee& la = analysees[i];
        const std::string    m  = majuscules(la.mnemonique);

        // ---- .db ----
        if (m == ".DB") {
            for (std::size_t k = 0; k < la.operandes.size(); ++k) {
                uint8_t v = 0;
                if (lireValeur(la.operandes[k], la.numero, v)) {
                    ecrire(la.adresse + static_cast<int>(k), v, la.numero);
                }
            }
            res.listing.push_back(LigneListing{
                la.numero, la.adresse,
                static_cast<int>(la.operandes.size()), la.texte});
            continue;
        }

        const isa::DescInstruction* d =
            isa::trouverParMnemonique(la.mnemonique.c_str());
        if (d == nullptr) {
            continue; // deja signale en passe 1
        }

        // ---- verification du nombre d'operandes ----
        const std::size_t attendus = isa::nbOperandes(d->format);
        if (la.operandes.size() != attendus) {
            std::ostringstream msg;
            msg << d->mnemonique << " attend " << attendus << " operande"
                << (attendus > 1 ? "s" : "") << ", " << la.operandes.size()
                << " fournie" << (la.operandes.size() > 1 ? "s" : "");
            erreur(la.numero, msg.str());
            continue;
        }

        // ---- encodage ----
        uint8_t octet1 = d->opcode;
        uint8_t octet2 = 0;
        bool    valide = true;

        switch (d->format) {
        case isa::Format::Aucun:
            break;

        case isa::Format::RegImm:
        case isa::Format::RegAdr: {
            uint8_t r = 0;
            uint8_t v = 0;
            valide = lireRegistre(la.operandes[0], la.numero, r) & valide;
            valide = lireValeur(la.operandes[1], la.numero, v) & valide;
            octet1 = static_cast<uint8_t>(d->opcode | (r & isa::MASQUE_REGISTRE));
            octet2 = v;
            break;
        }

        case isa::Format::RegReg: {
            uint8_t rd = 0;
            uint8_t rs = 0;
            valide = lireRegistre(la.operandes[0], la.numero, rd) & valide;
            valide = lireRegistre(la.operandes[1], la.numero, rs) & valide;
            octet1 = static_cast<uint8_t>(d->opcode | (rd & isa::MASQUE_REGISTRE));
            octet2 = static_cast<uint8_t>(rs & isa::MASQUE_REGISTRE);
            break;
        }

        case isa::Format::Adr: {
            uint8_t a = 0;
            valide = lireValeur(la.operandes[0], la.numero, a) & valide;
            octet2 = a;
            break;
        }
        }

        if (!valide) {
            continue;
        }

        ecrire(la.adresse, octet1, la.numero);
        if (d->taille == 2) {
            ecrire(la.adresse + 1, octet2, la.numero);
        }

        res.listing.push_back(
            LigneListing{la.numero, la.adresse, d->taille, la.texte});
    }

    // Les erreurs proviennent des deux passes : on les remet dans
    // l'ordre du fichier source pour l'affichage.
    std::stable_sort(res.erreurs.begin(), res.erreurs.end(),
                     [](const Erreur& a, const Erreur& b) {
                         return a.ligne < b.ligne;
                     });

    res_ = nullptr;
    return res;
}
