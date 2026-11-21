#include "processeur.h"

// =====================================================================
//  Libelle des etats
// =====================================================================
const char* texteEtat(Etat e)
{
    switch (e) {
    case Etat::OK:              return "en cours";
    case Etat::Halt:            return "arrete (HALT)";
    case Etat::OpcodeInvalide:  return "ERREUR : opcode invalide";
    case Etat::DebordementPile: return "ERREUR : debordement de pile";
    case Etat::PileVide:        return "ERREUR : pile vide";
    }
    return "?";
}

std::string CProcesseur::texteDrapeaux() const
{
    std::string s;
    s += "Z=";  s += (drapeauZ() ? '1' : '0');
    s += " C="; s += (drapeauC() ? '1' : '0');
    s += " N="; s += (drapeauN() ? '1' : '0');
    s += " I="; s += (drapeauI() ? '1' : '0');
    return s;
}

// =====================================================================
//  Outils internes
// =====================================================================

uint8_t CProcesseur::lireOctetProgramme()
{
    const uint8_t octet = bus_.lire(pc_);
    pc_ = static_cast<uint8_t>(pc_ + 1);   // rebouclage naturel sur 8 bits
    return octet;
}

// La pile descend depuis SP_INITIAL (0xFF) et ne doit pas deborder
// sous DEBUT_PILE (0xF0).
bool CProcesseur::empiler(uint8_t valeur)
{
    if (sp_ < isa::DEBUT_PILE) {
        return false;
    }
    bus_.ecrire(sp_, valeur);
    sp_ = static_cast<uint8_t>(sp_ - 1);
    return true;
}

bool CProcesseur::depiler(uint8_t& valeur)
{
    if (sp_ >= isa::SP_INITIAL) {
        return false;   // rien n'a ete empile
    }
    sp_    = static_cast<uint8_t>(sp_ + 1);
    valeur = bus_.lire(sp_);
    return true;
}

void CProcesseur::majDrapeau(uint8_t masque, bool actif)
{
    if (actif) {
        etat_ = static_cast<uint8_t>(etat_ | masque);
    } else {
        etat_ = static_cast<uint8_t>(etat_ & ~masque);
    }
}

void CProcesseur::majZN(uint8_t resultat)
{
    majDrapeau(isa::FLAG_Z, resultat == 0);
    majDrapeau(isa::FLAG_N, (resultat & 0x80) != 0);
}

// =====================================================================
//  Interruptions
//
//  Sequence d'entree (entre deux instructions seulement) :
//     1. empiler le PC
//     2. empiler le registre d'etat
//     3. interdire les interruptions (I <- 0)
//     4. PC <- VECTEUR_INTERRUPTION
//
//  RETI fait l'inverse : depiler l'etat, puis le PC. L'ordre doit etre
//  strictement respecte des deux cotes (pile LIFO).
// =====================================================================

bool CProcesseur::interruptionAPrendre() const
{
    return drapeauI() && !dansIT_ && bus_.interruptionEnAttente();
}

bool CProcesseur::prendreInterruption()
{
    if (!empiler(pc_))   return false;
    if (!empiler(etat_)) return false;

    majDrapeau(isa::FLAG_I, false);
    dansIT_ = true;
    pc_     = static_cast<uint8_t>(isa::VECTEUR_INTERRUPTION);

    bus_.acquitterInterruptions();
    ++nbInterruptions_;

    cycles_ += 3;          // cout d'entree en interruption
    bus_.tick(3);
    return true;
}

// =====================================================================
//  Un pas d'execution
// =====================================================================

