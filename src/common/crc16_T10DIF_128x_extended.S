#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
# Copyright (c) 2012, Intel Corporation
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the
#   distribution.
#
# * Neither the name of the Intel Corporation nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
#
# THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL CORPORATION OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#       Function API:
#       UINT16 crc16_T10DIF_128x(
#               UINT16 init_crc,                //initial CRC value, 16 bits
#               const unsigned char *buf,       //buffer pointer to calculate CRC on
#               UINT64 len                      //buffer length in bytes (64-bit data)
#               );
#
#       This code works only on multiple of 128 Byte length buffers as input.
#       If, for example, len is set to 257, the code will compute CRC of a buffer with length 257 - (257 mod 128) = 256 Bytes.
#       Code works only on 64-bit platforms.
#
#       Authors:
#               Erdinc Ozturk
#               Vinodh Gopal
#               James Guilford
#               Wajdi Feghali
#
#       Reference paper titled "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
#       URL: http://download.intel.com/design/intarch/papers/323102.pdf
#
#
#       sample yasm command line:
#               yasm -f x64 -f elf64 -X gnu -g dwarf2 crc16_T10DIF_128x.asm
#

	.intel_syntax
# [bits 64]



	.text

        ## see e.g. http://en.wikipedia.org/wiki/X86_calling_conventions

# for windows platforms, add "WIN_ABI" to preprocessor definitions.
#ifdef WIN_ABI
         #define        arg1 rcx
         #define        arg2 rdx
         #define        arg3 r8

         %xdefine        arg1_low32 ecx
#else
        #define        arg1 rdi         # initial value of the CRC register
        #define        arg2 rsi         # arg2 is the pointer to the beginning of the data buffer
        #define        arg3 %rdx         # length of the buffer

        #define        arg1_low32 edi
#endif


	.align   16

## for OSX
	.global  _crc16_T10DIF_128x_extended


## for Linux
	.global  crc16_T10DIF_128x_extended

crc16_T10DIF_128x_extended:
_crc16_T10DIF_128x_extended:

        ## round to multiples of 8
        and     %rdx /* arg3 */, ~7

        ## jump if no data at all
        je      _zero_bytes

        ## keep track of remainder w.r.t 128
        mov     %rax, arg3
        and     %rax, 127

        # clean out low 7 bits of the length to get length - (length mod 128)
        and     %rdx /* arg3 */, ~127

        # adjust the 16-bit initial_crc value, scale it to 32 bits
        shl     %edi /* arg1_low32 */, 16

        # adjust stack pointer
        sub     %rsp,16*10+8


#ifdef WIN_ABI
        # push the xmm registers into the stack to maintain
        movdqa  [rsp+16*2],%xmm6
        movdqa  [rsp+16*3],%xmm7
        movdqa  [rsp+16*4],%xmm8
        movdqa  [rsp+16*5],%xmm9
        movdqa  [rsp+16*6],%xmm10
        movdqa  [rsp+16*7],%xmm11
        movdqa  [rsp+16*8],%xmm12
        movdqa  [rsp+16*9],%xmm13
#endif



        # load the initial crc value
        movd    %xmm10, %edi/* arg1_low32 */            # xmm10 := arg1_low32

        pslldq  %xmm10, 12                    # xmm10 <<= 96 (12 bytes)

        ## ----------------------------------------------------------------------
        ## should we look at the first < 128 bytes ?
        ## ----------------------------------------------------------------------

        movdqa  %xmm11, [SHUF_MASK[%rip]]   # xmm11 := SHUF_MASK  # WRT RIP

        push %rbx
        lea %rbx,[_jump_table[%rip]] /* [_jump_table wrt rip] */
        add %rax, %rbx
        pop %rbx
        jmp [%rax]

#        mov rax, qword [_jump_table]


#        jmp qword [%rax]
#                jmp qword [_jump_table + rax]

#        or rax, rax



        ## jump if buffer length is a multiple of 128
#        jz _standard_128byte_folding

        ## we must pad the data with zeros as follows: valid data starts
        ## at buf[0], so we must assume zeros at buf[-1] buf[-2] etc.
        ##
        ## if the remaining length is 16: clear xmm0..xmm6, put buf[0]..buf[7] into xmm7[0:63]
        ##
        ## for all operations, the shuffle mask is applied afterwards (as in normal,
        ## 128 byte block loading)

        #                        .... jump into table, indexed by (rax >> 3)




        ## we should never come here
        int 3


