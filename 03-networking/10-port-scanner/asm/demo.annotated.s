# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. This is the TCP pseudo-header checksum: the ones-complement sum
# (sum16), the end-around-carry fold + complement (fold_csum), and the routine
# that stitches them together over the pseudo-header and the segment
# (tcp_checksum). It is the arithmetic core of the SYN scanner.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), annotated. AT&T syntax throughout:
#
#     op   src, dst                      # movl %esp,%ebp  =>  ebp = esp
#     %reg      register    $imm immediate    sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]         (%r1,%r2) memory at [r1+r2]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al. Writing
# a 32-bit name (eax) ZERO-EXTENDS into the 64-bit register, which is why the
# compiler prefers `movl` when the top 32 bits should be zero. The 16-bit (%ax)
# and 8-bit (%al) names do NOT zero the rest — they leave the upper bytes intact,
# which matters below where `rolw $8,%cx` deliberately touches only the low word.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   * Return value:                          rax  (eax for a 32-bit / int return)
#   * Caller-saved (scratch; a callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (a callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp a leaf function may use without moving rsp.
#     Every function here is a leaf (calls nobody), so none touches rsp for locals.
#   * Stack alignment: rsp must be 16-byte aligned at a `call`. Nothing here calls,
#     so the only stack traffic is the debug frame-pointer push/pop.
#
# THE ARGUMENTS OF EACH FUNCTION (so the register names below read as values)
#   sum16(const u8 *buf /*rdi*/, u32 len /*esi*/, u32 sum /*edx*/) -> u32 /*eax*/
#   fold_csum(u32 sum /*edi*/) -> u16 /*ax*/
#   tcp_checksum(const struct pseudo_header *ph /*rdi*/,
#                const u8 *segment /*rsi*/, u32 seg_len /*edx*/) -> u16 /*ax*/
#
# WHY THE CODE LOOKS LIKE THIS
# ----------------------------
# The one optimization to watch for is how clang reads a big-endian 16-bit word.
# The C says `(buf[0] << 8) | buf[1]`. The compiler recognises this as "load two
# bytes, then swap them", and emits a 16-bit load followed by `rolw $8` (rotate
# the 16-bit value by 8 = swap its two bytes). That single rotate IS the network
# byte order conversion — it is exactly what ntohs()/htons() compile to. Watching
# it here is the point: endianness handling is one instruction, not magic.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# sum16(const u8 *buf /*rdi*/, u32 len /*esi*/, u32 sum /*edx*/) -> u32 /*eax*/
#   while (len > 1) { sum += (buf[0]<<8)|buf[1]; buf += 2; len -= 2; }
#   if (len == 1)    sum += buf[0] << 8;
#   return sum;
# The ones-complement accumulation. `sum` arrives in edx and is moved into the
# return register eax up front, so the whole loop just adds into eax.
# =============================================================================
	.globl	sum16
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	sum16,@function
sum16:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only; leaf)
	movl	%edx, %eax                  # eax = sum  (seed the accumulator = return reg)
	cmpl	$2, %esi                    # compare len to 2 ...
	jb	.LBB0_2                         #   if len < 2, skip the word loop entirely
	.p2align	4                       # align the hot loop top for the fetcher
# ---- word loop: consume two bytes per iteration -----------------------------
.LBB0_1:                                # do {
	movzwl	(%rdi), %ecx                #   ecx = *(u16*)buf  (load the two bytes as-is)
	rolw	$8, %cx                     #   swap the two bytes -> big-endian value.
	                                    #     THIS is the htons/ntohs: (buf[0]<<8)|buf[1].
	movzwl	%cx, %ecx                   #   zero-extend the 16-bit word into ecx
	addl	%ecx, %eax                  #   sum += word   (carries pile into eax's high half)
	addq	$2, %rdi                    #   buf += 2  (advance the pointer by one word)
	addl	$-2, %esi                   #   len -= 2
	cmpl	$1, %esi                    #   compare len to 1 ...
	ja	.LBB0_1                         # } while (len > 1)   (unsigned "above")
