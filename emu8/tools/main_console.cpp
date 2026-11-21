// =====================================================================
//  main_console.cpp  --  Emulateur en mode console
//
//  C'est le CRITERE DE VALIDATION de l'architecture demande par le
//  sujet : le processeur et ses peripheriques tournent ici sans la
//  moindre ligne de code graphique.
//
//  Usage :
//    emu8 programme.hex                 execute jusqu'au HALT
//    emu8 programme.hex -t              idem, en affichant la trace
//    emu8 programme.hex -i              mode interactif (pas a pas)
//    emu8 programme.hex -e 0x0F         interrupteurs a 0x0F
//    emu8 programme.hex -c 5000         limite de cycles
//    emu8 programme.hex -b 0x08         point d'arret a l'adresse 0x08
// =====================================================================

#include "machine.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string hexa(unsigned valeur, int chiffres)
{
    static const char* table = "0123456789ABCDEF";
    std::string r;
    for (int i = chiffres - 1; i >= 0; --i) {
        r += table[(valeur >> (4 * i)) & 0x0F];
    }
    return r;
}

// Lit un nombre ecrit 0x1F, 0b1010 ou 31.
bool lireNombre(const std::string& texte, long& valeur)
{
    if (texte.empty()) return false;
    const char* debut = texte.c_str();
    int         base  = 10;
    if (texte.size() > 2 && texte[0] == '0' &&
        (texte[1] == 'x' || texte[1] == 'X')) {
        base  = 16;
        debut = texte.c_str() + 2;
    } else if (texte.size() > 2 && texte[0] == '0' &&
               (texte[1] == 'b' || texte[1] == 'B')) {
        base  = 2;
        debut = texte.c_str() + 2;
    }
    char* fin = nullptr;
    valeur = std::strtol(debut, &fin, base);
    return fin != nullptr && *fin == '\0' && fin != debut;
}

// ---------------------------------------------------------------------
//  Affichage de l'etat complet de la machine
// ---------------------------------------------------------------------
void afficherEtat(const CMachine& machine)
{
    const CProcesseur& cpu = machine.cpu();

    std::cout << "\n--- Processeur -------------------------------------\n";
    std::cout << " PC=0x" << hexa(cpu.pc(), 2)
              << "   SP=0x" << hexa(cpu.sp(), 2)
              << "   " << cpu.texteDrapeaux()
              << "   cycles=" << cpu.cycles()
              << "   instr=" << cpu.nbInstructions();
    if (cpu.dansInterruption()) {
        std::cout << "   [dans l'ISR]";
    }
    std::cout << "\n";

    std::cout << " ";
    for (int i = 0; i < CProcesseur::NB_REGISTRES; ++i) {
        std::cout << "R" << i << "=0x" << hexa(cpu.registre(i), 2) << " ";
    }
    std::cout << "\n";
}

void afficherPeripheriques(CMachine& machine)
{
    std::cout << "--- Peripheriques ---------------------------------\n";
    std::cout << " LED  (0x80) : " << texteBinaire8(machine.portSortie().valeur())
              << "   (0x" << hexa(machine.portSortie().valeur(), 2) << ")\n";
    std::cout << " Int. (0x81) : " << texteBinaire8(machine.portEntree().valeur())
              << "   (0x" << hexa(machine.portEntree().valeur(), 2) << ")\n";
    std::cout << " Timer       : valeur=0x" << hexa(machine.timer().valeur(), 2)
              << " periode=" << static_cast<int>(machine.timer().periode())
              << " marche=" << (machine.timer().enMarche() ? "oui" : "non")
              << " IT=" << (machine.timer().itArmee() ? "armee" : "non")
              << " debordements=" << machine.timer().nbDebordements() << "\n";
    std::cout << " Interruptions prises : " << machine.cpu().nbInterruptions()
              << "\n";
    if (!machine.uart().sortie().empty()) {
        std::cout << " UART        : \"" << machine.uart().sortie() << "\"\n";
    }
}

void afficherListing(const CMachine& machine, int nbLignes)
{
    std::cout << "--- Code ------------------------------------------\n";
    const std::vector<std::string> lignes = machine.listingAutourDuPc(nbLignes);
    for (std::size_t i = 0; i < lignes.size(); ++i) {
        std::cout << " " << lignes[i] << "\n";
    }
}

