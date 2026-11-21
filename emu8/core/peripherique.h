// =====================================================================
//  peripherique.h  --  Classe abstraite CPeripherique et ses derivees
//
//  Les peripheriques sont mappes en memoire : le processeur y accede
//  par une simple lecture ou ecriture a une adresse donnee, exactement
//  comme sur un microcontroleur reel.
//
//  AUCUNE dependance a une bibliotheque graphique dans ce fichier.
// =====================================================================

#ifndef PERIPHERIQUE_H
#define PERIPHERIQUE_H

#include "isa.h"

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------
//  Interface commune a tous les peripheriques.
// ---------------------------------------------------------------------
class CPeripherique {
public:
    virtual ~CPeripherique() = default;

    // Nom affiche dans l'interface et la console.
    virtual const char* nom() const = 0;

    // Ce peripherique repond-il a cette adresse ?
    virtual bool gereAdresse(uint8_t adresse) const = 0;

    // Acces memoire mappe.
    virtual uint8_t lire(uint8_t adresse) = 0;
    virtual void    ecrire(uint8_t adresse, uint8_t valeur) = 0;

    // Appele apres chaque instruction, avec le nombre de cycles
    // consommes par cette instruction.
    virtual void tick(unsigned cycles) { (void)cycles; }

    // Gestion des interruptions.
    virtual bool interruptionEnAttente() const { return false; }
    virtual void acquitterInterruption() {}

    virtual void reset() = 0;
};

// ---------------------------------------------------------------------
//  Port de sortie 8 bits (0x80) : les 8 LED.
// ---------------------------------------------------------------------
class CPortSortie : public CPeripherique {
public:
    const char* nom() const override { return "Port de sortie"; }

    bool gereAdresse(uint8_t adresse) const override {
        return adresse == isa::ADR_PORT_SORTIE;
    }

    uint8_t lire(uint8_t adresse) override {
        (void)adresse;
        return valeur_;
    }

    void ecrire(uint8_t adresse, uint8_t valeur) override {
        (void)adresse;
        valeur_ = valeur;
        ++nbEcritures_;
    }

    void reset() override {
        valeur_      = 0;
        nbEcritures_ = 0;
    }

    // Lecture par l'interface graphique.
    uint8_t  valeur() const      { return valeur_; }
    bool     led(int i) const    { return ((valeur_ >> i) & 1) != 0; }
    unsigned nbEcritures() const { return nbEcritures_; }

private:
    uint8_t  valeur_      = 0;
    unsigned nbEcritures_ = 0;
};

// ---------------------------------------------------------------------
//  Port d'entree 8 bits (0x81) : les 8 interrupteurs.
//
//  L'interface graphique appelle setBit() ou setValeur() ; le
//  programme emule lit l'adresse 0x81. Une ecriture du programme a
//  cette adresse est ignoree (c'est une entree).
// ---------------------------------------------------------------------
class CPortEntree : public CPeripherique {
public:
    const char* nom() const override { return "Port d'entree"; }

    bool gereAdresse(uint8_t adresse) const override {
        return adresse == isa::ADR_PORT_ENTREE;
    }

    uint8_t lire(uint8_t adresse) override {
        (void)adresse;
        return valeur_;
    }

    void ecrire(uint8_t adresse, uint8_t valeur) override {
        (void)adresse;
        (void)valeur;  // entree : ecriture ignoree
    }

    void reset() override { valeur_ = 0; }

    // Pilotage par l'interface graphique.
    void setValeur(uint8_t v) { valeur_ = v; }
    void setBit(int i, bool etat) {
        const uint8_t masque = static_cast<uint8_t>(1u << i);
        valeur_ = etat ? static_cast<uint8_t>(valeur_ | masque)
                       : static_cast<uint8_t>(valeur_ & ~masque);
    }
    void basculeBit(int i) {
        valeur_ = static_cast<uint8_t>(valeur_ ^ (1u << i));
    }

    uint8_t valeur() const           { return valeur_; }
    bool    interrupteur(int i) const { return ((valeur_ >> i) & 1) != 0; }

private:
    uint8_t valeur_ = 0;
};

