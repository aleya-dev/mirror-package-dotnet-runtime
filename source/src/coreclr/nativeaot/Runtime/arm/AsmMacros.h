;; Licensed to the.NET Foundation under one or more agreements.
;; The.NET Foundation licenses this file to you under the MIT license.

;; OS provided macros
#include <ksarm.h>
;; generated by the build from AsmOffsets.cpp
#include "AsmOffsets.inc"

;;
;; CONSTANTS -- INTEGER
;;
TSF_Attached                    equ 0x01
TSF_SuppressGcStress            equ 0x08
TSF_DoNotTriggerGc              equ 0x10
TSF_SuppressGcStress__OR__TSF_DoNotTriggerGC equ 0x18

;; GC type flags
GC_ALLOC_FINALIZE               equ 1
GC_ALLOC_ALIGN8_BIAS            equ 4
GC_ALLOC_ALIGN8                 equ 8

;; GC minimal sized object. We use this to switch between 4 and 8 byte alignment in the GC heap (see AllocFast.asm).
SIZEOF__MinObject               equ 12
    ASSERT (SIZEOF__MinObject :MOD: 8) == 4

;; Note: these must match the defs in PInvokeTransitionFrameFlags
PTFF_SAVE_R4            equ 0x00000001
PTFF_SAVE_R5            equ 0x00000002
PTFF_SAVE_R6            equ 0x00000004
PTFF_SAVE_R7            equ 0x00000008
PTFF_SAVE_R8            equ 0x00000010
PTFF_SAVE_R9            equ 0x00000020
PTFF_SAVE_R10           equ 0x00000040
PTFF_SAVE_ALL_PRESERVED equ 0x00000077  ;; NOTE: FP is not included in this set!
PTFF_SAVE_SP            equ 0x00000100
PTFF_SAVE_R0            equ 0x00000200  ;; R0 is saved if it contains a GC ref and we're in hijack handler
PTFF_SAVE_ALL_SCRATCH   equ 0x00003e00  ;; R0-R3,LR (R12 is trashed by the helpers anyway, but LR is relevant for loop hijacking
PTFF_R0_IS_GCREF        equ 0x00004000  ;; iff PTFF_SAVE_R0: set -> r0 is Object, clear -> r0 is scalar
PTFF_R0_IS_BYREF        equ 0x00008000  ;; iff PTFF_SAVE_R0: set -> r0 is ByRef, clear -> r0 is Object or scalar
PTFF_THREAD_ABORT       equ 0x00010000  ;; indicates that ThreadAbortException should be thrown when returning from the transition

;; These must match the TrapThreadsFlags enum
TrapThreadsFlags_None            equ 0
TrapThreadsFlags_AbortInProgress equ 1
TrapThreadsFlags_TrapThreads     equ 2

;; This must match HwExceptionCode.STATUS_REDHAWK_THREAD_ABORT
STATUS_REDHAWK_THREAD_ABORT      equ 0x43

;;
;; Rename fields of nested structs
;;
OFFSETOF__Thread__m_alloc_context__alloc_ptr        equ OFFSETOF__Thread__m_rgbAllocContextBuffer + OFFSETOF__gc_alloc_context__alloc_ptr
OFFSETOF__Thread__m_alloc_context__alloc_limit      equ OFFSETOF__Thread__m_rgbAllocContextBuffer + OFFSETOF__gc_alloc_context__alloc_limit


__tls_array     equ 0x2C    ;; offsetof(TEB, ThreadLocalStoragePointer)

;;
;; MACROS
;;

        GBLS __SECTIONREL_tls_CurrentThread
__SECTIONREL_tls_CurrentThread SETS "SECTIONREL_tls_CurrentThread"

    MACRO
        INLINE_GETTHREAD $destReg, $trashReg
        EXTERN _tls_index

        ldr         $destReg, =_tls_index
        ldr         $destReg, [$destReg]
        mrc         p15, 0, $trashReg, c13, c0, 2
        ldr         $trashReg, [$trashReg, #__tls_array]
        ldr         $destReg, [$trashReg, $destReg, lsl #2]
        ldr         $trashReg, $__SECTIONREL_tls_CurrentThread
        add         $destReg, $trashReg
    MEND

        ;; INLINE_GETTHREAD_CONSTANT_POOL macro has to be used after the last function in the .asm file that used
        ;; INLINE_GETTHREAD. Optionally, it can be also used after any function that used INLINE_GETTHREAD
        ;; to improve density, or to reduce distance between the constant pool and its use.
    MACRO
        INLINE_GETTHREAD_CONSTANT_POOL
        EXTERN tls_CurrentThread

$__SECTIONREL_tls_CurrentThread
        DCD tls_CurrentThread
        RELOC 15 ;; SECREL

__SECTIONREL_tls_CurrentThread SETS "$__SECTIONREL_tls_CurrentThread":CC:"_"

    MEND

    MACRO
        INLINE_THREAD_UNHIJACK $threadReg, $trashReg1, $trashReg2
        ;;
        ;; Thread::Unhijack()
        ;;
        ldr         $trashReg1, [$threadReg, #OFFSETOF__Thread__m_pvHijackedReturnAddress]
        cbz         $trashReg1, %ft0

        ldr         $trashReg2, [$threadReg, #OFFSETOF__Thread__m_ppvHijackedReturnAddressLocation]
        str         $trashReg1, [$trashReg2]
        mov         $trashReg1, #0
        str         $trashReg1, [$threadReg, #OFFSETOF__Thread__m_ppvHijackedReturnAddressLocation]
        str         $trashReg1, [$threadReg, #OFFSETOF__Thread__m_pvHijackedReturnAddress]
0
    MEND

DEFAULT_FRAME_SAVE_FLAGS equ PTFF_SAVE_ALL_PRESERVED + PTFF_SAVE_SP

;;
;; Macro used from unmanaged helpers called from managed code where the helper does not transition immediately
;; into pre-emptive mode but may cause a GC and thus requires the stack is crawlable. This is typically the
;; case for helpers that meddle in GC state (e.g. allocation helpers) where the code must remain in
;; cooperative mode since it handles object references and internal GC state directly but a garbage collection
;; may be inevitable. In these cases we need to be able to transition to pre-meptive mode deep within the
;; unmanaged code but still be able to initialize the stack iterator at the first stack frame which may hold
;; interesting GC references. In all our helper cases this corresponds to the most recent managed frame (e.g.
;; the helper's caller).
;;
;; This macro builds a frame describing the current state of managed code.
;;
;; INVARIANTS
;; - The macro assumes it defines the method prolog, it should typically be the first code in a method and
;;   certainly appear before any attempt to alter the stack pointer.
;; - This macro uses trashReg (after its initial value has been saved in the frame) and upon exit trashReg
;;   will contain the address of transition frame.
;;
    MACRO
        PUSH_COOP_PINVOKE_FRAME $trashReg

        PROLOG_STACK_ALLOC 4        ; Save space for caller's SP
        PROLOG_PUSH {r4-r6,r8-r10}  ; Save preserved registers
        PROLOG_STACK_ALLOC 8        ; Save space for flags and Thread*
        PROLOG_PUSH {r7}            ; Save caller's FP
        PROLOG_PUSH {r11,lr}        ; Save caller's frame-chain pointer and PC

        ; Compute SP value at entry to this method and save it in the last slot of the frame (slot #11).
        add         $trashReg, sp, #(12 * 4)
        str         $trashReg, [sp, #(11 * 4)]

        ; Record the bitmask of saved registers in the frame (slot #4).
        mov         $trashReg, #DEFAULT_FRAME_SAVE_FLAGS
        str         $trashReg, [sp, #(4 * 4)]

        mov         $trashReg, sp
    MEND

;; Pop the frame and restore register state preserved by PUSH_COOP_PINVOKE_FRAME
    MACRO
        POP_COOP_PINVOKE_FRAME
        EPILOG_POP  {r11,lr}        ; Restore caller's frame-chain pointer and PC (return address)
        EPILOG_POP  {r7}            ; Restore caller's FP
        EPILOG_STACK_FREE 8         ; Discard flags and Thread*
        EPILOG_POP  {r4-r6,r8-r10}  ; Restore preserved registers
        EPILOG_STACK_FREE 4         ; Discard caller's SP
    MEND


; Macro used to assign an alternate name to a symbol containing characters normally disallowed in a symbol
; name (e.g. C++ decorated names).
    MACRO
      SETALIAS   $name, $symbol
        GBLS    $name
$name   SETS    "|$symbol|"
    MEND


        ;
        ; Helper macro: create a global label for the given name,
        ; decorate it, and export it for external consumption.
        ;

        MACRO
        __ExportLabelName $FuncName

        LCLS    Name
Name    SETS    "|$FuncName|"
        EXPORT  $Name
$Name
        MEND

        ;
        ; Macro for indicating an alternate entry point into a function.
        ;

        MACRO
        LABELED_RETURN_ADDRESS $ReturnAddressName

        ; export the return address name, but do not perturb the code by forcing alignment
        __ExportLabelName $ReturnAddressName

        ; flush any pending literal pool stuff
        ROUT

        MEND

        MACRO
        EXPORT_POINTER_TO_ADDRESS $Name

1

        AREA        |.rdata|, ALIGN=4, DATA, READONLY

$Name

        DCD         %BT1

        EXPORT      $Name

        TEXTAREA

        ROUT

        MEND

;-----------------------------------------------------------------------------
; Macro used to check (in debug builds only) whether the stack is 64-bit aligned (a requirement before calling
; out into C++/OS code). Invoke this directly after your prolog (if the stack frame size is fixed) or directly
; before a call (if you have a frame pointer and a dynamic stack). A breakpoint will be invoked if the stack
; is misaligned.
;
    MACRO
        CHECK_STACK_ALIGNMENT

#ifdef _DEBUG
        push    {r0}
        add     r0, sp, #4
        tst     r0, #7
        pop     {r0}
        beq     %F0
        EMIT_BREAKPOINT
0
#endif
    MEND

;; Loads a 32bit constant into destination register
    MACRO
        MOV32   $destReg, $constant

        movw    $destReg, #(($constant) & 0xFFFF)
        movt    $destReg, #(($constant) >> 16)
    MEND

;;
;; CONSTANTS -- SYMBOLS
;;

        SETALIAS G_LOWEST_ADDRESS, g_lowest_address
        SETALIAS G_HIGHEST_ADDRESS, g_highest_address
        SETALIAS G_EPHEMERAL_LOW, g_ephemeral_low
        SETALIAS G_EPHEMERAL_HIGH, g_ephemeral_high
        SETALIAS G_CARD_TABLE, g_card_table
        SETALIAS G_FREE_OBJECT_EETYPE, ?g_pFreeObjectEEType@@3PAVEEType@@A
#ifdef FEATURE_GC_STRESS
        SETALIAS THREAD__HIJACKFORGCSTRESS, ?HijackForGcStress@Thread@@SAXPAUPAL_LIMITED_CONTEXT@@@Z
        SETALIAS REDHAWKGCINTERFACE__STRESSGC, ?StressGc@RedhawkGCInterface@@SAXXZ
#endif ;; FEATURE_GC_STRESS
;;
;; IMPORTS
;;
        EXTERN RhpGcAlloc
        EXTERN RhDebugBreak
        EXTERN RhpWaitForGC2
        EXTERN RhExceptionHandling_FailedAllocation


        EXTERN $G_LOWEST_ADDRESS
        EXTERN $G_HIGHEST_ADDRESS
        EXTERN $G_EPHEMERAL_LOW
        EXTERN $G_EPHEMERAL_HIGH
        EXTERN $G_CARD_TABLE
        EXTERN RhpTrapThreads
        EXTERN $G_FREE_OBJECT_EETYPE

        EXTERN RhThrowHwEx
        EXTERN RhThrowEx
        EXTERN RhRethrow

#ifdef FEATURE_GC_STRESS
        EXTERN $REDHAWKGCINTERFACE__STRESSGC
        EXTERN $THREAD__HIJACKFORGCSTRESS
#endif ;; FEATURE_GC_STRESS