_remaining_length_8:
        ## if the remaining length is 8:  clear xmm0..xmm6, put buf[0]..buf[7] into xmm7[127:64] (apply the shuffle mask later on)

        ## load memory into bits 127:64 of xmm7 (see e.g. http://stackoverflow.com/a/1890956/288875 , it looks
        ## like there is no single instruction to do that)

        movq %xmm7, [%rsi] /* [arg2] */       # will clear the uppper 64 bits according to the Intel manual
        pslldq %xmm7, 8          # shift logical left by 8 bytes

        ## reorder the bytes
        pshufb  %xmm7, %xmm11

        # XOR the initial_crc value (after shuffling)
        #pxor    xmm0, xmm10                  # xmm0 ^= xmm10

        movd    %xmm10, %edi /* arg1_low32 */      # xmm10 := arg1_low32
        pslldq  %xmm10, 4                    # 4 not 12 here
        pxor     %xmm7, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        ## TODO: can we jump even further ?
        jl _compute_crc_16bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 8               #    buf += 8;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3
        pxor %xmm4, %xmm4
        pxor %xmm5, %xmm5
        pxor %xmm6, %xmm6

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2

        ## ----------------------------------------------------------------------

_remaining_length_16:

        movdqu %xmm7, [%rsi] /* [arg2] */       # load 16 bytes

        ## reorder the bytes
        pshufb  %xmm7, %xmm11

        # XOR the initial_crc value (after shuffling)
        #pxor    xmm0, xmm10                  # xmm0 ^= xmm10

        movd    %xmm10, %edi   /* arg1_low32 */ # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm7, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        ## TODO: can we jump even further ?
        jl _compute_crc_16bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 16               #    buf += 16;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3
        pxor %xmm4, %xmm4
        pxor %xmm5, %xmm5
        pxor %xmm6, %xmm6

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2

        ## ----------------------------------------------------------------------

_remaining_length_24:

        movq %xmm6, [%rsi] /* [arg2] */       # load 8 bytes
        pslldq %xmm6, 8          # shift logical left by 8 bytes
        movdqu %xmm7, [%rsi + 8] /* [arg2 + 8] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm6, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_32bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 24               #    buf += 24;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3
        pxor %xmm4, %xmm4
        pxor %xmm5, %xmm5

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2


        ## ----------------------------------------------------------------------

_remaining_length_32:

        movdqu %xmm6, [%rsi] /* [arg2]  */    # load 16 bytes
        movdqu %xmm7, [%rsi + 16]/* [arg2 + 16] */

        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm6, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_32bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 32               #    buf += 24;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3
        pxor %xmm4, %xmm4
        pxor %xmm5, %xmm5

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2

        ## ----------------------------------------------------------------------

_remaining_length_40:

        movq %xmm5, [%rsi] /* [arg2] */       # load 8 bytes
        pslldq %xmm5, 8          # shift logical left by 8 bytes
        movdqu %xmm6, [%rsi + 8] /* [arg2 + 8] */
        movdqu %xmm7, [%rsi + 24] /* [arg2 + 24] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm5, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_48bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 40               #    buf += 24;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3
        pxor %xmm4, %xmm4

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2
        ## ----------------------------------------------------------------------

_remaining_length_48:

        movdqu %xmm5, [%rsi] /* [arg2] */     # load 16 bytes
        movdqu %xmm6, [%rsi + 16] /* [arg2 + 16] */
        movdqu %xmm7, [%rsi + 32] /* [arg2 + 32] */

        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm5, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_48bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 48               #    buf += 24;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3
        pxor %xmm4, %xmm4

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2

        ## ----------------------------------------------------------------------

_remaining_length_56:

        movq %xmm4, [%rsi] /* [arg2] */       # load 8 bytes
        pslldq %xmm4, 8          # shift logical left by 8 bytes
        movdqu %xmm5, [%rsi + 8] /* [arg2 + 8] */
        movdqu %xmm6, [%rsi + 24] /* [arg2 + 24] */
        movdqu %xmm7, [%rsi + 40] /* [arg2 + 40] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm4, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_64bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 56               #    buf += 56;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    # xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2
        ## ----------------------------------------------------------------------

_remaining_length_64:

        movdqu %xmm4, [%rsi] /* [arg2] */     # load 16 bytes
        movdqu %xmm5, [%rsi + 16] /* [arg2 + 16] */ # load 16 bytes
        movdqu %xmm6, [%rsi + 32] /* [arg2 + 32] */
        movdqu %xmm7, [%rsi + 48] /* [arg2 + 48] */

        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm4, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_64bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 64               #    buf += 64;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2
        pxor %xmm3, %xmm3

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2


        ## ----------------------------------------------------------------------

