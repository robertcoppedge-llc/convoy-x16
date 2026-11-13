; Resident trampoline. phase_run lives in bank 63 (PHASECODE / boot/phase.bin).
; Same five bytes as x16lib BANK_WRAPPER: Y = bank, then bank_jsr.
.export _phase_call
.import _bank_jsr

.segment "CODE"

_phase_call:
        ldy #63
        jmp _bank_jsr
