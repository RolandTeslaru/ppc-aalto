step(float*, float const*, int):
        test    edx, edx
        jle     .L13
        push    r12
        movsx   r11, edx
        lea     rcx, [0+r11*4]
        push    rbp
        vmovss  xmm2, DWORD PTR .LC0[rip]
        mov     rbp, rsi
        push    rbx
        mov     r10, rdi
        mov     r12d, edx
        lea     rsi, [rsi+rcx]
        mov     r9, rbp
        xor     ebx, ebx
.L3:
        mov     r8, rbp
        xor     edi, edi
.L7:
        mov     rdx, r8
        mov     rax, r9
        vmovaps xmm1, xmm2
.L6:
        vmovss  xmm0, DWORD PTR [rax]
        add     rax, 4
        vaddss  xmm0, xmm0, DWORD PTR [rdx]
        add     rdx, rcx
        vminss  xmm1, xmm0, xmm1
        cmp     rsi, rax
        jne     .L6
        vmovss  DWORD PTR [r10+rdi*4], xmm1
        inc     rdi
        add     r8, 4
        cmp     r11, rdi
        jne     .L7
        inc     ebx
        add     r10, rcx
        add     rsi, rcx
        add     r9, rcx
        cmp     r12d, ebx
        jne     .L3
        pop     rbx
        pop     rbp
        pop     r12
        ret
.L13:
        ret
.LC18:
        .string "Time: "
.LC19:
        .string " seconds\n"
.LC20:
        .string "Useful operations/second: "
.LC22:
        .string " billion\n"
.LC23:
        .string "Checksum: "
main:
        push    r13
        mov     edi, 36
        push    r12
        push    rbp
        push    rbx
        sub     rsp, 5032
        call    operator new(unsigned long)
        mov     DWORD PTR [rax], 0x00000000
        vpxor   xmm0, xmm0, xmm0
        vmovdqu XMMWORD PTR [rax+4], xmm0
        vmovdqu XMMWORD PTR [rax+20], xmm0
        mov     edi, 36
        mov     rbx, rax
        call    operator new(unsigned long)
        mov     DWORD PTR [rax], 0x00000000
        mov     QWORD PTR [rsp+16], 42
        vpxor   xmm0, xmm0, xmm0
        mov     rbp, rax
        vmovdqu XMMWORD PTR [rax+4], xmm0
        vmovdqu XMMWORD PTR [rax+20], xmm0
        mov     ecx, 42
        mov     edx, 1
.L28:
        mov     rax, rcx
        shr     rax, 30
        xor     rax, rcx
        imul    rax, rax, 1812433253
        lea     ecx, [rax+rdx]
        mov     QWORD PTR [rsp+16+rdx*8], rcx
        inc     rdx
        cmp     rdx, 624
        jne     .L28
        lea     rdi, [rsp+16]
        mov     QWORD PTR [rsp+5008], 624
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rax, QWORD PTR [rsp+5008]
        vxorps  xmm0, xmm0, xmm0
        lea     rcx, [rax+1]
        mov     rax, QWORD PTR [rsp+16+rax*8]
        mov     QWORD PTR [rsp+5008], rcx
        mov     rdx, rax
        shr     rdx, 11
        mov     edx, edx
        xor     rdx, rax
        mov     rax, rdx
        sal     rax, 7
        and     eax, 2636928640
        xor     rdx, rax
        mov     rax, rdx
        sal     rax, 15
        and     eax, 4022730752
        xor     rax, rdx
        mov     rdx, rax
        shr     rdx, 18
        xor     rax, rdx
        js      .L29
        vcvtsi2ss       xmm1, xmm0, rax
.L30:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmulss  xmm1, xmm1, xmm3
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx], xmm1
        cmp     rcx, 623
        ja      .L82
.L32:
        lea     rdx, [rcx+1]
        mov     rcx, QWORD PTR [rsp+16+rcx*8]
        mov     QWORD PTR [rsp+5008], rdx
        mov     rax, rcx
        shr     rax, 11
        mov     eax, eax
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 7
        and     ecx, 2636928640
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 15
        and     ecx, 4022730752
        xor     rax, rcx
        mov     rcx, rax
        shr     rcx, 18
        xor     rax, rcx
        js      .L33
        vcvtsi2ss       xmm1, xmm0, rax
