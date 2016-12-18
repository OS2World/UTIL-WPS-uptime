 	PAGE	64,132
	TITLE	UPTIME - device driver to return system uptime
	NAME	UPTIME
	.286
;
; Description:
;	OS/2 device driver for returning system uptime in seconds,
;	as a 32 bit quuantity.
;	Takes no arguments.
;
; Version:
;	1.3
;
; Date:
;	13th October 2004
;
; Author:
;	R D Eager
;
; History:
;	1.3	Initial version
;
; Copyright and License
;
; This Software and its documentation are Copyright, 2004 by the Author:
;			Bob Eager
;			45 Fleetwood Avenue
;			Herne Bay
;			United Kingdom
;			CT6 8QW
;
;			rde@tavi.co.uk
;
; License is granted to User to duplicate and disseminate this software
; product, and to use on one computer running OS/2, PROVIDED user agrees to
; 1) hold Author free of any and all liability for any consequences of use
; of the Software, 2) copy the UPTIME.DOC file and retain it with any copy
; of UPTIME.SYS copied to any medium, and 3) not charge any other person or
; organization for such copies.
;
	.XLIST
	INCLUDE	DEVCMD.INC
	INCLUDE	DEVHLP.INC
	INCLUDE	DEVSYM.INC
	.LIST
;
; Constants
;
STDOUT		EQU	1		; Standard output handle
;
TAB		EQU	09H		; Tab character
CR		EQU	0DH		; Carriage return
LF		EQU	0AH		; Linefeed
;
; External references
;
	EXTRN	DosWrite:FAR
;
	SUBTTL	Data areas
	PAGE+
;
DGROUP	GROUP	_DATA
;
_DATA	SEGMENT	WORD PUBLIC 'DATA'
;
; Device driver header
;
HEADER	DD	-1			; Link to next device driver
	DW	1000100110000000B	; Device attributes:
;		|   | |||_______________;  function level 011 (3)
;		|   |___________________;  device open/close supported
;		|__ ____________________;  character device
	DW	OFFSET STRATEGY		; Strategy entry point
	DW	0			; IDC entry point - not used
	DB	'UPTIME$ '		; Device name
	DB	8 DUP (0)		; Reserved
	DW	00000010B		; Capabilities bit strip
;		      |_________________; Supports 32 bit addressing
;
; Command function jump table
;
FUNTAB	DW	INIT			; 00H - Initialisation
	DW	BADCMD			; 01H - Media check
	DW	BADCMD			; 02H - Build BPB
	DW	BADCMD			; 03H - Reserved
	DW	INPUT			; 04H - Input
	DW	INPUT			; 05H - Non destructive input
	DW	FLSTAT			; 06H - Input status
	DW	FLSTAT			; 07H - Input flush
	DW	BADCMD			; 08H - Output
	DW	BADCMD			; 09H - Output with verify
	DW	BADCMD			; 0AH - Output status
	DW	BADCMD			; 0BH - Output flush
	DW	BADCMD			; 0CH - Reserved
	DW	OPCLOSE			; 0DH - Open device
	DW	OPCLOSE			; 0EH - Close device
;
MAXCMD	EQU	($ - FUNTAB - 1) SHR 1	; Highest legal command code
;
	DW	BADCMD			; Pseudo function - command out of range
;
; Permanent data
;
PTIME	DD	?			; Pointer to time DWORD in global infoseg
BASE	DW	2 DUP (?)		; Base time (boot time) in seconds
;
DevHlp	DD	?			; Entry point to DevHlp
;
	SUBTTL	Initialisation data
	PAGE+
;
; The following data are used only during initialisation, and are then
; discarded.
;
DINIT	EQU	THIS WORD
;
WLEN	DW	?			; Receives DosWrite length
FIRED	DW	?			; Timer fired flag
GISSEG	DW	?			; Segment of global infoseg pointer
GISOFF	DW	?			; Offset of global infoseg pointer
MES1	DB	'UPTIME driver - invalid argument',CR,LF,0
;
	DB	'*** Copyright (C) R D Eager  2004 ***'
;
_DATA	ENDS
;
	SUBTTL	Main code
	PAGE+
;
_TEXT	SEGMENT	WORD PUBLIC 'CODE'
;
	ASSUME	CS:_TEXT,DS:DGROUP,ES:NOTHING
;
; Strategy entry point
;
; Inputs:
;	ES:BX	- points to the request packet
;
; Outputs:
;	ES:BX.PktStatus
;		- status
;
STRATEGY	PROC	FAR
;
	MOV	AL,ES:[BX].PktCmd	; get function code
	CMP	AL,MAXCMD		; check range
	JLE	CMDOK			; j if OK
	MOV	AL,MAXCMD+1		; pseudo code for error
;
CMDOK:	ROL	AL,1			; form word offset
	CBW				; convert for index
	MOV	SI,AX
	CALL	[SI+FUNTAB]		; call requested function
;
; Function exit code - sets up status and exits
; At this point, AX contains the required status.
; We assume that ES:BX still point to the request header.
;
EXIT:	OR	AX,AX			; check return code
	JZ	NOERR			; j if no error
	CMP	AH,02H			; just busy?
	JE	NOERR			; OK too
