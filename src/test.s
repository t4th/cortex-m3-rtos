
            AREA    |.text|, CODE, READONLY
                
                extern kernel
                    
                ALIGN
Contex_Switch   FUNCTION ;PROC
                EXPORT  Contex_Switch                [WEAK]
                AND r0, #1
                ;mov r4, [kernel]
                LDR r4, =kernel
                ENDFUNC ;ENDP

                ALIGN
    END