_remaining_length_72:

        movq %xmm3, [%rsi] /* [arg2] */      # load 8 bytes
        pslldq %xmm3, 8          # shift logical left by 8 bytes
        movdqu %xmm4, [%rsi + 8] /* [arg2 + 8] */
        movdqu %xmm5, [%rsi + 24] /* [arg2 + 24] */
        movdqu %xmm6, [%rsi + 40] /* [arg2 + 40] */
        movdqu %xmm7, [%rsi + 56] /* [arg2 + 56] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm3, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_80bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 72               #    buf += 72;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2
        ## ----------------------------------------------------------------------

_remaining_length_80:

        movdqu %xmm3, [%rsi] /* [arg2] */     # load 16 bytes
        movdqu %xmm4, [%rsi + 16] /* [arg2 + 16] */ # load 16 bytes
        movdqu %xmm5, [%rsi + 32] /* [arg2 + 32] */
        movdqu %xmm6, [%rsi + 48] /* [arg2 + 48] */
        movdqu %xmm7, [%rsi + 64] /* [arg2 + 64] */

        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm3, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_80bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 80               #    buf += 80;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1
        pxor %xmm2, %xmm2

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2

        ## ----------------------------------------------------------------------

_remaining_length_88:

        movq %xmm2, [%rsi] /* [arg2] */       # load 8 bytes
        pslldq %xmm2, 8          # shift logical left by 8 bytes
        movdqu %xmm3, [%rsi + 8]	 /* [arg2 + 8]  */
        movdqu %xmm4, [%rsi + 24] /* [arg2 + 24] */
        movdqu %xmm5, [%rsi + 40] /* [arg2 + 40] */
        movdqu %xmm6, [%rsi + 56] /* [arg2 + 56] */
        movdqu %xmm7, [%rsi + 72] /* [arg2 + 72] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11
        pshufb  %xmm2, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm2, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_96bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 88               #    buf += 88;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2
        ## ----------------------------------------------------------------------

_remaining_length_96:

        movdqu %xmm2, [%rsi]      /* [arg2]      */# load 16 bytes
        movdqu %xmm3, [%rsi + 16] /* [arg2 + 16] */# load 16 bytes
        movdqu %xmm4, [%rsi + 32] /* [arg2 + 32] */
        movdqu %xmm5, [%rsi + 48] /* [arg2 + 48] */
        movdqu %xmm6, [%rsi + 64] /* [arg2 + 64] */
        movdqu %xmm7, [%rsi + 80] /* [arg2 + 80] */

        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11
        pshufb  %xmm2, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm2, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_96bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 96               #    buf += 96;

        pxor %xmm0, %xmm0
        pxor %xmm1, %xmm1

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2

        ## ----------------------------------------------------------------------

_remaining_length_104:

        movq %xmm1, [%rsi] /* [arg2] */       # load 8 bytes
        pslldq %xmm1, 8          # shift logical left by 8 bytes
        movdqu %xmm2, [%rsi + 8]   /* [arg2 + 8]  */
        movdqu %xmm3, [%rsi + 24]  /* [arg2 + 24] */
        movdqu %xmm4, [%rsi + 40]  /* [arg2 + 40] */
        movdqu %xmm5, [%rsi + 56]  /* [arg2 + 56] */
        movdqu %xmm6, [%rsi + 72]  /* [arg2 + 72] */
        movdqu %xmm7, [%rsi + 88]  /* [arg2 + 88] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11
        pshufb  %xmm2, %xmm11
        pshufb  %xmm1, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm1, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_112bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 104               #    buf += 104;

        pxor %xmm0, %xmm0

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2
        ## ----------------------------------------------------------------------

_remaining_length_112:

        movdqu %xmm1, [%rsi]      /* [arg2]      */# load 16 bytes
        movdqu %xmm2, [%rsi + 16] /* [arg2 + 16] */# load 16 bytes
        movdqu %xmm3, [%rsi + 32] /* [arg2 + 32] */
        movdqu %xmm4, [%rsi + 48] /* [arg2 + 48] */
        movdqu %xmm5, [%rsi + 64] /* [arg2 + 64] */
        movdqu %xmm6, [%rsi + 80] /* [arg2 + 80] */
        movdqu %xmm7, [%rsi + 96] /* [arg2 + 96] */

        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11
        pshufb  %xmm2, %xmm11
        pshufb  %xmm1, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */          # xmm10 := arg1_low32
        pslldq  %xmm10, 12
        pxor     %xmm1, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _compute_crc_112bytes

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

                ## advance the buffer pointer
        add     %rsi /* arg2 */, 112               #    buf += 112;

        pxor %xmm0, %xmm0

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */   #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2


        ## ----------------------------------------------------------------------
