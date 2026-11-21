; ---------------------------------------------------------------
;  chenillard.asm  --  allume les 8 LED l'une apres l'autre
; ---------------------------------------------------------------

.equ PORT_SORTIE, 0x80
.equ PORT_ENTREE, 0x81

        .org 0x00
        JMP  debut          ; vecteur de reset
        .org 0x02
        RETI                ; vecteur d'interruption (inutilise ici)

        .org 0x04
debut:  LDI  R0, 0x01       ; premiere LED
        LDI  R7, 0x01       ; constante 1, sert aux decrementations

boucle: ST   R0, PORT_SORTIE
        CALL delai
        ADD  R0, R0         ; decalage a gauche
        JNZ  boucle         ; apres 8 tours R0 = 0 donc Z = 1
        HALT

; ---- temporisation ----
delai:  LDI  R1, 0x20
d_bcl:  SUB  R1, R7
        JNZ  d_bcl
        RET
