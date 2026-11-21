; =====================================================================
;  binaire_decimal.asm  --  conversion binaire vers decimal
;
;  Lit la valeur presentee sur les 8 interrupteurs (0x81), la convertit
;  en trois chiffres decimaux ranges en 0x90, 0x91 et 0x92, puis les
;  emet en ASCII sur l'UART.
;
;  Il n'y a pas d'instruction de division : on divise par soustractions
;  successives. C'est exactement ce que fait un microcontroleur sans
;  multiplieur materiel.
;
;  A lancer avec :  emu8 binaire_decimal.bin -e 170
;  Attendu : 0x90 = 1, 0x91 = 7, 0x92 = 0, UART = "170"
; =====================================================================

.equ ENTREE,    0x81
.equ CENTAINES, 0x90
.equ DIZAINES,  0x91
.equ UNITES,    0x92
.equ UART,      0x85
.equ ASCII_0,   0x30

        .org 0x00
        JMP  debut
        .org 0x02
        RETI

        .org 0x04
debut:  LD   R0, ENTREE      ; R0 = valeur a convertir, puis le reste
        LDI  R7, 0x01        ; constante 1
        LDI  R1, 0x00        ; R1 = centaines
        LDI  R2, 0x00        ; R2 = dizaines

; ---- combien de fois 100 ? ----
        LDI  R3, 100
cent:   CMP  R0, R3          ; C = 1 si R0 < 100
        JC   fin_cent
        SUB  R0, R3
        ADD  R1, R7
        JMP  cent
fin_cent:
        ST   R1, CENTAINES

; ---- combien de fois 10 dans le reste ? ----
        LDI  R3, 10
diz:    CMP  R0, R3          ; C = 1 si R0 < 10
        JC   fin_diz
        SUB  R0, R3
        ADD  R2, R7
        JMP  diz
fin_diz:
        ST   R2, DIZAINES
        ST   R0, UNITES      ; ce qui reste, ce sont les unites

; ---- emission des trois chiffres en ASCII ----
        LDI  R4, ASCII_0
        ADD  R1, R4
        ST   R1, UART
        ADD  R2, R4
        ST   R2, UART
        ADD  R0, R4
        ST   R0, UART
        LDI  R4, 0x0A        ; retour a la ligne
        ST   R4, UART
        HALT
