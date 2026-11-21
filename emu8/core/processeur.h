// =====================================================================
//  processeur.h  --  Le coeur du microcontroleur emule
//
//  Cycle fetch / decode / execute, pile, interruptions.
//
//  CONTRAINTE D'ARCHITECTURE (critere de validation du sujet) :
//  cette classe ne connait que CBus. Elle n'inclut aucun en-tete
//  graphique et n'affiche rien elle-meme. L'interface graphique se
//  contente de LIRE son etat apres chaque pas.
// =====================================================================

#ifndef PROCESSEUR_H
#define PROCESSEUR_H

#include "bus.h"
#include "isa.h"

#include <cstdint>
#include <string>

// Resultat d'un pas d'execution.
enum class Etat {
    OK,                  // instruction executee, on peut continuer
    Halt,                // HALT rencontre
    OpcodeInvalide,      // octet ne correspondant a aucune instruction
    DebordementPile,     // CALL / interruption de trop
    PileVide             // RET / RETI sans CALL correspondant
};

const char* texteEtat(Etat e);

class CProcesseur {
public:
    static constexpr int NB_REGISTRES = 8;

    explicit CProcesseur(CBus& bus) : bus_(bus) { reset(); }

    // -----------------------------------------------------------------
    //  Remise a zero : le PC repart du vecteur de reset, la pile est
    //  reinitialisee, les interruptions sont autorisees.
    // -----------------------------------------------------------------
    void reset() {
        for (int i = 0; i < NB_REGISTRES; ++i) {
            registres_[i] = 0;
        }
        pc_             = static_cast<uint8_t>(isa::VECTEUR_RESET);
        sp_             = static_cast<uint8_t>(isa::SP_INITIAL);
        etat_           = isa::FLAG_I;   // interruptions autorisees au demarrage
        cycles_         = 0;
        nbInstructions_ = 0;
        nbInterruptions_ = 0;
        arrete_         = false;
        dansIT_         = false;
    }

    // -----------------------------------------------------------------
    //  Execute une instruction (ou prend en compte une interruption).
    //  Renvoie l'etat resultant.
    // -----------------------------------------------------------------
    Etat pas();

    // ---- lecture de l'etat, pour la console et l'interface graphique ----
    uint8_t  registre(int i) const  { return registres_[i & 0x07]; }
    uint8_t  pc() const             { return pc_; }
    uint8_t  sp() const             { return sp_; }
    uint8_t  etat() const           { return etat_; }
    uint64_t cycles() const         { return cycles_; }
    uint64_t nbInstructions() const { return nbInstructions_; }
    unsigned nbInterruptions() const { return nbInterruptions_; }
    bool     arrete() const         { return arrete_; }
    bool     dansInterruption() const { return dansIT_; }

    bool drapeauZ() const { return (etat_ & isa::FLAG_Z) != 0; }
    bool drapeauC() const { return (etat_ & isa::FLAG_C) != 0; }
    bool drapeauN() const { return (etat_ & isa::FLAG_N) != 0; }
    bool drapeauI() const { return (etat_ & isa::FLAG_I) != 0; }

    // "Z=1 C=0 N=0 I=1"
    std::string texteDrapeaux() const;

    // ---- ecriture, reservee a l'interface graphique (mise au point) ----
    void setRegistre(int i, uint8_t v) { registres_[i & 0x07] = v; }
    void setPc(uint8_t v)              { pc_ = v; }

private:
    CBus&    bus_;
    uint8_t  registres_[NB_REGISTRES] = {0};
    uint8_t  pc_                      = 0;
    uint8_t  sp_                      = 0;
    uint8_t  etat_                    = 0;
    uint64_t cycles_                  = 0;
    uint64_t nbInstructions_          = 0;
    unsigned nbInterruptions_         = 0;
    bool     arrete_                  = false;
    bool     dansIT_                  = false;

    // ---- outils internes ----
    uint8_t lireOctetProgramme();              // lit a [PC] puis incremente PC
    bool    empiler(uint8_t valeur);
    bool    depiler(uint8_t& valeur);

    void    majDrapeau(uint8_t masque, bool actif);
    void    majZN(uint8_t resultat);

    bool    interruptionAPrendre() const;
    bool    prendreInterruption();
};

#endif // PROCESSEUR_H
