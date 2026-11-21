// =====================================================================
//  machine.h  --  Assemblage de la carte complete
//
//  CMachine possede le bus, le processeur et les peripheriques, et
//  offre les services dont la console ET l'interface graphique ont
//  besoin : chargement d'un programme, pas a pas, execution continue,
//  points d'arret, reset.
//
//  C'est le SEUL point d'entree que l'interface graphique doit
//  utiliser. Elle ne manipule jamais le processeur directement.
// =====================================================================

#ifndef MACHINE_H
#define MACHINE_H

#include "bus.h"
#include "desassembleur.h"
#include "isa.h"
#include "peripherique.h"
#include "processeur.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

// Raison pour laquelle une execution continue s'est arretee.
enum class Arret {
    Halt,            // le programme a execute HALT
    PointArret,      // un point d'arret a ete atteint
    LimiteCycles,    // la limite de cycles demandee est atteinte
    Erreur           // opcode invalide, probleme de pile...
};

const char* texteArret(Arret a);

// "01001010" -- pratique pour afficher les LED et les interrupteurs.
std::string texteBinaire8(uint8_t valeur);

class CMachine {
public:
    CMachine();

    // -----------------------------------------------------------------
    //  Chargement d'un programme
    //
    //  Deux formats acceptes :
    //    .bin  image memoire brute (jusqu'a 256 octets)
    //    .hex  texte : octets en hexadecimal, '@XX' pour se placer a
    //          une adresse, ';' pour un commentaire
    //
    //  Le format est deduit de l'extension, ou force par chargerBin /
    //  chargerHex. Renvoie false et renseigne derniereErreur() en cas
    //  de probleme.
    // -----------------------------------------------------------------
    bool charger(const std::string& chemin);
    bool chargerBin(const std::string& chemin);
    bool chargerHex(const std::string& chemin);
    bool chargerImage(const std::vector<uint8_t>& octets, uint8_t adresse = 0);

    const std::string& derniereErreur() const { return derniereErreur_; }

    // -----------------------------------------------------------------
    //  Execution
    // -----------------------------------------------------------------
    void reset();

    // Un seul pas. Le resultat renseigne sur l'etat du processeur.
    Etat pas();

    // Execution continue. S'arrete sur HALT, sur un point d'arret, sur
    // une erreur, ou apres maxCycles cycles emules (0 = sans limite,
    // deconseille depuis une interface graphique).
    Arret executer(uint64_t maxCycles = 100000);

    // -----------------------------------------------------------------
    //  Points d'arret
    // -----------------------------------------------------------------
    void ajouterPointArret(uint8_t adresse)  { pointsArret_.insert(adresse); }
    void retirerPointArret(uint8_t adresse)  { pointsArret_.erase(adresse); }
    void basculePointArret(uint8_t adresse);
    void effacerPointsArret()                { pointsArret_.clear(); }
    bool estPointArret(uint8_t adresse) const {
        return pointsArret_.count(adresse) != 0;
    }
    const std::set<uint8_t>& pointsArret() const { return pointsArret_; }

    // -----------------------------------------------------------------
    //  Lecture de l'etat (pour la console et l'interface graphique)
    // -----------------------------------------------------------------
    CProcesseur&       cpu()         { return cpu_; }
    const CProcesseur& cpu() const   { return cpu_; }
    CBus&              bus()         { return bus_; }
    const CBus&        bus() const   { return bus_; }

    CPortSortie&       portSortie()  { return portSortie_; }
    CPortEntree&       portEntree()  { return portEntree_; }
    CTimer&            timer()       { return timer_; }
    CUart&             uart()        { return uart_; }

    Etat dernierEtat() const { return dernierEtat_; }

    // Desassemblage de l'instruction a l'adresse donnee.
    std::string desassembler(uint8_t adresse, int& nbOctets) const;

    // Lignes desassemblees autour du PC, pretes a afficher.
    std::vector<std::string> listingAutourDuPc(int nbLignes = 8) const;

    // Dump memoire formate (16 octets par ligne).
    std::vector<std::string> dumpMemoire(uint8_t debut, uint8_t fin) const;

private:
    CBus           bus_;
    CPortSortie    portSortie_;
    CPortEntree    portEntree_;
    CTimer         timer_;
    CUart          uart_;
    CProcesseur    cpu_;
    CDesassembleur desassembleur_;

    std::set<uint8_t> pointsArret_;
    Etat              dernierEtat_    = Etat::OK;
    std::string       derniereErreur_;
};

#endif // MACHINE_H
