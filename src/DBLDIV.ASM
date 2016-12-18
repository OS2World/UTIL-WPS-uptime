	TITLE	DBLDIV
	PAGE	66,132
;
; Assembler support for 'uptime' class.
;
; Bob Eager   October 2004
;
	.386
	.MODEL	FLAT,C,OS_OS2
;
; This module contains one function - 'dbldiv', which divides a 64 bit
; value by a 32 bit value, returning a 32 bit quotient and a 32 bit
; remainder.
;
;	Inputs:
;		NUMADR	(in EAX)	- address of input and output
;					  structure
;	Outputs:
;					- in same structure
	SUBTTL	Structure definitions
	PAGE+
;
; Structure definitions
;
IO	STRUCT				; Input/output structure
DENDHI	DD	?			; IN  - Dividend high
DENDLO	DD	?			; IN  - Dividend low
DIVISOR	DD	?			; IN  - Divisor
QUOT	DD	?			; OUT - Quotient
REMAIN	DD	?			; OUT - Remainder
IO	ENDS
;
	SUBTTL	Main code
	PAGE+
;
	.CODE
;
dbldiv	PROC	NEAR PUBLIC USES EBX, NUMADR
;
	MOV	EBX,EAX			; for use
	MOV	EDX,[IO PTR EBX].DENDHI	; get input value
	MOV	EAX,[IO PTR EBX].DENDLO	; get input value
	DIV	[IO PTR EBX].DIVISOR	; divide it
	MOV	[IO PTR EBX].QUOT,EAX	; quotient out
	MOV	[IO PTR EBX].REMAIN,EDX	; remainder out
;
; All done; exit.
;
	RET				; and return
;
dbldiv	ENDP
;
	END