_remaining_length_120:

        movq %xmm0, [%rsi] /* [arg2] */       # load 8 bytes
        pslldq %xmm0, 8          # shift logical left by 8 bytes
        movdqu %xmm1, [%rsi + 8]   /* [arg2 + 8]   */
        movdqu %xmm2, [%rsi + 24]  /* [arg2 + 24]  */
        movdqu %xmm3, [%rsi + 40]  /* [arg2 + 40]  */
        movdqu %xmm4, [%rsi + 56]  /* [arg2 + 56]  */
        movdqu %xmm5, [%rsi + 72]  /* [arg2 + 72]  */
        movdqu %xmm6, [%rsi + 88]  /* [arg2 + 88]  */
        movdqu %xmm7, [%rsi + 104] /* [arg2 + 104] */


        ## reorder the bytes
        pshufb  %xmm7, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm3, %xmm11
        pshufb  %xmm2, %xmm11
        pshufb  %xmm1, %xmm11
        pshufb  %xmm0, %xmm11

        ## add initial register CRC value
        movd    %xmm10, %edi /* arg1_low32 */            # xmm10 := arg1_low32
        pslldq  %xmm10, 4        # 4 not 12 here
        pxor     %xmm0, %xmm10

        # process the subsequent data (after checking that we have
        # at least 128 bytes left (for simplicity, we just duplicate
        # the code here)

        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of bytes to be processed minus 128

        jl _128B_left

        ## we have at least another 128 bytes to process
        ## jump just behind updating the buffer pointer

        ## advance the buffer pointer
        add     %rsi /* arg2 */, 120               #    buf += 120;

        ## load constants
        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        jmp     _fold_128B_loop2
        ## ----------------------------------------------------------------------

_standard_128byte_folding:
        movdqa  %xmm11,[SHUF_MASK[%rip]] /* [SHUF_MASK wrt rip] */   # xmm11 := SHUF_MASK ??
        # receive the initial 128B data
        movdqu  %xmm0, [%rsi+16*0]  /* [arg2+16*0] */         # xmm0 := arg2[0 * 16]
        movdqu  %xmm1, [%rsi+16*1]  /* [arg2+16*1] */         # xmm1 := arg2[1 * 16]
        movdqu  %xmm2, [%rsi+16*2]  /* [arg2+16*2] */         # xmm2 := arg2[2 * 16]
        movdqu  %xmm3, [%rsi+16*3]  /* [arg2+16*3] */         # xmm3 := arg2[3 * 16]
        movdqu  %xmm4, [%rsi+16*4]  /* [arg2+16*4] */         # xmm4 := arg2[4 * 16]
        movdqu  %xmm5, [%rsi+16*5]  /* [arg2+16*5] */         # xmm5 := arg2[5 * 16]
        movdqu  %xmm6, [%rsi+16*6]  /* [arg2+16*6] */         # xmm6 := arg2[6 * 16]
        movdqu  %xmm7, [%rsi+16*7]  /* [arg2+16*7] */         # xmm7 := arg2[7 * 16]

        ## reorder the bytes
        pshufb  %xmm0, %xmm11                # packed shuffle bytes
                                             # xmm0[i] = xmm0[SHUF_MASK[i]] for i = 0..15 ?!
                                             # with our (CMS DAQ) mask, we get:
                                             #
                                             # xmm0[127:0] = arg2[7] .. arg2[0] arg2[15] .. arg2[8]

        # XOR the initial_crc value (after shuffling)
        pxor    %xmm0, %xmm10                  # xmm0 ^= xmm10

        pshufb  %xmm1, %xmm11                       # xmm1[i] = xmm1[SHUF_MASK[i]] for i = 0..15 ?!
        pshufb  %xmm2, %xmm11                       # xmm2[i] = xmm2[SHUF_MASK[i]] for i = 0..15 ?!
        pshufb  %xmm3, %xmm11
        pshufb  %xmm4, %xmm11
        pshufb  %xmm5, %xmm11
        pshufb  %xmm6, %xmm11
        pshufb  %xmm7, %xmm11

        movdqa  %xmm10,[rk3[%rip]] /* [rk3 wrt rip] */    #xmm10 has rk3 and rk4

        #;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



        sub     %rdx /* arg3 */, 256

        ## arg3 now holds the number of bytes still to be processed minus 128
        ## (we just have processed 128 bytes above)

        ## jump if we only have 128 bytes left
        ## (note that this INCLUDES the bytes we just loaded)

        ## (i.e. arg3 - 256 will be -128)
        ## do NOT jump if we have >= 256 bytes left (effectively > 128 bytes)
        jl      _128B_left


        # fold 128B at a time. This section of the code folds 8 xmm registers in parallel