void afficherMemoire(const CMachine& machine, uint8_t debut, uint8_t fin)
{
    const std::vector<std::string> lignes = machine.dumpMemoire(debut, fin);
    for (std::size_t i = 0; i < lignes.size(); ++i) {
        std::cout << " " << lignes[i] << "\n";
    }
}

void afficherAide()
{
    std::cout
        << "\nCommandes :\n"
        << "  s [n]        executer n pas (1 par defaut)\n"
        << "  r            executer jusqu'a HALT ou point d'arret\n"
        << "  b <adr>      poser / retirer un point d'arret\n"
        << "  l            afficher le code autour du PC\n"
        << "  m [d] [f]    dump memoire (0x90 a 0xFF par defaut)\n"
        << "  e <valeur>   positionner les 8 interrupteurs\n"
        << "  t            activer / desactiver la trace\n"
        << "  z            reset du processeur\n"
        << "  ?            cet aide-memoire\n"
        << "  q            quitter\n\n";
}

// Une ligne de trace : adresse, instruction, puis effet observe.
void afficherTrace(CMachine& machine, uint8_t pcAvant)
{
    int nb = 1;
    const std::string instruction = machine.desassembler(pcAvant, nb);
    std::cout << "  " << hexa(pcAvant, 4) << "  " << std::left
              << std::setw(16) << instruction << std::right
              << " | LED=" << texteBinaire8(machine.portSortie().valeur())
              << " " << machine.cpu().texteDrapeaux()
              << " cyc=" << machine.cpu().cycles() << "\n";
}

// ---------------------------------------------------------------------
//  Mode interactif
// ---------------------------------------------------------------------
int modeInteractif(CMachine& machine, bool trace)
{
    afficherAide();
    afficherListing(machine, 6);
    afficherEtat(machine);

    std::string ligne;
    for (;;) {
        std::cout << "\nemu8> ";
        if (!std::getline(std::cin, ligne)) {
            break;
        }

        std::istringstream flux(ligne);
        std::string        commande;
        if (!(flux >> commande)) {
            continue;
        }

        if (commande == "q") {
            break;
        }

        if (commande == "?" || commande == "h") {
            afficherAide();
            continue;
        }

        if (commande == "s") {
            long n = 1;
            std::string arg;
            if (flux >> arg) {
                lireNombre(arg, n);
            }
            for (long i = 0; i < n; ++i) {
                const uint8_t pcAvant = machine.cpu().pc();
                const Etat    etat    = machine.pas();
                if (trace) {
                    afficherTrace(machine, pcAvant);
                }
                if (etat != Etat::OK) {
                    std::cout << " >>> " << texteEtat(etat) << "\n";
                    break;
                }
            }
            afficherListing(machine, 4);
            afficherEtat(machine);
            afficherPeripheriques(machine);
            continue;
        }

        if (commande == "r") {
            const Arret arret = machine.executer(0);
            std::cout << " >>> arret : " << texteArret(arret);
            if (arret == Arret::Erreur) {
                std::cout << " (" << texteEtat(machine.dernierEtat()) << ")";
            }
            std::cout << "\n";
            afficherEtat(machine);
            afficherPeripheriques(machine);
            continue;
        }

        if (commande == "b") {
            std::string arg;
            long        valeur = 0;
            if ((flux >> arg) && lireNombre(arg, valeur)) {
                const uint8_t adresse = static_cast<uint8_t>(valeur);
                machine.basculePointArret(adresse);
                std::cout << " point d'arret a 0x" << hexa(adresse, 2) << " : "
                          << (machine.estPointArret(adresse) ? "pose" : "retire")
                          << "\n";
            } else {
                std::cout << " usage : b <adresse>\n";
            }
            continue;
        }

        if (commande == "l") {
            afficherListing(machine, 10);
            continue;
        }

        if (commande == "m") {
            long debut = 0x90;
            long fin   = 0xFF;
            std::string a1;
            std::string a2;
            if (flux >> a1) lireNombre(a1, debut);
            if (flux >> a2) lireNombre(a2, fin);
            afficherMemoire(machine, static_cast<uint8_t>(debut),
                            static_cast<uint8_t>(fin));
            continue;
        }

        if (commande == "e") {
            std::string arg;
            long        valeur = 0;
            if ((flux >> arg) && lireNombre(arg, valeur)) {
                machine.portEntree().setValeur(static_cast<uint8_t>(valeur));
                std::cout << " interrupteurs = "
                          << texteBinaire8(machine.portEntree().valeur()) << "\n";
            } else {
                std::cout << " usage : e <valeur>\n";
            }
            continue;
        }

        if (commande == "t") {
            trace = !trace;
            std::cout << " trace " << (trace ? "activee" : "desactivee") << "\n";
            continue;
        }

        if (commande == "z") {
            machine.reset();
            std::cout << " processeur remis a zero\n";
            afficherEtat(machine);
            continue;
        }

        std::cout << " commande inconnue : '" << commande
                  << "'   (? pour l'aide)\n";
    }
    return 0;
}