.L34:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vmulss  xmm1, xmm1, xmm3
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+4], xmm1
        cmp     rdx, 623
        ja      .L83
.L36:
        mov     rax, QWORD PTR [rsp+16+rdx*8]
        lea     rcx, [rdx+1]
        mov     rdx, rax
        shr     rdx, 11
        mov     edx, edx
        xor     rax, rdx
        mov     rdx, rax
        sal     rdx, 7
        and     edx, 2636928640
        xor     rax, rdx
        mov     rdx, rax
        sal     rdx, 15
        and     edx, 4022730752
        xor     rax, rdx
        mov     rdx, rax
        shr     rdx, 18
        mov     QWORD PTR [rsp+5008], rcx
        xor     rax, rdx
        js      .L37
        vcvtsi2ss       xmm1, xmm0, rax
.L38:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vmulss  xmm1, xmm1, xmm3
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+8], xmm1
        cmp     rcx, 623
        ja      .L84
.L40:
        mov     rax, QWORD PTR [rsp+16+rcx*8]
        lea     rdx, [rcx+1]
        mov     rcx, rax
        shr     rcx, 11
        mov     ecx, ecx
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 7
        and     ecx, 2636928640
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 15
        and     ecx, 4022730752
        xor     rax, rcx
        mov     rcx, rax
        shr     rcx, 18
        mov     QWORD PTR [rsp+5008], rdx
        xor     rax, rcx
        js      .L41
        vcvtsi2ss       xmm1, xmm0, rax
.L42:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vmulss  xmm1, xmm1, xmm3
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+12], xmm1
        cmp     rdx, 623
        ja      .L85
.L44:
        mov     rax, QWORD PTR [rsp+16+rdx*8]
        lea     rcx, [rdx+1]
        mov     rdx, rax
        shr     rdx, 11
        mov     edx, edx
        xor     rax, rdx
        mov     rdx, rax
        sal     rdx, 7
        and     edx, 2636928640
        xor     rax, rdx
        mov     rdx, rax
        sal     rdx, 15
        and     edx, 4022730752
        xor     rax, rdx
        mov     rdx, rax
        shr     rdx, 18
        mov     QWORD PTR [rsp+5008], rcx
        xor     rax, rdx
        js      .L45
        vcvtsi2ss       xmm1, xmm0, rax
.L46:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vmulss  xmm1, xmm1, xmm3
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+16], xmm1
        cmp     rcx, 623
        ja      .L86
.L48:
        mov     rax, QWORD PTR [rsp+16+rcx*8]
        lea     rdx, [rcx+1]
        mov     rcx, rax
        shr     rcx, 11
        mov     ecx, ecx
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 7
        and     ecx, 2636928640
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 15
        and     ecx, 4022730752
        xor     rax, rcx
        mov     rcx, rax
        shr     rcx, 18
        mov     QWORD PTR [rsp+5008], rdx
        xor     rax, rcx
        js      .L49
        vcvtsi2ss       xmm1, xmm0, rax
.L50:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vmulss  xmm1, xmm1, xmm3
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+20], xmm1
        cmp     rdx, 623
        ja      .L87
.L52:
        mov     rax, QWORD PTR [rsp+16+rdx*8]
        lea     rcx, [rdx+1]
        mov     rdx, rax
        shr     rdx, 11
        mov     edx, edx
        xor     rax, rdx
        mov     rdx, rax
        sal     rdx, 7
        and     edx, 2636928640
        xor     rax, rdx
        mov     rdx, rax
        sal     rdx, 15
        and     edx, 4022730752
        xor     rax, rdx
        mov     rdx, rax
        shr     rdx, 18
        mov     QWORD PTR [rsp+5008], rcx
        xor     rax, rdx
        js      .L53
        vcvtsi2ss       xmm1, xmm0, rax
.L54:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmovss  xmm5, DWORD PTR .LC12[rip]
        vmulss  xmm1, xmm1, xmm3
        vcmpless        xmm6, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm5, xmm6
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+24], xmm1
        cmp     rcx, 623
        ja      .L88