.LBB0_2:                                # ---- odd trailing byte? -----------------
	testl	%esi, %esi                  # len == 0 ?
	je	.LBB0_4                         #   if len == 0, nothing left, go return
# %bb.3: exactly one byte remains
	movzbl	(%rdi), %ecx                # ecx = buf[0]  (zero-extend the single byte)
	shll	$8, %ecx                    # ecx = buf[0] << 8  (it is the HIGH half; low = 0)
	addl	%ecx, %eax                  # sum += padded byte   (RFC 1071 zero-pad)
.LBB0_4:
	popq	%rbp                        # EPILOGUE: restore caller's frame pointer
	retq                                # return sum in eax
.Lfunc_end0:
	.size	sum16, .Lfunc_end0-sum16

# =============================================================================
# fold_csum(u32 sum /*edi*/) -> u16 /*ax*/
#   while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
#   return ~sum;
# End-around carry: keep folding the high 16 bits into the low 16 until no carry
# remains, then complement. `sum >> 16` is the carry, `sum & 0xFFFF` the low word.
# =============================================================================
	.globl	fold_csum
	.p2align	4
	.type	fold_csum,@function
fold_csum:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                  # eax = sum
	cmpl	$65536, %edi                # sum >= 0x10000 ?  (are any bits above bit 15 set?)
	jb	.LBB1_2                         #   if sum < 0x10000, no carry to fold, skip loop
	.p2align	4
# ---- fold loop: add the high half back into the low half --------------------
.LBB1_1:                                # do {
	movl	%eax, %ecx                  #   ecx = sum
	shrl	$16, %ecx                   #   ecx = sum >> 16      (the carry-out bits)
	movzwl	%ax, %eax                   #   eax = sum & 0xFFFF   (the low 16 bits)
	addl	%ecx, %eax                  #   sum = low + carry    (may itself carry, hence loop)
	cmpl	$65535, %eax                #   sum > 0xFFFF ?
	ja	.LBB1_1                         # } while (sum >> 16)  (another carry appeared)
.LBB1_2:
	notl	%eax                        # eax = ~sum   (ones-complement = the wire checksum)
	                                    # kill: the upper 16 bits are discarded by the u16
	                                    #   return; only %ax is meaningful to the caller.
	popq	%rbp                        # EPILOGUE
	retq                                # return ~sum in ax
.Lfunc_end1:
	.size	fold_csum, .Lfunc_end1-fold_csum

# =============================================================================
# tcp_checksum(const struct pseudo_header *ph /*rdi*/, const u8 *segment /*rsi*/,
#              u32 seg_len /*edx*/) -> u16 /*ax*/
#   sum = sum16(ph, 12, 0);           // (b) the 12-byte pseudo-header
#   sum = sum16(segment, seg_len, sum);// (c) the TCP segment
#   return fold_csum(sum);            // (d) fold + complement
# BOTH sum16 calls and fold_csum are INLINED. Two optimizer moves to notice:
#   1. The pseudo-header length is the COMPILE-TIME constant sizeof(*ph)==12, so
#      its loop is specialised to "index rcx from 0 to 12" with NO length variable
#      and NO odd-byte tail (12 is even, so that whole branch is deleted as dead).
#   2. The frame-pointer PROLOGUE is deferred: the first loop touches no stack, so
#      clang pushes rbp only AFTER it (at .LBB2_pseudo_done) and pops it BEFORE the
#      final fold — bookkeeping placed exactly where a frame is nominally "live".
# =============================================================================
	.globl	tcp_checksum
	.p2align	4
	.type	tcp_checksum,@function
tcp_checksum:
# ---- (b) inlined sum16 over the 12-byte pseudo-header -----------------------
	xorl	%ecx, %ecx                  # rcx = 0  = byte index into the pseudo-header
	xorl	%eax, %eax                  # eax = 0  = running sum  (sum16's initial seed 0)
	.p2align	4