_fold_128B_loop:

        # update the buffer pointer
        add     %rsi /* arg2 */, 128               #    buf += 128;

_fold_128B_loop2:


        ## ----------------------------------------
        ## 32 bytes (2 XMM registers): buf[16*2 .. 16*3 + 15]
        ## ----------------------------------------

        ## load new data (32 bytes) into xmm9 and xmm12
        movdqu  %xmm9,  [%rsi+16*0]  /* [arg2+16*0] */             # xmm9 := buf[0 * 16]
        movdqu  %xmm12, [%rsi+16*1]  /* [arg2+16*1] */             # xmm12 := buf[1 * 16]

        ## apply same shuffling mask
        pshufb  %xmm9, %xmm11
        pshufb  %xmm12, %xmm11

        ## copy xmm0 and xmm1 (previous data) to xmm8 and xmm13
        movdqa  %xmm8, %xmm0                        # xmm8 := xmm0
        movdqa  %xmm13, %xmm1                       # xmm13 := xmm1

        ## rk3 and rk4 are in xmm10 (xmm10[63:0] == rk3 ?)

        ## lower 64 bits of xmm0
        ## with our shuffle mask: arg2[15] .. arg2[8]
        ##
        ## multiply previous data by x^(15*64+32) mod polynomial
        ##
        ## lower 8 bytes of xmm0
        pclmulqdq       %xmm0, %xmm10, 0x00         # xmm0[127:0]   := xmm0[63:0] * xmm10[63:0]     (rk3), x^(15*64+32) mod poly

        ## upper 8 bytes of xmm10 (originally xmm0): arg2[7] .. arg2[0]
        pclmulqdq       %xmm8, %xmm10 , 0x11        # xmm8[127:0]   := xmm8[127:64] * xmm10[127:64] (rk4), x^(16*64+32) mod poly

        ## lower 8 bytes of xmm1: arg2[16 + 15] .. arg2[16 + 8]
        pclmulqdq       %xmm1, %xmm10, 0x00         # xmm1[127:0]   := xmm1[63:0] * xmm10[63:0]     (rk3), x^(15*64+32) mod poly

        ## upper 64 bits of xmm13 (originally xmm1): arg2[16 + 7] .. arg2[16 + 0]
        pclmulqdq       %xmm13, %xmm10 , 0x11       # xmm13[127:0]  := xmm13[63:0] * xmm10[127:64]  (rk4), x^(16*64+32) mod poly

        ## here, xmm0 contains xmm0[63:0] * rk3
        pxor    %xmm0, %xmm9                        # xmm0 ^= xmm9  (xmm9 contains new data)
        xorps   %xmm0, %xmm8                        # xmm0 ^= xmm8  (xmm8 contains xmm0[126:64] multiplied by a remainder)
        pxor    %xmm1, %xmm12                       # xmm1 ^= xmm12
        xorps   %xmm1, %xmm13                       # xmm1 ^= xmm13

        ## ----------------------------------------
        ## 32 bytes (2 XMM registers): buf[16*2 .. 16*3 + 15]
        ##
        ## we use the same remainders (rk3 and rk4) here as in the previous step
        ## ----------------------------------------
        movdqu  %xmm9,  [%rsi+16*2]  /* [arg2+16*2] */
        movdqu  %xmm12, [%rsi+16*3]  /* [arg2+16*3] */

        ## apply byte shuffling
        pshufb  %xmm9, %xmm11
        pshufb  %xmm12, %xmm11

        ## copy xmm2 and xmm3 (previous data) to xmm8 and xmm13
        movdqa  %xmm8, %xmm2
        movdqa  %xmm13, %xmm3

        pclmulqdq       %xmm2,  %xmm10, 0x00
        pclmulqdq       %xmm8,  %xmm10 , 0x11
        pclmulqdq       %xmm3,  %xmm10, 0x00
        pclmulqdq       %xmm13, %xmm10 , 0x11
        pxor    %xmm2, %xmm9
        xorps   %xmm2, %xmm8
        pxor    %xmm3, %xmm12
        xorps   %xmm3, %xmm13

        ## ----------------------------------------
        ## 32 bytes (2 XMM registers): buf[16*4 .. 16*5 + 15]
        ##
        ## we use the same remainders (rk3 and rk4) here as in the previous step
        ## ----------------------------------------

        movdqu  %xmm9,  [%rsi+16*4]  /* [arg2+16*4] */
        movdqu  %xmm12, [%rsi+16*5]  /* [arg2+16*5] */
        pshufb  %xmm9, %xmm11
        pshufb  %xmm12, %xmm11
        movdqa  %xmm8, %xmm4
        movdqa  %xmm13, %xmm5
        pclmulqdq       %xmm4,  %xmm10, 0x00
        pclmulqdq       %xmm8,  %xmm10 , 0x11
        pclmulqdq       %xmm5,  %xmm10, 0x00
        pclmulqdq       %xmm13, %xmm10 , 0x11
        pxor    %xmm4, %xmm9
        xorps   %xmm4, %xmm8
        pxor    %xmm5, %xmm12
        xorps   %xmm5, %xmm13

        ## ----------------------------------------
        ## 32 bytes (2 XMM registers): buf[16*6 .. 16*7 + 15]
        ##
        ## we use the same remainders (rk3 and rk4) here as in the previous step
        ## ----------------------------------------

        movdqu  %xmm9,  [%rsi+16*6]   /* [arg2+16*6] */
        movdqu  %xmm12, [%rsi+16*7]   /* [arg2+16*7] */
        pshufb  %xmm9,  %xmm11
        pshufb  %xmm12, %xmm11
        movdqa  %xmm8,  %xmm6
        movdqa  %xmm13, %xmm7
        pclmulqdq       %xmm6,  %xmm10, 0x00
        pclmulqdq       %xmm8,  %xmm10, 0x11
        pclmulqdq       %xmm7,  %xmm10, 0x00
        pclmulqdq       %xmm13, %xmm10, 0x11
        pxor    %xmm6, %xmm9
        xorps   %xmm6, %xmm8
        pxor    %xmm7, %xmm12
        xorps   %xmm7, %xmm13

        # check if there is another 128B in the buffer to be able to fold
        sub     %rdx /* arg3 */, 128
        ## arg3 now holds the number of remaining bytes to be processes minus 128

        jge     _fold_128B_loop

        ## arg3 should be -128 here

        #;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ## ----------------------------------------------------------------------
        ##  calculation for the last/remaining 128 bytes (after folding 128 bytes in parallel)
        ## ----------------------------------------------------------------------
