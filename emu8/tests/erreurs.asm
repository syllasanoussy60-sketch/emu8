; Fichier de test : chaque ligne doit produire une erreur

        LDI  R9, 0x01       ; registre invalide
        MOVE R0, R1         ; instruction inconnue
        LDI  R0, 0x1FF      ; valeur hors 8 bits
        ADD  R0             ; nombre d'operandes incorrect
        JMP  inexistante    ; symbole non defini
        LDI  R0, 0xZZ       ; nombre invalide
boucle: NOP
boucle: NOP                 ; etiquette dupliquee
        ST   R0, 0x80
        HALT