.LBB2_1:                                # do {   (loop over ph[0..12) as 6 words)
	movzwl	(%rdi,%rcx), %r8d           #   r8d = *(u16*)(ph + rcx)  (load two bytes)
	rolw	$8, %r8w                    #   swap them -> big-endian word (the htons again)
	movzwl	%r8w, %r8d                  #   zero-extend the 16-bit word
	addl	%r8d, %eax                  #   sum += word
	addq	$2, %rcx                    #   index += 2
	cmpl	$12, %ecx                   #   reached the fixed length 12 ?
	jne	.LBB2_1                        # } while (rcx != 12)   (no len var: 12 is constant)
# %bb.2: pseudo-header done — NOW take the frame (the loop above needed no stack)
	pushq	%rbp                        # PROLOGUE (deferred): save caller's frame pointer
	movq	%rsp, %rbp                  #   establish frame
# ---- (c) inlined sum16 over the variable-length TCP segment ------------------
	cmpl	$2, %edx                    # seg_len < 2 ?
	jb	.LBB2_4                         #   if so, skip the word loop
	.p2align	4
.LBB2_3:                                # do {
	movzwl	(%rsi), %ecx                #   ecx = *(u16*)segment
	rolw	$8, %cx                     #   byte-swap -> big-endian word
	movzwl	%cx, %ecx                   #   zero-extend
	addl	%ecx, %eax                  #   sum += word
	addq	$2, %rsi                    #   segment += 2
	addl	$-2, %edx                   #   seg_len -= 2
	cmpl	$1, %edx                    #   seg_len > 1 ?
	ja	.LBB2_3                        # } while (seg_len > 1)
.LBB2_4:                                # ---- odd trailing byte of the segment? --
	testl	%edx, %edx                  # seg_len == 0 ?
	je	.LBB2_6                         #   if 0, none left
# %bb.5: one leftover byte
	movzbl	(%rsi), %ecx                # ecx = segment[0]
	shll	$8, %ecx                    # ecx = segment[0] << 8   (high half; low = 0)
	addl	%ecx, %eax                  # sum += padded byte
.LBB2_6:                                # ---- (d) inlined fold_csum(sum) ---------
	cmpl	$65536, %eax                # sum >= 0x10000 ?  (any carry bits to fold?)
	popq	%rbp                        # EPILOGUE (early): frame is dead past this point,
	                                    #   the fold below is pure register arithmetic.
	jb	.LBB2_8                         #   if no carry, skip the fold loop
	.p2align	4
.LBB2_7:                                # do {
	movl	%eax, %ecx                  #   ecx = sum
	shrl	$16, %ecx                   #   ecx = sum >> 16   (carry-out)
	movzwl	%ax, %eax                   #   eax = sum & 0xFFFF (low word)
	addl	%ecx, %eax                  #   sum = low + carry
	cmpl	$65535, %eax                #   sum > 0xFFFF ?
	ja	.LBB2_7                        # } while (still carrying)
.LBB2_8:
	notl	%eax                        # eax = ~sum   (the final 16-bit checksum in %ax)
	                                    # kill: only %ax is returned (u16); high bits ignored
	retq                                # return the checksum in ax
.Lfunc_end2:
	.size	tcp_checksum, .Lfunc_end2-tcp_checksum

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security default)
	.addrsig                            # address-significance table (LLVM ICF metadata)
# =============================================================================
# WHAT TO TAKE AWAY
#   * Network byte order is not magic: `(buf[0]<<8)|buf[1]` compiles to a 16-bit
#     load plus `rolw $8` — one instruction, the same one htons()/ntohs() use.
#   * The ones-complement sum runs in a wider (32-bit) accumulator so carries are
#     never lost; fold_csum's end-around-carry loop reconciles them at the end.
#   * A compile-time-constant length (sizeof pseudo_header == 12) lets the compiler
#     drop the loop's length variable AND delete the odd-byte tail as dead code —
#     visible only in the asm (compare the pseudo-header loop with the segment one).
#   * The frame pointer is bookkeeping: clang slid the push/pop to bracket only the
#     region where a debugger would want a frame, around the pure-arithmetic edges.
#   * Compare with demo.O0.s (every value spilled to the stack, every branch real)
#     and demo.O2.s (the loops unrolled / laid out for the branch predictor).
# =============================================================================
