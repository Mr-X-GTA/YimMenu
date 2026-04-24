EXTERN process_script_function_hooks:PROC
EXTERN g_script_vm_on_enter_end_jump_offset:QWORD

.code

script_vm_on_enter_end_handler PROC
    ; fix the stack
    add rsp, 8

    ; save stuff
    pushfq
    push r11
    push r10
    push r9
    push r8
    push rdx
    push rcx
    push rbx
    push rsi
    push rdi

    ; setup args and call our function
    mov rcx, rsp ; ip
    lea rdx, [rsp + 8] ; base
    lea r8, [rsp + 16] ; sp
    mov r9, r14 ; fp
    sub rsp, 30h
    mov [rsp + 20h], r15 ; ctx
    mov [rsp + 28h], r13 ; code
    call process_script_function_hooks
    add rsp, 30h

    ; restore stuff
    pop rdi
    pop rsi
    pop rbx
    pop rcx
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r11
    popfq

    ; original logic (jump to next case)
    mov rax, qword ptr [g_script_vm_on_enter_end_jump_offset]
    jmp rax
script_vm_on_enter_end_handler ENDP

END