_128B_left:
        ## increase the buffer pointer
        ## (why do we do that here, it is not used later on ?!)
        add     %rsi /* arg2 */, 128

        # the 128B of folded data is in 8 of the xmm registers: xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
        # fold the 8 xmm registers to 1 xmm register with different constants
        ## (that's 16 bytes per xmm register, i.e. 128 bit, so the full width of the xmm register)

        ## fold xmm0 to xmm7
        movdqa  %xmm10,[rk9[%rip]] /* [rk9 wrt rip] */    # xmm10 := rk9 + rk10 << 64 (?)
        movdqa  %xmm8, %xmm0                        # xmm8 := xmm0
        pclmulqdq       %xmm0, %xmm10, 0x11         # xmm0[127:0] := xmm0[126:64] * xmm10[126:64] (rk10) (?)
        pclmulqdq       %xmm8, %xmm10, 0x00         # xmm8[127:0] := xmm8[63:0]   * xmm10[63:0]   (rk9)  (?)
        pxor    %xmm7, %xmm8                        # xmm7 ^= xmm8 (add/subtract polynomials)
        xorps   %xmm7, %xmm0                        # xmm7 ^= xmm0 (add/subtract polynomials)

_compute_crc_112bytes:
        ## fold xmm1 to xmm7
        movdqa  %xmm10,[rk11[%rip]] /* [rk11 wrt rip] */  # xmm10 := rk11 + rk12 << 64 (?)
        movdqa  %xmm8, %xmm1
        pclmulqdq       %xmm1, %xmm10, 0x11
        pclmulqdq       %xmm8, %xmm10, 0x00
        pxor    %xmm7, %xmm8
        xorps   %xmm7, %xmm1

_compute_crc_96bytes:
        ## fold xmm2 to xmm7
        movdqa  %xmm10,[rk13[%rip]] /* [rk13 wrt rip] */
        movdqa  %xmm8, %xmm2
        pclmulqdq       %xmm2, %xmm10, 0x11
        pclmulqdq       %xmm8, %xmm10, 0x00
        pxor    %xmm7, %xmm8
        pxor    %xmm7, %xmm2

