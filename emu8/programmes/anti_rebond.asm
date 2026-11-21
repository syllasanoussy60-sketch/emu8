; =====================================================================
;  anti_rebond.asm  --  anti-rebond logiciel sur l'interrupteur 0
;
;  Un contact mecanique ne commute pas proprement : il rebondit
;  pendant quelques millisecondes. Si on lit l'entree sans precaution,
;  un seul appui est compte plusieurs fois.
;
;  Principe : on n'accepte un changement d'etat que s'il reste
;  identique pendant SEUIL lectures consecutives. Toute lecture qui
;  revient a l'ancien etat remet le compteur a zero.
;
;  Chaque appui valide incremente un compteur affiche sur les LED.
;
;  En console, les interrupteurs sont figes par l'option -e :
;     emu8 anti_rebond.bin -e 0x01 -c 400
;  Attendu : apres 8 lectures stables, les LED passent a 00000001,
;  puis le programme boucle sans rien changer (limite de cycles).
;
;  Dans l'interface graphique, l'interet devient visible : on peut
;  faire vibrer l'interrupteur et verifier qu'un seul appui est compte.
; =====================================================================

.equ ENTREE, 0x81
.equ SORTIE, 0x80
.equ SEUIL,  0x08           ; nombre de lectures stables exigees

        .org 0x00
        JMP  debut
        .org 0x02
        RETI

        .org 0x04
debut:  LDI  R7, 0x01        ; masque du bit 0, sert aussi de constante 1
        LDI  R6, 0x00        ; etat stable actuellement retenu
        LDI  R5, 0x00        ; compteur de lectures stables
        LDI  R4, SEUIL
        LDI  R0, 0x00        ; compteur d'appuis, affiche sur les LED
        ST   R0, SORTIE

boucle: LD   R1, ENTREE
        AND  R1, R7          ; on ne garde que l'interrupteur 0
        CMP  R1, R6          ; identique a l'etat retenu ?
        JZ   remise          ; oui : rien de nouveau, compteur a zero
        ADD  R5, R7          ; non : une lecture de plus dans le meme sens
        CMP  R5, R4
        JZ   valide          ; SEUIL lectures stables : on accepte
        JMP  boucle

remise: LDI  R5, 0x00
        JMP  boucle

; ---- changement d'etat accepte ----
valide: MOV  R6, R1          ; le nouvel etat devient l'etat retenu
        LDI  R5, 0x00
        CMP  R1, R7          ; front montant (l'interrupteur vient d'etre mis a 1) ?
        JNZ  boucle          ; non, c'est un relachement : on ne compte pas
        ADD  R0, R7          ; oui : un appui de plus
        ST   R0, SORTIE
        JMP  boucle
