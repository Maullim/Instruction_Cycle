        ORG 000             / Origin of program is location 0
        LDA A               / Load first operand into AC
        ADD B               / Add second operand to AC
        STA C               / Store sum in location C
        HLT                 / Halt computer
A,      DEC 83              / First operand (decimal 83 = 0x0053)
B,      DEC -23             / Second operand (negative, -23 = 0xFFE9)
C,      HEX 0000            / Sum stored here
        END                 / End of symbolic program