Etat CProcesseur::pas()
{
    if (arrete_) {
        return Etat::Halt;
    }

    // ---- interruption eventuelle, AVANT de lire l'instruction ----
    if (interruptionAPrendre()) {
        if (!prendreInterruption()) {
            return Etat::DebordementPile;
        }
        return Etat::OK;
    }

    // ---- fetch ----
    const uint8_t octet1 = lireOctetProgramme();

    // ---- decode ----
    const isa::DescInstruction* d = isa::trouverParOpcode(octet1);
    if (d == nullptr) {
        pc_ = static_cast<uint8_t>(pc_ - 1);   // on laisse le PC sur la faute
        return Etat::OpcodeInvalide;
    }

    const uint8_t rd = static_cast<uint8_t>(octet1 & isa::MASQUE_REGISTRE);
    uint8_t octet2 = 0;
    if (d->taille == 2) {
        octet2 = lireOctetProgramme();
    }

    // ---- execute ----
    Etat resultat = Etat::OK;

    switch (d->opcode) {

    case 0x00:  // NOP
        break;

    case 0x08:  // LDI Rd, k        (ne touche pas aux drapeaux)
        registres_[rd] = octet2;
        break;

    case 0x10:  // LD Rd, adr       (ne touche pas aux drapeaux)
        registres_[rd] = bus_.lire(octet2);
        break;

    case 0x18:  // ST Rs, adr
        bus_.ecrire(octet2, registres_[rd]);
        break;

    case 0x20:  // MOV Rd, Rs       (ne touche pas aux drapeaux)
        registres_[rd] = registres_[octet2 & isa::MASQUE_REGISTRE];
        break;

    case 0x28: { // ADD Rd, Rs
        const unsigned somme = static_cast<unsigned>(registres_[rd]) +
                               registres_[octet2 & isa::MASQUE_REGISTRE];
        const uint8_t res = static_cast<uint8_t>(somme & 0xFF);
        majDrapeau(isa::FLAG_C, somme > 0xFF);
        majZN(res);
        registres_[rd] = res;
        break;
    }

    case 0x30: { // SUB Rd, Rs
        const uint8_t a = registres_[rd];
        const uint8_t b = registres_[octet2 & isa::MASQUE_REGISTRE];
        const uint8_t res = static_cast<uint8_t>(a - b);
        majDrapeau(isa::FLAG_C, a < b);      // C = emprunt
        majZN(res);
        registres_[rd] = res;
        break;
    }

    case 0x38: { // AND Rd, Rs
        const uint8_t res = static_cast<uint8_t>(
            registres_[rd] & registres_[octet2 & isa::MASQUE_REGISTRE]);
        majDrapeau(isa::FLAG_C, false);
        majZN(res);
        registres_[rd] = res;
        break;
    }

    case 0x40: { // OR Rd, Rs
        const uint8_t res = static_cast<uint8_t>(
            registres_[rd] | registres_[octet2 & isa::MASQUE_REGISTRE]);
        majDrapeau(isa::FLAG_C, false);
        majZN(res);
        registres_[rd] = res;
        break;
    }

    case 0x48: { // CMP Rd, Rs      (comme SUB, sans ecrire le resultat)
        const uint8_t a = registres_[rd];
        const uint8_t b = registres_[octet2 & isa::MASQUE_REGISTRE];
        const uint8_t res = static_cast<uint8_t>(a - b);
        majDrapeau(isa::FLAG_C, a < b);
        majZN(res);
        break;
    }

    case 0x50:  // JMP adr
        pc_ = octet2;
        break;

    case 0x58:  // JZ adr
        if (drapeauZ()) pc_ = octet2;
        break;

    case 0x60:  // JNZ adr
        if (!drapeauZ()) pc_ = octet2;
        break;

    case 0x80:  // JC adr   -- saut si retenue / emprunt
        if (drapeauC()) pc_ = octet2;
        break;

    case 0x88:  // JN adr   -- saut si resultat negatif (bit 7 a 1)
        if (drapeauN()) pc_ = octet2;
        break;

    case 0x68:  // CALL adr
        if (!empiler(pc_)) {          // PC pointe deja sur l'instruction suivante
            resultat = Etat::DebordementPile;
            break;
        }
        pc_ = octet2;
        break;

    case 0x70: { // RET
        uint8_t retour = 0;
        if (!depiler(retour)) {
            resultat = Etat::PileVide;
            break;
        }
        pc_ = retour;
        break;
    }

    case 0x78: { // RETI
        uint8_t ancienEtat = 0;
        uint8_t retour     = 0;
        if (!depiler(ancienEtat) || !depiler(retour)) {
            resultat = Etat::PileVide;
            break;
        }
        etat_   = static_cast<uint8_t>(ancienEtat | isa::FLAG_I);
        pc_     = retour;
        dansIT_ = false;
        break;
    }

    case 0xF8:  // HALT
        arrete_  = true;
        resultat = Etat::Halt;
        break;

    default:
        resultat = Etat::OpcodeInvalide;
        break;
    }

    // ---- comptabilite des cycles ----
    cycles_ += d->cycles;
    ++nbInstructions_;
    bus_.tick(d->cycles);

    return resultat;
}