;
ERR:	OR	AH,80H			; set error bit
;
NOERR:	OR	AH,01H			; set done bit
	MOV	ES:[BX].PktStatus,AX	; store status in request packet
	RET				; return to system
;
STRATEGY	ENDP
;
; Bad command handler; simply sets status and returns.
;
BADCMD	PROC	NEAR
;
	MOV	AX,0003H		; unknown command
	RET				; return to outer level
;
BADCMD	ENDP
;
	SUBTTL	Open or close device
	PAGE+
;
; Device open and close code. Always successful, and does nothing.
; Monitor opens and closes are, however, failed.
;
; Inputs:
;	ES:BX	- points to the request packet
;
; Outputs:
;	AX	- status
;
OPCLOSE	PROC	NEAR
;
	XOR	AX,AX			; assume success
	TEST	ES:[BX].PktStatus,08H	; monitor open or close?
	JZ	OPC10			; j if not
	MOV	AL,12H			; monitors not supported
;
OPC10:	RET				; return with status in AX
;
OPCLOSE	ENDP
;
	SUBTTL	Do input
	PAGE+
;
; Perform input.
;
; Inputs:
;	ES:BX	- points to the request packet
;
; Outputs:
;	AX	- status
;
INPUT	PROC	NEAR
;
	MOV	CX,ES:[BX].IOcount	; get input count
;
; Call DevHlp to convert buffer address
;
IN10:	PUSH	ES			; save registers
	PUSH	BX
	MOV	AX,WORD PTR ES:[BX].IOpData+2
					; high word of transfer address
	MOV	BX,WORD PTR ES:[BX].IOpData
					; low word of transfer address
	MOV	DX,(1 SHL 8) OR DevHlp_PhysToVirt
					; DL=function required
					; DH=1 => result in ES:DI
	CALL	DevHlp			; convert it
	POP	BX			; recover register
	JNC	IN20			; j if OK
;
IN15:	POP	ES			; recover register
	MOV	ES:[BX].IOcount,0	; length of transfer
	MOV	AX,0013H		; general failure
	JMP	SHORT IN40		; and return
;
; Now do the input.
;
IN20:	OR	CX,CX			; check for zero transfer
	JNZ	IN21			; j if something to do
	POP	ES			; else recover register
	MOV	ES:[BX].IOcount,CX	; set bytes transferred
	JMP	SHORT IN35		; and return with success
;
IN21:	CMP	CX,4			; check for short transfer
	JL	IN15			; j if too short
	PUSH	BX			; save registers
	PUSH	ES
	LES	BX,PTIME		; point to time
	MOV	AX,ES:[BX]		; get low time
	MOV	DX,ES:[BX+2]		; get high time
	SUB	AX,BASE			; get difference
	SBB	DX,BASE+2
	POP	ES			; recover register
	CLD
	STOSW				; pass back low order word
	MOV	AX,DX
	STOSW				; pass back high order word
	POP	BX			; recover registers
	POP	ES
;
	MOV	ES:[BX].IOcount,4	; 4 bytes transferred
;
IN35:	XOR	AX,AX			; success
;
IN40:	RET				; return with status in AX
;
INPUT	ENDP
;
	SUBTTL	I/O status and flush
	PAGE+
;
; Return input or output status, and flush input or output.
; Since input happens at once, is never busy for input.
; Since there is never any output, is never busy for output.
; Since there is never any input to flush, just returns success and not busy.
; Since there is never any output to flush, just returns success, and not busy.
;
; Inputs:
;	ES:BX	- points to the request packet
;
; Outputs:
;	AX	- status
;
FLSTAT	PROC	NEAR
;
	XOR	AX,AX			; success, not busy
;
	RET				; return with status in AX
;
FLSTAT	ENDP
;
	SUBTTL	Initialisation code
	PAGE+
;
; Initialisation code. All of this code is present only during initialisation;
; none of the driver data is used after that time either.
;
; This code runs at ring 3.
;
; ES:BX points to the request packet.
; Status is returned in AX.
;
	ASSUME	CS:_TEXT,DS:DGROUP,ES:NOTHING
;
INIT	PROC	NEAR
;
; Process the INIT arguments
;
	PUSH	DS			; save data segment for later
;
	MOV	AX,WORD PTR ES:[BX].InitDevHlp
					; offset of DevHlp entry point
	MOV	WORD PTR DevHlp,AX	; save it
	MOV	AX,WORD PTR ES:[BX].InitDevHlp+2
					; segment of DevHlp entry point
	MOV	WORD PTR DevHlp+2,AX	; save it
	MOV	SI,WORD PTR ES:[BX].InitParms
					; offset of INIT arguments
	MOV	DS,WORD PTR ES:[BX].InitParms+2
					; segment of INIT arguments
;
	ASSUME	CS:_TEXT,DS:NOTHING,ES:NOTHING
;
	CLD				; autoincrement
;
INIT10:	LODSB				; skip leading whitespace
	CMP	AL,' '
	JE	INIT10
	CMP	AL,TAB
	JE	INIT10
	DEC	SI			; back to first non-space
