# Contrat d'encodage — microcontrôleur 8 bits (version 2)

> **Changements depuis la version 1 :** ajout des sauts conditionnels
> `JC` (opcode 0x80) et `JN` (opcode 0x88). Sans eux, les drapeaux C et N
> étaient calculés mais inutilisables, et il était impossible de tester
> « plus grand ou égal », donc d'écrire une division par soustractions
> successives. Ils occupent deux opcodes qui étaient déjà réservés à cet
> usage par la version 1. Aucune autre modification : tout code machine
> produit pour la version 1 reste valide.

Ce document est le **contrat entre le groupe assembleur et le groupe émulateur**.
Il correspond exactement au fichier `core/isa.h`, qui en est la version
faisant foi en C++. Toute modification doit être validée par les deux
groupes et faire monter le numéro de version.

Le fichier `core/isa.h` doit être **le même octet pour octet** dans les deux
parties du projet. C'est lui, et non ce document, qu'il faut copier.

---

## 1. Registres

| Élément | Taille | Rôle |
|---|---|---|
| R0 à R7 | 8 bits | registres généraux |
| PC | 8 bits | compteur de programme, démarre à 0x00 |
| SP | 8 bits | pointeur de pile, initialisé à 0xFF, la pile descend |
| État | 8 bits | drapeaux Z, C, N, I |

Drapeaux : `Z = 0x01`, `C = 0x02`, `N = 0x04`, `I = 0x08` (interruptions
autorisées). Au reset, l'état vaut `I` seul, donc les interruptions sont
autorisées dès le départ.

## 2. Carte mémoire (256 octets)

| Adresses | Contenu |
|---|---|
| 0x00 | vecteur de reset — le PC démarre ici |
| 0x02 | vecteur d'interruption |
| 0x00–0x7F | zone de programme |
| 0x80 | port de sortie (8 LED) |
| 0x81 | port d'entrée (8 interrupteurs) |
| 0x82 | timer — valeur du compteur |
| 0x83 | timer — contrôle : bit0 = marche, bit1 = interruption armée |
| 0x84 | timer — période N, en cycles (0 vaut 256) |
| 0x85 | UART — une écriture émet le caractère *(optionnel)* |
| 0x86 | UART — état, bit0 = prêt à émettre *(optionnel)* |
| 0x90–0xEF | RAM de données |
| 0xF0–0xFF | pile |