void afficherUsage()
{
    std::cout
        << "usage : emu8 programme[.hex|.bin] [options]\n"
        << "  -i          mode interactif (pas a pas)\n"
        << "  -t          afficher la trace d'execution\n"
        << "  -e <val>    valeur initiale des 8 interrupteurs\n"
        << "  -c <n>      limite de cycles (defaut 100000, 0 = illimite)\n"
        << "  -b <adr>    point d'arret (repetable)\n";
}

} // namespace

int main(int argc, char* argv[])
{
    std::string          fichier;
    bool                 interactif = false;
    bool                 trace      = false;
    long                 entrees    = 0;
    long                 maxCycles  = 100000;
    std::vector<uint8_t> arrets;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-i") {
            interactif = true;
        } else if (arg == "-t") {
            trace = true;
        } else if (arg == "-e" && i + 1 < argc) {
            lireNombre(argv[++i], entrees);
        } else if (arg == "-c" && i + 1 < argc) {
            lireNombre(argv[++i], maxCycles);
        } else if (arg == "-b" && i + 1 < argc) {
            long valeur = 0;
            if (lireNombre(argv[++i], valeur)) {
                arrets.push_back(static_cast<uint8_t>(valeur));
            }
        } else if (arg == "-h" || arg == "--help") {
            afficherUsage();
            return 0;
        } else if (fichier.empty()) {
            fichier = arg;
        } else {
            std::cerr << "argument inattendu : " << arg << "\n";
            return 2;
        }
    }

    if (fichier.empty()) {
        afficherUsage();
        return 2;
    }

    CMachine machine;
    if (!machine.charger(fichier)) {
        std::cerr << "erreur : " << machine.derniereErreur() << "\n";
        return 1;
    }
    machine.portEntree().setValeur(static_cast<uint8_t>(entrees));
    for (std::size_t i = 0; i < arrets.size(); ++i) {
        machine.ajouterPointArret(arrets[i]);
    }

    std::cout << "Programme charge : " << fichier << "\n";

    if (interactif) {
        return modeInteractif(machine, trace);
    }

    // ---- execution directe ----
    if (trace) {
        std::cout << "\n--- Trace -----------------------------------------\n";
        for (;;) {
            const uint8_t pcAvant = machine.cpu().pc();
            const Etat    etat    = machine.pas();
            afficherTrace(machine, pcAvant);

            if (etat == Etat::Halt) {
                std::cout << " >>> HALT\n";
                break;
            }
            if (etat != Etat::OK) {
                std::cout << " >>> " << texteEtat(etat) << "\n";
                break;
            }
            if (machine.estPointArret(machine.cpu().pc())) {
                std::cout << " >>> point d'arret a 0x"
                          << hexa(machine.cpu().pc(), 2) << "\n";
                break;
            }
            if (maxCycles != 0 &&
                machine.cpu().cycles() >= static_cast<uint64_t>(maxCycles)) {
                std::cout << " >>> limite de cycles atteinte\n";
                break;
            }
        }
    } else {
        const Arret arret =
            machine.executer(static_cast<uint64_t>(maxCycles < 0 ? 0 : maxCycles));
        std::cout << "Arret : " << texteArret(arret);
        if (arret == Arret::Erreur) {
            std::cout << " (" << texteEtat(machine.dernierEtat()) << ")";
        }
        std::cout << "\n";
    }

    afficherEtat(machine);
    afficherPeripheriques(machine);
    std::cout << "--- Memoire (0x90-0xFF) ---------------------------\n";
    afficherMemoire(machine, 0x90, 0xFF);

    // Code de retour : 0 si le programme s'est arrete proprement.
    return (machine.dernierEtat() == Etat::Halt ||
            machine.dernierEtat() == Etat::OK)
               ? 0
               : 1;
}