// ---------------------------------------------------------------------
//  Timer (0x82 a 0x84).
//
//    0x82  valeur   : compteur, s'incremente tous les N cycles
//    0x83  controle : bit0 = marche, bit1 = interruption autorisee
//    0x84  periode  : N, en cycles (0 est interprete comme 256)
//
//  Quand le compteur passe de 0xFF a 0x00 (debordement) et que le
//  bit1 du registre de controle est arme, une interruption est levee.
// ---------------------------------------------------------------------
class CTimer : public CPeripherique {
public:
    static constexpr uint8_t BIT_MARCHE = 0x01;
    static constexpr uint8_t BIT_IT     = 0x02;

    const char* nom() const override { return "Timer"; }

    bool gereAdresse(uint8_t adresse) const override {
        return adresse >= isa::ADR_TIMER_VALEUR &&
               adresse <= isa::ADR_TIMER_PERIODE;
    }

    uint8_t lire(uint8_t adresse) override {
        switch (adresse) {
        case isa::ADR_TIMER_VALEUR:   return valeur_;
        case isa::ADR_TIMER_CONTROLE: return controle_;
        case isa::ADR_TIMER_PERIODE:  return periode_;
        default:                      return 0;
        }
    }

    void ecrire(uint8_t adresse, uint8_t valeur) override {
        switch (adresse) {
        case isa::ADR_TIMER_VALEUR:
            valeur_ = valeur;
            break;
        case isa::ADR_TIMER_CONTROLE:
            controle_ = valeur;
            // Desarmer les interruptions annule une demande en attente.
            if ((controle_ & BIT_IT) == 0) {
                itEnAttente_ = false;
            }
            break;
        case isa::ADR_TIMER_PERIODE:
            periode_   = valeur;
            compteur_  = 0;
            break;
        default:
            break;
        }
    }

    void tick(unsigned cycles) override {
        if ((controle_ & BIT_MARCHE) == 0) {
            return;
        }
        const unsigned n = (periode_ == 0) ? 256u : periode_;
        compteur_ += cycles;
        while (compteur_ >= n) {
            compteur_ -= n;
            ++valeur_;
            if (valeur_ == 0) {  // debordement 0xFF -> 0x00
                ++nbDebordements_;
                if ((controle_ & BIT_IT) != 0) {
                    itEnAttente_ = true;
                }
            }
        }
    }

    bool interruptionEnAttente() const override { return itEnAttente_; }
    void acquitterInterruption() override       { itEnAttente_ = false; }

    void reset() override {
        valeur_         = 0;
        controle_       = 0;
        periode_        = 0;
        compteur_       = 0;
        itEnAttente_    = false;
        nbDebordements_ = 0;
    }

    // Lecture par l'interface graphique.
    uint8_t  valeur() const         { return valeur_; }
    uint8_t  controle() const       { return controle_; }
    uint8_t  periode() const        { return periode_; }
    bool     enMarche() const       { return (controle_ & BIT_MARCHE) != 0; }
    bool     itArmee() const        { return (controle_ & BIT_IT) != 0; }
    unsigned nbDebordements() const { return nbDebordements_; }

private:
    uint8_t  valeur_         = 0;
    uint8_t  controle_       = 0;
    uint8_t  periode_        = 0;
    unsigned compteur_       = 0;
    bool     itEnAttente_    = false;
    unsigned nbDebordements_ = 0;
};

// ---------------------------------------------------------------------
//  UART simulee (0x85 et 0x86) -- fonctionnalite optionnelle du sujet.
//
//    0x85  donnee : une ecriture emet le caractere
//    0x86  etat   : bit0 = pret a emettre (toujours 1 ici)
//
//  Les caracteres emis sont accumules et affiches par la console ou
//  par l'interface graphique.
// ---------------------------------------------------------------------
class CUart : public CPeripherique {
public:
    const char* nom() const override { return "UART"; }

    bool gereAdresse(uint8_t adresse) const override {
        return adresse == isa::ADR_UART_DONNEE ||
               adresse == isa::ADR_UART_ETAT;
    }

    uint8_t lire(uint8_t adresse) override {
        if (adresse == isa::ADR_UART_ETAT) {
            return 0x01;  // toujours pret
        }
        return 0;
    }

    void ecrire(uint8_t adresse, uint8_t valeur) override {
        if (adresse == isa::ADR_UART_DONNEE) {
            sortie_ += static_cast<char>(valeur);
        }
    }

    void reset() override { sortie_.clear(); }

    const std::string& sortie() const { return sortie_; }
    void viderSortie()                { sortie_.clear(); }

private:
    std::string sortie_;
};

#endif // PERIPHERIQUE_H