_compute_crc_80bytes:
        ## fold xmm3 to xmm7
        movdqa  %xmm10,[rk15[%rip]] /* [rk15 wrt rip] */
        movdqa  %xmm8, %xmm3
        pclmulqdq       %xmm3, %xmm10, 0x11
        pclmulqdq       %xmm8, %xmm10, 0x00
        pxor    %xmm7, %xmm8
        xorps   %xmm7, %xmm3

_compute_crc_64bytes:
        ## fold xmm4 to xmm7
        movdqa  %xmm10,[rk17[%rip]] /* [rk17 wrt rip] */
        movdqa  %xmm8, %xmm4
        pclmulqdq       %xmm4, %xmm10, 0x11
        pclmulqdq       %xmm8, %xmm10, 0x00
        pxor    %xmm7, %xmm8
        pxor    %xmm7, %xmm4

_compute_crc_48bytes:

        ## fold xmm5 to xmm7
        movdqa  %xmm10,[rk19[%rip]] /* [rk19 wrt rip] */
        movdqa  %xmm8, %xmm5
        pclmulqdq       %xmm5, %xmm10, 0x11
        pclmulqdq       %xmm8, %xmm10, 0x00
        pxor    %xmm7, %xmm8
        xorps   %xmm7, %xmm5

_compute_crc_32bytes:
        ## fold xmm6 to xmm7
        movdqa  %xmm10,[rk1[%rip]] /* [rk1 wrt rip] */
        movdqa  %xmm8, %xmm6
        pclmulqdq       %xmm6, %xmm10, 0x11
        pclmulqdq       %xmm8, %xmm10, 0x00
        pxor    %xmm7, %xmm8
        pxor    %xmm7, %xmm6


        add     %rdx /* arg3 */, 128

        ## ----------------------------------------
        ## compute crc of a 128-bit value (16 bytes) in xmm7
        ## ----------------------------------------
_compute_crc_16bytes:
        ## (current value of CRC is in xmm7[127:0] ?)
        movdqa  %xmm10,[rk5[%rip]] /* [rk5 wrt rip] */    # rk5 and rk6 in xmm10
        movdqa  %xmm0, %xmm7              # xmm0 := xmm7

        #64b fold
        ## multiplying by k5 reduces 128 bit to 96 bit (according to pdf)
        ## fold upper 32 bits of xmm7  -> a 96 bit result
        ## rk5 has bits 63..48 valid (16 bits)
        ## xmm7 should have all bits valid: 127..0 (but only 127..64 used here)
        ## -> result should have bits 126..48 valid (79 bits ~ 80 bits)
        ## for a 32 bit CRC: rk5 would have bits 63..32 valid -> result bits 126..32 (96 bits)
        pclmulqdq       %xmm7, %xmm10, 0x1 # xmm7[127:0] := xmm7[127:64] * xmm10[63:0] (rk5), rk5 ~ x^96

        ## add result of multiplication to lower 8 bytes of xmm0 and store in xmm7
        pslldq  %xmm0, 8                 # xmm0 <<= 64 (8 bytes)
        pxor    %xmm7, %xmm0              # xmm7 ^= xmm0

        ## xmm7 stil has bits 127..32 valid for a 32 bit CRC (127..48 for a 16 bit CRC)

        #32b fold
        movdqa  %xmm0, %xmm7              # xmm0 := xmm7

        pand    %xmm0,[mask[%rip]] /* [mask wrt rip] */  # xmm0 &= mask (96 lower bits set to 1)
                                        # so now only bits 95..0 are valid in xmm0
                                        # but for a 32 bit CRC, only bits 127..32 were valid before
                                        # so now only bits 95..32 are valid in xmm0 (64 bits)

        psrldq  %xmm7, 12                # xmm7 >>= 96 (12 bytes), so at most bits 31..0 are valid in xmm7
                                        # (which were the top most 32 bits of the previous computation step)

        ## we have 96 bits left:
        ##   - the top  32 bits  are in bits 31..0  in xmm7
        ##   - the lower 64 bits are in bits 95..32 in xmm0

        ## multiplication with rk6 reduces 96 bits to 64 bits
        ## rk6 has only bits 63..48 set -> xmm[127:112], would be 63..32 for a 32 bit CRC
        ## xmm7 only has bits 31..0 set
        ## so the result (in xmm7) will have bits 94..48 set (31 bits) ? for a 32 bit CRC: 94..32 (63 bits)
        pclmulqdq       %xmm7, %xmm10, 0x10 # xmm7[127:0] := xmm7[63:0] * xmm10[127:64] (rk6), rk6 ~ x^64

        ## xmm0 only has bits 95..0 valid (actually only 95..32, for a 32 bit CRC)
        ## xmm7 only has bits 95..48 valid (95..32 for a 32 bit CRC)
        pxor    %xmm7, %xmm0              # xmm7 ^= xmm0

        ## current CRC value in xmm7 ? in which bits, 95..32 for a 32 bit CRC ?

        ## if we have an additional 64 bits (8 bytes) to add, do it here. Or do it above, padding
        ## to 128 bytes ?

        ## ----------------------------------------
        #barrett reduction of 64 bit to 32 bit (?)
        ## ----------------------------------------