.L56:
        mov     rax, QWORD PTR [rsp+16+rcx*8]
        lea     rdx, [rcx+1]
        mov     rcx, rax
        shr     rcx, 11
        mov     ecx, ecx
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 7
        and     ecx, 2636928640
        xor     rax, rcx
        mov     rcx, rax
        sal     rcx, 15
        and     ecx, 4022730752
        xor     rax, rcx
        mov     rcx, rax
        shr     rcx, 18
        mov     QWORD PTR [rsp+5008], rdx
        xor     rax, rcx
        js      .L57
        vcvtsi2ss       xmm1, xmm0, rax
.L58:
        vxorps  xmm4, xmm4, xmm4
        vaddss  xmm1, xmm1, xmm4
        vmulss  xmm1, xmm1, xmm3
        vmovss  xmm3, DWORD PTR .LC12[rip]
        vcmpless        xmm2, xmm2, xmm1
        vblendvps       xmm1, xmm1, xmm3, xmm2
        vaddss  xmm1, xmm1, xmm4
        vmovss  DWORD PTR [rbx+28], xmm1
        cmp     rdx, 623
        ja      .L89
.L60:
        mov     DWORD PTR [rbx], 0x00000000
        mov     DWORD PTR [rbx+16], 0x00000000
        mov     DWORD PTR [rbx+32], 0x00000000
        inc     rdx
        mov     QWORD PTR [rsp+5008], rdx
        call    std::chrono::_V2::steady_clock::now()
        mov     edx, 3
        mov     rsi, rbx
        mov     rdi, rbp
        mov     r12, rax
        call    step(float*, float const*, int)
        call    std::chrono::_V2::steady_clock::now()
        sub     rax, r12
        vxorps  xmm0, xmm0, xmm0
        vcvtsi2sd       xmm1, xmm0, rax
        vxorpd  xmm2, xmm2, xmm2
        mov     edx, 6
        mov     esi, OFFSET FLAT:.LC18
        vdivsd  xmm7, xmm1, QWORD PTR .LC16[rip]
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+0]
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+4]
        vmovq   r13, xmm7
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+8]
        mov     edi, OFFSET FLAT:std::cout
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+12]
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+16]
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+20]
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+24]
        vaddsd  xmm2, xmm1, xmm2
        vcvtss2sd       xmm1, xmm0, DWORD PTR [rbp+28]
        vcvtss2sd       xmm0, xmm0, DWORD PTR [rbp+32]
        vaddsd  xmm1, xmm1, xmm2
        vaddsd  xmm7, xmm0, xmm1
        vmovq   r12, xmm7
        call    std::basic_ostream<char, std::char_traits<char>>& std::__ostream_insert<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&, char const*, long)
        vmovq   xmm0, r13
        mov     edi, OFFSET FLAT:std::cout
        call    std::ostream& std::ostream::_M_insert<double>(double)
        mov     rdi, rax
        mov     edx, 9
        mov     esi, OFFSET FLAT:.LC19
        call    std::basic_ostream<char, std::char_traits<char>>& std::__ostream_insert<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&, char const*, long)
        mov     edx, 26
        mov     esi, OFFSET FLAT:.LC20
        mov     edi, OFFSET FLAT:std::cout
        call    std::basic_ostream<char, std::char_traits<char>>& std::__ostream_insert<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&, char const*, long)
        vmovsd  xmm0, QWORD PTR .LC21[rip]
        vmovq   xmm7, r13
        vdivsd  xmm0, xmm0, xmm7
        mov     edi, OFFSET FLAT:std::cout
        vdivsd  xmm0, xmm0, QWORD PTR .LC16[rip]
        call    std::ostream& std::ostream::_M_insert<double>(double)
        mov     rdi, rax
        mov     edx, 9
        mov     esi, OFFSET FLAT:.LC22
        call    std::basic_ostream<char, std::char_traits<char>>& std::__ostream_insert<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&, char const*, long)
        mov     edx, 10
        mov     esi, OFFSET FLAT:.LC23
        mov     edi, OFFSET FLAT:std::cout
        call    std::basic_ostream<char, std::char_traits<char>>& std::__ostream_insert<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&, char const*, long)
        vmovq   xmm0, r12
        mov     edi, OFFSET FLAT:std::cout
        call    std::ostream& std::ostream::_M_insert<double>(double)
        mov     rdi, rax
        mov     rax, QWORD PTR [rax]
        mov     BYTE PTR [rsp+15], 10
        mov     rax, QWORD PTR [rax-24]
        cmp     QWORD PTR [rdi+16+rax], 0
        je      .L64
        mov     edx, 1
        lea     rsi, [rsp+15]
        call    std::basic_ostream<char, std::char_traits<char>>& std::__ostream_insert<char, std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>>&, char const*, long)
