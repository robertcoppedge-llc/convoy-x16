; Resident trampoline. phase_run lives in bank B_PHASE (PHASECODE / boot/PHASE.BIN).
; Same five bytes as x16lib BANK_WRAPPER: Y = bank, then bank_jsr.
; resmap.inc is written into build/gen by tools/resmap from src/resmap.h.
.include "resmap.inc"
.export _phase_call
.import _bank_jsr

.segment "CODE"

_phase_call:
        ldy #B_PHASE
        jmp _bank_jsr