Le vecteur de reset en 0x00 et le vecteur d'interruption en 0x02 sont
séparés de 2 octets, soit exactement la taille d'un `JMP`. Tout programme
commence donc par `JMP debut` puis, en 0x02, `JMP isr` (ou `RETI` si le
programme n'utilise pas les interruptions).

## 3. Encodage

```
Octet 1 :  O O O O O r r r      5 bits d'opcode, 3 bits de registre
Octet 2 :  donnée immédiate, adresse, ou numéro de registre source
```

Toutes les instructions font **1 ou 2 octets**, ce qui simplifie autant
l'assembleur que le décodeur.

| Instruction | Octet 1 | Octet 2 | Effet | Cycles |
|---|---|---|---|---|
| `NOP` | 0x00 | — | rien | 1 |
| `LDI Rd, k` | 0x08 \| d | k | Rd ← k | 1 |
| `LD Rd, adr` | 0x10 \| d | adr | Rd ← mem[adr] | 2 |
| `ST Rs, adr` | 0x18 \| s | adr | mem[adr] ← Rs | 2 |
| `MOV Rd, Rs` | 0x20 \| d | s | Rd ← Rs | 1 |
| `ADD Rd, Rs` | 0x28 \| d | s | Rd ← Rd + Rs | 1 |
| `SUB Rd, Rs` | 0x30 \| d | s | Rd ← Rd − Rs | 1 |
| `AND Rd, Rs` | 0x38 \| d | s | Rd ← Rd & Rs | 1 |
| `OR Rd, Rs` | 0x40 \| d | s | Rd ← Rd \| Rs | 1 |
| `CMP Rd, Rs` | 0x48 \| d | s | comme SUB, sans écrire le résultat | 1 |
| `JMP adr` | 0x50 | adr | PC ← adr | 2 |
| `JZ adr` | 0x58 | adr | si Z : PC ← adr | 2 |
| `JNZ adr` | 0x60 | adr | si Z = 0 : PC ← adr | 2 |
| `CALL adr` | 0x68 | adr | empile PC, PC ← adr | 3 |
| `RET` | 0x70 | — | dépile PC | 3 |
| `RETI` | 0x78 | — | dépile état puis PC, I ← 1 | 3 |
| `JC adr` | 0x80 | adr | si C : PC ← adr | 2 |
| `JN adr` | 0x88 | adr | si N : PC ← adr | 2 |
| `HALT` | 0xF8 | — | arrêt du processeur | 1 |

Pour les instructions à adresse seule (`JMP`, `JZ`, `JNZ`, `JC`, `JN`,
`CALL`), les 3 bits de registre de l'octet 1 valent 0.

`JC` est le saut à retenir : comme `CMP Rd, Rs` met C à 1 quand
`Rd < Rs`, la séquence `CMP R0, R3` puis `JC fin` se lit « si R0 est plus
petit que R3, sortir ». C'est ce qui permet d'écrire une division par
soustractions successives, comme dans
`programmes/binaire_decimal.asm`.

`HALT` vaut **0xF8** et non 0xFF, mais comme le décodage ignore les 3 bits
de poids faible, tout octet de 0xF8 à 0xFF se décode en `HALT`. Une mémoire
vierge remplie de 0xFF arrête donc proprement le processeur au lieu de
partir dans le décor.

### Exemples

| Source | Code machine |
|---|---|
| `LDI R0, 0x01` | `08 01` |
| `LDI R7, 0x01` | `0F 01` |
| `ST R0, 0x80` | `18 80` |
| `ADD R3, R5` | `2B 05` |
| `CMP R6, R5` | `4E 05` |
| `JNZ 0x08` | `60 08` |
| `CALL 0x11` | `68 11` |

## 4. Drapeaux

| Instruction | Z | C | N |
|---|---|---|---|
| `ADD` | résultat nul | débordement au-delà de 255 | bit 7 du résultat |
| `SUB`, `CMP` | résultat nul | emprunt, c'est-à-dire Rd < Rs | bit 7 du résultat |
| `AND`, `OR` | résultat nul | forcé à 0 | bit 7 du résultat |
| `LDI`, `LD`, `ST`, `MOV` | inchangé | inchangé | inchangé |

Le drapeau I n'est modifié que par le matériel : il passe à 0 à l'entrée en
interruption et revient à 1 sur `RETI`. Il n'existe pas d'instruction pour
le manipuler ; c'est le bit1 du registre de contrôle du timer qui sert à
armer ou désarmer la source d'interruption.

## 5. Pile

La pile descend depuis 0xFF et ne doit pas franchir 0xF0, soit 16 octets.

- empiler : `mem[SP] ← valeur` puis `SP ← SP − 1`
- dépiler : `SP ← SP + 1` puis `valeur ← mem[SP]`

`CALL` empile l'adresse de retour, c'est-à-dire le PC **après** lecture des
deux octets de l'instruction. L'émulateur signale un débordement de pile
(`CALL` de trop) et une pile vide (`RET` sans `CALL`) comme des erreurs
d'exécution.

## 6. Interruptions

Une interruption n'est prise en compte qu'**entre deux instructions**, et
seulement si `I = 1` et qu'un périphérique en demande une.

Séquence d'entrée :

1. empiler le PC ;
2. empiler le registre d'état ;
3. `I ← 0` ;
4. `PC ← 0x02`.

`RETI` fait l'inverse : dépiler l'état, puis le PC, et remettre `I` à 1.
**L'ordre doit être strictement respecté des deux côtés**, la pile étant
LIFO.

Sauvegarder le registre d'état est indispensable : sans cela, un `SUB` dans
la routine d'interruption écraserait le résultat du `CMP` du programme
principal, et un `JZ` pourrait sauter au mauvais endroit. Le programme de
test `programmes/timer_it.hex` vérifie précisément ce point.

Le timer lève une interruption quand son compteur déborde de 0xFF à 0x00,
et seulement si le bit1 de son registre de contrôle est armé.

## 7. Format des fichiers échangés

L'assembleur produit, et l'émulateur consomme, l'un de ces deux formats.

**`.bin`** — image mémoire brute, jusqu'à 256 octets, chargée à partir de
l'adresse 0x00. C'est le format d'échange normal.

**`.hex`** — la même chose en texte, pour écrire ou corriger un programme à
la main sans assembleur :

```
; commentaire jusqu'à la fin de la ligne
@04                 ; place les octets suivants à partir de 0x04
08 01 18 80
```

L'émulateur accepte les deux, en choisissant selon l'extension.

## 8. Limites connues de cette version 1

Ces points sont assumés pour l'instant et pourront évoluer d'un commun
accord :

- pas d'instruction pour autoriser ou interdire les interruptions depuis le
  programme ;
- pas d'instruction de décalage ni d'incrémentation — on décrémente avec un
  registre contenant 1, et on décale à gauche avec `ADD Rd, Rd` ;
- pas d'adressage indirect, donc pas de parcours de tableau indexé ;
- les opcodes 0x90 à 0xF0 restent libres pour d'éventuels ajouts.