.L65:
        mov     rdi, rbp
        mov     esi, 36
        call    operator delete(void*, unsigned long)
        mov     rdi, rbx
        mov     esi, 36
        call    operator delete(void*, unsigned long)
        add     rsp, 5032
        pop     rbx
        pop     rbp
        pop     r12
        xor     eax, eax
        pop     r13
        ret
.L29:
        mov     rdx, rax
        shr     rdx
        and     eax, 1
        or      rdx, rax
        vcvtsi2ss       xmm1, xmm0, rdx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L30
.L64:
        mov     esi, 10
        call    std::ostream::put(char)
        jmp     .L65
.L57:
        mov     rcx, rax
        shr     rcx
        and     eax, 1
        or      rcx, rax
        vcvtsi2ss       xmm1, xmm0, rcx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L58
.L53:
        mov     rdx, rax
        shr     rdx
        and     eax, 1
        or      rdx, rax
        vcvtsi2ss       xmm1, xmm0, rdx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L54
.L49:
        mov     rcx, rax
        shr     rcx
        and     eax, 1
        or      rcx, rax
        vcvtsi2ss       xmm1, xmm0, rcx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L50
.L45:
        mov     rdx, rax
        shr     rdx
        and     eax, 1
        or      rdx, rax
        vcvtsi2ss       xmm1, xmm0, rdx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L46
.L41:
        mov     rcx, rax
        shr     rcx
        and     eax, 1
        or      rcx, rax
        vcvtsi2ss       xmm1, xmm0, rcx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L42
.L37:
        mov     rdx, rax
        shr     rdx
        and     eax, 1
        or      rdx, rax
        vcvtsi2ss       xmm1, xmm0, rdx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L38
.L33:
        mov     rcx, rax
        shr     rcx
        and     eax, 1
        or      rcx, rax
        vcvtsi2ss       xmm1, xmm0, rcx
        vaddss  xmm1, xmm1, xmm1
        jmp     .L34
.L82:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rcx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L32
.L86:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rcx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L48
.L85:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rdx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L44
.L84:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rcx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L40
.L83:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rdx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L36
.L89:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rdx, QWORD PTR [rsp+5008]
        jmp     .L60
.L88:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rcx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L56
.L87:
        lea     rdi, [rsp+16]
        call    std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()
        mov     rdx, QWORD PTR [rsp+5008]
        vmovss  xmm2, DWORD PTR .LC15[rip]
        vmovss  xmm3, DWORD PTR .LC14[rip]
        vxorps  xmm0, xmm0, xmm0
        jmp     .L52
        mov     r12, rax
        jmp     .L66
        mov     r12, rax
        vzeroupper
        jmp     .L67
main.cold:
_GLOBAL__sub_I_step(float*, float const*, int):
        sub     rsp, 8
        mov     edi, OFFSET FLAT:std::__ioinit
        call    std::ios_base::Init::Init() [complete object constructor]
        mov     edx, OFFSET FLAT:__dso_handle
        mov     esi, OFFSET FLAT:std::__ioinit
        mov     edi, OFFSET FLAT:std::ios_base::Init::~Init() [complete object destructor]
        add     rsp, 8
        jmp     __cxa_atexit
.LC0:
        .long   2139095040
.LC9:
        .quad   -2147483648
.LC10:
        .quad   2147483647
.LC11:
        .quad   1
.LC12:
        .long   1065353215
.LC14:
        .long   796917760
.LC15:
        .long   1065353216
.LC16:
        .long   0
        .long   1104006501
.LC21:
        .long   0
        .long   1078657024