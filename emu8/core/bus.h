// =====================================================================
//  bus.h  --  Bus memoire 8 bits
//
//  Le processeur ne connait QUE le bus : il ignore totalement
//  l'existence des peripheriques et de l'interface graphique.
//  C'est le bus qui, a chaque acces, regarde si un peripherique
//  revendique l'adresse demandee ; sinon il accede a la memoire.
// =====================================================================

#ifndef BUS_H
#define BUS_H

#include "isa.h"
#include "peripherique.h"

#include <algorithm>
#include <cstdint>
#include <vector>

class CBus {
public:
    CBus() : memoire_(isa::TAILLE_MEMOIRE, 0) {}

    // Le bus ne devient pas proprietaire du peripherique : c'est
    // CMachine qui le possede et garantit sa duree de vie.
    void ajouterPeripherique(CPeripherique* p) {
        if (p != nullptr) {
            peripheriques_.push_back(p);
        }
    }

    uint8_t lire(uint8_t adresse) {
        for (CPeripherique* p : peripheriques_) {
            if (p->gereAdresse(adresse)) {
                ++nbLectures_;
                return p->lire(adresse);
            }
        }
        ++nbLectures_;
        return memoire_[adresse];
    }

    void ecrire(uint8_t adresse, uint8_t valeur) {
        ++nbEcritures_;
        for (CPeripherique* p : peripheriques_) {
            if (p->gereAdresse(adresse)) {
                p->ecrire(adresse, valeur);
                return;
            }
        }
        memoire_[adresse] = valeur;
    }

    // ---- acces direct, sans passer par les peripheriques ----
    // Reserve au chargeur de programme et a l'affichage de la memoire
    // par l'interface graphique : un dump memoire ne doit pas avoir
    // d'effet de bord sur un peripherique.
    uint8_t lireBrut(uint8_t adresse) const  { return memoire_[adresse]; }
    void    ecrireBrut(uint8_t adresse, uint8_t valeur) {
        memoire_[adresse] = valeur;
    }

    const std::vector<uint8_t>& memoire() const { return memoire_; }

    void effacerMemoire() {
        std::fill(memoire_.begin(), memoire_.end(), static_cast<uint8_t>(0));
    }

    void tick(unsigned cycles) {
        for (CPeripherique* p : peripheriques_) {
            p->tick(cycles);
        }
    }

    // Un peripherique demande-t-il une interruption ?
    bool interruptionEnAttente() const {
        for (const CPeripherique* p : peripheriques_) {
            if (p->interruptionEnAttente()) {
                return true;
            }
        }
        return false;
    }

    void acquitterInterruptions() {
        for (CPeripherique* p : peripheriques_) {
            if (p->interruptionEnAttente()) {
                p->acquitterInterruption();
            }
        }
    }

    void reset() {
        for (CPeripherique* p : peripheriques_) {
            p->reset();
        }
        nbLectures_  = 0;
        nbEcritures_ = 0;
    }

    unsigned nbLectures() const  { return nbLectures_; }
    unsigned nbEcritures() const { return nbEcritures_; }

private:
    std::vector<uint8_t>        memoire_;
    std::vector<CPeripherique*> peripheriques_;
    unsigned                    nbLectures_  = 0;
    unsigned                    nbEcritures_ = 0;
};

#endif // BUS_H
