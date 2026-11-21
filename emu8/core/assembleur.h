// =====================================================================
//  assembleur.h  --  Traduction du texte assembleur en code machine
//
//  Aucune dependance a l'interface graphique : cette classe se compile
//  et se teste en mode console.
// =====================================================================

#ifndef ASSEMBLEUR_H
#define ASSEMBLEUR_H

#include "isa.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// ---------------------------------------------------------------------
//  Une erreur de syntaxe, reperee par son numero de ligne.
//  (ligne == 0 : erreur globale, par exemple fichier introuvable)
// ---------------------------------------------------------------------
struct Erreur {
    int         ligne;
    std::string message;
};

// ---------------------------------------------------------------------
//  Une ligne effectivement assemblee (sert a l'affichage du listing et
//  au surlignage de la ligne courante dans l'interface graphique).
// ---------------------------------------------------------------------
struct LigneListing {
    int         ligne;     // numero dans le fichier source
    int         adresse;   // adresse d'implantation
    int         nbOctets;  // taille produite
    std::string texte;     // texte source, nettoye
};

// ---------------------------------------------------------------------
//  Resultat complet d'un assemblage.
// ---------------------------------------------------------------------
struct ResultatAssemblage {
    std::vector<uint8_t>           code;         // image memoire (256 octets)
    std::vector<bool>              occupe;       // octet ecrit par l'assembleur ?
    std::vector<int>               ligneSource;  // adresse -> ligne source (-1 sinon)
    std::map<std::string, uint8_t> symboles;     // etiquettes et constantes
    std::vector<LigneListing>      listing;
    std::vector<Erreur>            erreurs;

    ResultatAssemblage()
        : code(isa::TAILLE_MEMOIRE, 0),
          occupe(isa::TAILLE_MEMOIRE, false),
          ligneSource(isa::TAILLE_MEMOIRE, -1) {}

    bool ok() const { return erreurs.empty(); }
};

// ---------------------------------------------------------------------
//  L'assembleur proprement dit.
//
//  Fonctionne en deux passes :
//    passe 1 : analyse des lignes, calcul des adresses, collecte des
//              etiquettes (necessaire car une etiquette peut etre
//              utilisee avant d'etre definie : CALL delai) ;
//    passe 2 : encodage, les etiquettes etant desormais connues.
//
//  Directives reconnues :
//    .org <valeur>          place le code a l'adresse indiquee
//    .equ <NOM>, <valeur>   definit une constante
//    .db  <v1>, <v2>, ...   place des octets bruts en memoire
//
//  Formats numeriques acceptes : 0x1F, 0b00011111, 31, 'A'
// ---------------------------------------------------------------------
class CAssembleur {
public:
    ResultatAssemblage assembler(const std::string& source);
    ResultatAssemblage assemblerFichier(const std::string& chemin);

private:
    struct LigneAnalysee {
        int                      numero   = 0;
        int                      adresse  = 0;
        bool                     vide     = true;
        std::string              etiquette;
        std::string              mnemonique;
        std::vector<std::string> operandes;
        std::string              texte;
    };

    ResultatAssemblage* res_ = nullptr;

    LigneAnalysee analyserLigne(const std::string& brut, int numero) const;

    void erreur(int ligne, const std::string& message);
    void ecrire(int adresse, uint8_t valeur, int ligne);

    bool lireRegistre(const std::string& jeton, int ligne, uint8_t& registre);
    bool lireValeur(const std::string& jeton, int ligne, uint8_t& valeur);
};

#endif // ASSEMBLEUR_H