_barrett:
        movdqa  %xmm10,[rk7[%rip]] /* [rk7 wrt rip] */ # rk7 and rk8 in xmm10
        movdqa  %xmm0, %xmm7                # xmm0 := xmm7
        pclmulqdq       %xmm7, %xmm10, 0x01 # xmm7[127:0] := xmm7[127:64] * xmm10[63:0] (rk7)
        pslldq  %xmm7, 4                   # xmm7 <<= 32
        pclmulqdq       %xmm7, %xmm10, 0x11 # xmm7[127:0] := xmm7[127:64] * xmm10[127:64] (rk8)

        pslldq  %xmm7, 4                   # xmm7 <<= 32
        pxor    %xmm7, %xmm0                # xmm7 ^= xmm0
        pextrd  %eax, %xmm7,1               # eax := xmm7[63..32]

_cleanup:
        # scale the result back to 16 bits
        shr     %eax, 16
#ifdef WIN_ABI
        movdqa  %xmm6, [rsp+16*2]
        movdqa  %xmm7, [rsp+16*3]
        movdqa  %xmm8, [rsp+16*4]
        movdqa  %xmm9, [rsp+16*5]
        movdqa  %xmm10, [rsp+16*6]
        movdqa  %xmm11, [rsp+16*7]
        movdqa  %xmm12, [rsp+16*8]
        movdqa  %xmm13, [rsp+16*9]
#endif
        add     %rsp,16*10+8
        ret


_check_less_than_128_bytes:
        or %rax,%rax
        je _zero_bytes




_zero_bytes:
        mov     %eax, %edi /* arg1_low32 */
        ret



#----------------------------------------------------------------------
	.data
#----------------------------------------------------------------------

# precomputed constants
# computed by AH from polynomial 0x8005
#

.align 16
rk1:
.quad 0x8663000000000000
rk2:
.quad 0x8617000000000000
rk3:
.quad 0x8665000000000000
rk4:
.quad 0x8077000000000000
rk5:
.quad 0x8663000000000000
rk6:
.quad 0x807b000000000000
rk7:
.quad 0x00000001fffbffe7
rk8:
.quad 0x0000000180050000
rk9:
.quad 0x6a7a000000000000
rk10:
.quad 0x5ccb000000000000
rk11:
.quad 0x006b000000000000
rk12:
.quad 0xedb3000000000000
rk13:
.quad 0xf997000000000000
rk14:
.quad 0x8c47000000000000
rk15:
.quad 0xbffa000000000000
rk16:
.quad 0x861b000000000000
rk17:
.quad 0xeac3000000000000
rk18:
.quad 0xed6b000000000000
rk19:
.quad 0xf557000000000000
rk20:
.quad 0x806f000000000000



	.align 16
mask:
	.octa 0x00000000FFFFFFFFFFFFFFFFFFFFFFFF

SHUF_MASK:

# little endian ?
#DDQ 0x000102030405060708090A0B0C0D0E0F

# big endian ?
	.octa 0x07060504030201000F0E0D0C0B0A0908


_jump_table:

.quad      _standard_128byte_folding # 0 bytes modulo 128
.quad      _remaining_length_8       # 8 bytes modulo 128
.quad      _remaining_length_16
.quad      _remaining_length_24
.quad      _remaining_length_32
.quad      _remaining_length_40
.quad      _remaining_length_48
.quad      _remaining_length_56
.quad      _remaining_length_64
.quad      _remaining_length_72
.quad      _remaining_length_80
.quad      _remaining_length_88
.quad      _remaining_length_96
.quad      _remaining_length_104
.quad      _remaining_length_112
.quad      _remaining_length_120