;
INIT20:	LODSB				; skip filename
	CMP	AL,' '
	JE	SHORT INIT30		; found next separator
	CMP	AL,TAB
	JE	SHORT INIT30		; found next separator
	CMP	AL,0			; found terminator?
	JE	SHORT INIT35		; j if so
	JMP	INIT20			; else keep looking
;
INIT30:	LODSB				; skip separating whitespace
	CMP	AL,' '
	JE	INIT30
	CMP	AL,TAB
	JE	INIT30
;
INIT35:	POP	DS			; recover data segment
;
	ASSUME	CS:_TEXT,DS:DGROUP,ES:NOTHING
;
	CMP	AL,0			; found terminator?
	JNE	INIT50			; j if not - error
;
; OK - now get address of global infoseg and store it.
;
	PUSH	BX			; save register
;
	MOV	AL,DHGETDOSV_SYSINFOSEG	; index of item required
	MOV	DL,DevHlp_GetDOSVar	; function required
	CALL	DevHlp			; do it
;
; AX:BX points to segment address of global infoseg. We cannot do any more
; with this in INIT context, so we register a timer routine to carry out the
; necessary actions in interrupt context.
;
	MOV	GISSEG,AX		; save segment
	MOV	GISOFF,BX		; and offset
	MOV	FIRED,0			; timer not fired yet
	MOV	AX,OFFSET TIMER		; set up for DevHlp call
	MOV	BX,1			; number of ticks - short as possible
	MOV	DL,DevHlp_TickCount	; required function
	CALL	DevHlp			; register timer
;
; Wait for timer to fire before continuing.
;
INIT45:	TEST	FIRED,1			; timer fired?
	JZ	INIT45			; j if not - keep waiting
;
; End of wait; deregister timer routine
;
	MOV	AX,OFFSET TIMER		; set up for DevHlp call
	MOV	DL,DevHlp_ResetTimer	; required function
	CALL	DevHlp
;
	POP	BX			; recover register
;
; Arrange to lose the initialisation code and data
;
	MOV	WORD PTR ES:[BX].InitEcode,OFFSET _TEXT:INIT
					; truncate code segment
	MOV	WORD PTR ES:[BX].InitEdata,OFFSET _DATA:DINIT
					; truncate data segment
	XOR	AX,AX			; success status
	RET
;
; Invalid argument (any at all) detected
;
INIT50:	MOV	AX,OFFSET MES1		; error message
	CALL	MESOUT			; display it
	MOV	WORD PTR ES:[BX].InitEcode,0
					; lose code segment
	MOV	WORD PTR ES:[BX].InitEdata,0
					; lose data segment
	MOV	AX,810CH		; error/done/general failure
	RET
;
INIT	ENDP
;
	SUBTTL	Timer handler
	PAGE+
;
; Timer routine; called when timer has ticked.
;
TIMER	PROC	FAR
;
	PUSH	AX			; save registers
	PUSH	BX
	PUSH	ES
	MOV	AX,GISSEG		; segment of global infoseg pointer
	MOV	ES,AX			; set up for segment access
	MOV	BX,GISOFF		; get offset too
	MOV	AX,ES:[BX]		; get segment address
;
; Since the time (seconds since 1st January 1970) is the first item
; in the global infoseg, that's all we need.
;
	MOV	WORD PTR PTIME+2,AX	; save for later
	XOR	BX,BX			; offset for later use
	MOV	WORD PTR PTIME,BX	; save it
	MOV	ES,AX			; address the segment right now
	MOV	AX,ES:[BX]		; low order base time
	MOV	BASE,AX			; save it
	MOV	AX,ES:[BX+2]		; high order base time
	MOV	BASE+2,AX		; save it
	MOV	FIRED,1			; say we've done it
	POP	ES			; recover registers
	POP	BX
	POP	AX
	RET				; and return
;
TIMER	ENDP
;
	SUBTTL	Output message
	PAGE+
;
; Routine to output a string to the screen.
;
; Inputs:
;	AX	- offset of zero terminated message
;
; Outputs:
;	AX	- not preserved
;
MESOUT	PROC	NEAR
;
	PUSH	DI			; save DI
	PUSH	CX			; save CX
	PUSH	ES			; save ES
	PUSH	AX			; save message offset
	PUSH	DS			; copy DS...
	POP	ES			; ...to ES
	MOV	DI,AX			; ES:DI point to message
	XOR	AL,AL			; set AL=0 for scan value
	MOV	CX,100			; just a large value
	REPNZ	SCASB			; scan for zero byte
	POP	AX			; recover message offset
	POP	ES			; recover ES
	POP	CX			; recover CX
	SUB	DI,AX			; get size to DI
	DEC	DI			; adjust
	PUSH	STDOUT			; standard output handle
	PUSH	DS			; segment of message
	PUSH	AX			; offset of message
	PUSH	DI			; length of message
	PUSH	DS			; segment for length written
	PUSH	OFFSET DGROUP:WLEN	; offset for length written
	CALL	DosWrite		; write message
	POP	DI			; recover DI
;
	RET
;
MESOUT	ENDP
;
_TEXT	ENDS
;
	END
