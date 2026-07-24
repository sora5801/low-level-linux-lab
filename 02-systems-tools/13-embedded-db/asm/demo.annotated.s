# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
# instruction. This is the KV store's pure-logic core: O_DIRECT alignment, the
# little-endian codecs, key comparison, the in-page binary search, and one byte
# of CRC32.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the EXACT assembly clang 20.1.8 emits for demo.c at -O1 (see demo.s for
# the untouched original), annotated. AT&T syntax throughout:
#
#     op   src, dst                       # movq %rsp,%rbp  =>  rbp = rsp
#     %reg      register    $imm immediate    sym(%rip) RIP-relative address
#     N(%reg)   memory at [reg+N]         (%r1,%r2) memory at [r1+r2]
#     N(%r1,%r2) memory at [r1 + r2 + N]  (%r1,%r2,s) memory at [r1 + r2*s]
#
# A register's narrow names are the SAME physical register: rax/eax/ax/al, and
# r8/r8d/r8w/r8b. Writing a 32-bit name ZERO-EXTENDS into the full 64-bit
# register, which is why the compiler prefers `movl`/`xorl` when the top 32 bits
# should be zero. `movzwl`/`movzbl` are explicit zero-extending loads (word/byte
# -> long); `movsbl` sign-extends a byte to a long.
#
# THE SYSTEM V AMD64 ABI (the contract every function below obeys)
# ----------------------------------------------------------------
#   * Integer/pointer arguments, in order:  rdi, rsi, rdx, rcx, r8, r9   (then stack)
#     A u16 argument arrives in the low 16 bits (e.g. si/cx) with the top bits
#     UNSPECIFIED — that is why the code zero-extends length args before use.
#   * Return value:                          rax (eax for an int)
#   * Caller-saved (scratch; a callee may clobber): rax, rcx, rdx, rsi, rdi, r8-r11
#   * Callee-saved (a callee MUST preserve):        rbx, rbp, r12-r15, rsp
#   * The RED ZONE: 128 bytes below rsp a leaf function may use without moving rsp.
#   * Stack alignment: rsp % 16 == 0 at every `call`. Nothing here calls anyone
#     (key_cmp was INLINED into slot_lower_bound), so the only stack traffic is
#     saving the frame pointer and, in slot_lower_bound, two callee-saved regs.
#
# WHY THE CODE LOOKS LIKE THIS (the four lessons in this file)
# ------------------------------------------------------------
#   1. le16/le32: a hand-rolled "p[0] | p[1]<<8 | ..." endian decode compiles to
#      a SINGLE load (movzwl / movl) because the target is itself little-endian.
#      Portable C, zero-cost on the native byte order.
#   2. align_up: the alignment mask ~(a-1) is computed as -a (two's complement).
#   3. key_cmp: the length tiebreak sign(alen-blen) is done branchlessly with
#      seta/sbb; slot_lower_bound then INLINES all of this.
#   4. slot_lower_bound / crc32_byte: branches the optimizer can predict poorly
#      become `cmov` (conditional move) — the lo/hi update of the binary search
#      is fully branchless.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# align_up(usize n /*rdi*/, usize a /*rsi*/) -> usize /*rax*/
#   return (n + (a-1)) & ~(a-1);
# Rounds an O_DIRECT buffer address / file offset / length up to a power-of-two
# block boundary. The elegant part: the mask ~(a-1) equals -a, via the identity
#   -a == ~a + 1 == ~(a-1),  so one `neg` builds the mask.
# =============================================================================
	.globl	align_up
	.p2align	4                       # 16-byte-align the entry (I-fetch friendly)
	.type	align_up,@function
align_up:
	pushq	%rbp                        # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp                  #   establish our frame (debug only here)
	leaq	(%rdi,%rsi), %rax           # rax = n + a   (LEA adds without touching flags)
	decq	%rax                        # rax = n + a - 1  (= n + (a-1))
	negq	%rsi                        # rsi = -a  ==  ~(a-1)  -> THIS is the mask
	andq	%rsi, %rax                  # rax = (n+a-1) & ~(a-1)  = rounded-up value
	popq	%rbp                        # EPILOGUE: restore caller's frame pointer
	retq                                # return rax
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up

# =============================================================================
# le16(const u8 *p /*rdi*/) -> u16 /*ax*/
#   return p[0] | (p[1] << 8);
# THE LESSON: we wrote an explicit, endian-correct, alignment-free byte assembly
# — and on a little-endian target clang proves it is byte-for-byte identical to
# a 16-bit load, so it emits exactly one `movzwl`. Portable source, native speed.
# =============================================================================
	.globl	le16
	.p2align	4
	.type	le16,@function
le16:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movzwl	(%rdi), %eax                # eax = zero-extend( *(u16*)p ) = p[0]|p[1]<<8
	popq	%rbp                        # EPILOGUE
	retq                                # return the 16-bit value in ax
.Lfunc_end1:
	.size	le16, .Lfunc_end1-le16

# =============================================================================
# le32(const u8 *p /*rdi*/) -> u32 /*eax*/
#   return p[0] | p[1]<<8 | p[2]<<16 | p[3]<<24;
# Same story one size up: four ORed, shifted bytes fold into a single `movl`.
# =============================================================================
	.globl	le32
	.p2align	4
	.type	le32,@function
le32:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movl	(%rdi), %eax                # eax = *(u32*)p  (one aligned-or-not 32-bit load)
	popq	%rbp                        # EPILOGUE
	retq                                # return the 32-bit value in eax
.Lfunc_end2:
	.size	le32, .Lfunc_end2-le32

# =============================================================================
# key_cmp(const u8 *a /*rdi*/, u16 alen /*si*/, const u8 *b /*rdx*/, u16 blen /*cx*/) -> int /*eax*/
#   m = min(alen,blen); compare m bytes; first difference decides; else the
#   shorter key is smaller. Unsigned byte order; returns <0 / 0 / >0.
# Note how the u16 length args (si, cx) are used narrowly and zero-extended only
# where a full-width index is needed.
# =============================================================================
	.globl	key_cmp
	.p2align	4
	.type	key_cmp,@function
key_cmp:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	# ---- m = min(alen, blen) ------------------------------------------------
	cmpw	%cx, %si                    # compare alen(si) - blen(cx); CF if alen<blen
	movl	%ecx, %eax                  # eax = blen
	cmovbl	%esi, %eax                  # if alen<blen, eax = alen  -> eax = min = m
	testl	%eax, %eax                  # m == 0 ?
	je	.LBB3_1                         # empty prefix: skip straight to length tiebreak
# %bb.3:  (m > 0: set up the byte-compare loop)
	movzwl	%cx, %r8d                   # r8 = blen (zero-extended to 64 bits)
	movzwl	%si, %eax                   # rax = alen
	cmpq	%rax, %r8                   # blen - alen; CF if blen<alen
	cmovbq	%r8, %rax                   # rax = min(alen,blen) = m  (as a 64-bit loop bound)
	xorl	%r8d, %r8d                   # r8 = i = 0
	.p2align	4
.LBB3_4:                                # ---- for (i=0; i<m; i++) ----
	movzbl	(%rdi,%r8), %r9d            # r9b = a[i]
	movzbl	(%rdx,%r8), %r10d           # r10b = b[i]
	cmpb	%r10b, %r9b                 # a[i] - b[i]
	jne	.LBB3_5                         # first differing byte -> decide by it
# %bb.2:  (bytes equal; advance)
	incq	%r8                         # i++
	cmpw	%r8w, %ax                   # i == m ?  (ax holds m)
	jne	.LBB3_4                         # loop while i < m
.LBB3_1:                                # ---- prefix fully equal (or m==0) -------
	xorl	%edi, %edi                   # dil = 0 : "a byte differed" flag = false
                                        # implicit-def: $edx  (edx unused on this path)
	jmp	.LBB3_6                         # go do the length tiebreak
.LBB3_5:                                # ---- bytes differ at i ------------------
	xorl	%edx, %edx                   # edx = 0
	cmpb	%r10b, %r9b                 # a[i] - b[i]; CF set iff a[i] < b[i] (unsigned)
	sbbl	%edx, %edx                   # edx = 0 - CF = -(a[i]<b[i]) -> -1 or 0
	orl	$1, %edx                        # edx = (a[i]<b[i]) ? -1 : 1   (the byte verdict)
	movb	$1, %dil                    # dil = 1 : "a byte differed" flag = true
.LBB3_6:                                # ---- combine byte verdict with length ---
	cmpw	%cx, %si                    # alen(si) - blen(cx)
	seta	%al                         # al = (alen > blen) ? 1 : 0   (unsigned above)
	sbbb	$0, %al                     # al -= CF(alen<blen) -> al = sign(alen-blen) in {-1,0,1}
	movsbl	%al, %eax                   # eax = sign-extended length verdict
	testb	%dil, %dil                  # did a byte differ?
	cmovnel	%edx, %eax                  # if yes, the byte verdict wins; else keep length verdict
	popq	%rbp                        # EPILOGUE
	retq                                # return <0 / 0 / >0 in eax
.Lfunc_end3:
	.size	key_cmp, .Lfunc_end3-key_cmp

# =============================================================================
# slot_lower_bound(const u8 *page /*rdi*/, u16 nslots /*si*/,
#                  const u8 *key /*rdx*/,  u16 klen /*cx*/) -> int /*eax*/
#
#   lo=0; hi=nslots;
#   while (lo<hi){ mid=(lo+hi)>>1;
#                  off=le16(page+16+mid*2); ml=le16(page+off);
#                  if (key_cmp(page+off+6, ml, key, klen) < 0) lo=mid+1; else hi=mid; }
#   return lo;
#
# This is THE routine — the binary search every get/put/del runs over a slotted
# page's sorted slot array. clang did three notable things at -O1:
#   * INLINED key_cmp entirely (no `call`; that is why key_cmp's logic reappears
#     below, fused into the loop).
#   * Kept lo in eax and hi in esi across the whole loop; computes the slot byte
#     address as (lo+hi)&~1 (== mid*2) and mid as (lo+hi)>>1, reusing lo+hi.
#   * Made the lo/hi update BRANCHLESS with cmov (see the LBB4_10 block).
# Register roles inside the loop:  eax=lo  esi=hi  rdi=page  rdx=key  cx=klen
# =============================================================================
	.globl	slot_lower_bound
	.p2align	4
	.type	slot_lower_bound,@function
slot_lower_bound:
	testl	%esi, %esi                  # nslots == 0 ?
	je	.LBB4_1                         # empty page: return lo=0 immediately
# %bb.3:  (PROLOGUE — this path uses callee-saved rbx/r14, so save them)
	pushq	%rbp                        # save frame pointer
	movq	%rsp, %rbp
	pushq	%r14                        # save callee-saved r14 (used as key[i] temp)
	pushq	%rbx                        # save callee-saved rbx (used as inner index i)
	movzwl	%si, %esi                   # esi = hi = (unsigned)nslots
	xorl	%eax, %eax                   # eax = lo = 0
	jmp	.LBB4_4                         # enter the loop at its header
	.p2align	4
.LBB4_5:                                # reached when the compared prefix is EQUAL
                                        #   (either m==0 or all m bytes matched):
	xorl	%r10d, %r10d                # r10b = 0 : "a byte differed" flag = false
	xorl	%r11d, %r11d                # r11b = 0 : per-byte verdict placeholder
.LBB4_10:                               # ---- compute key_cmp<0 and update lo/hi ----
	sarl	%r8d                        # r8d = (lo+hi) >> 1 = mid  (r8d held lo+hi)
	xorl	%ebx, %ebx
	cmpw	%cx, %r9w                   # ml(r9w) - klen(cx); CF if ml<klen
	setb	%bl                         # bl = (ml < klen) : the length-based "mk<key"
	movzbl	%r11b, %r9d                 # r9d = per-byte verdict (mk[i]<key[i] ? 1 : 0)
	testb	%r10b, %r10b                # did a byte differ?
	cmovel	%ebx, %r9d                  # if NOT (prefix equal), use the length verdict bl
                                        #   => r9b now == (key_cmp(...) < 0)
	leal	1(%r8), %r10d               # r10d = mid + 1  (the lo-goes-right candidate)
	testb	%r9b, %r9b                  # key_cmp < 0 ? (i.e. mid's key < target -> go right)
	cmovnel	%esi, %r8d                  # if go-right: r8d = hi (so hi stays); else r8d = mid
	cmovnel	%r10d, %eax                 # if go-right: lo = mid + 1
	movl	%r8d, %esi                  # hi = r8d  (== hi when going right, == mid when left)
	cmpl	%r8d, %eax                  # lo - hi
	jge	.LBB4_11                        # loop exits when lo >= hi
.LBB4_4:                                # ---- loop header: mid = (lo+hi)>>1; load slot ----
	leal	(%rax,%rsi), %r8d           # r8d = lo + hi
	movl	%r8d, %r9d                  # r9d = lo + hi
	andl	$-2, %r9d                   # r9d = (lo+hi) & ~1  = mid*2  (the slot's byte offset)
	movzwl	16(%rdi,%r9), %r10d         # r10 = le16(page + 16 + mid*2) = off  (slot -> cell offset)
	movzwl	(%rdi,%r10), %r9d           # r9d = le16(page + off) = ml  (that cell's key_len)
	# ---- m = min(ml, klen) (inlined from key_cmp) ---------------------------
	cmpw	%cx, %r9w                   # ml - klen
	movl	%r9d, %r11d                 # r11d = ml
	cmovael	%ecx, %r11d                 # if ml>=klen, r11d = klen  -> r11d = min = m
	testw	%r11w, %r11w                # m == 0 ?
	je	.LBB4_5                         # empty prefix: decide by length only
# %bb.7:  (m>0: compare the key bytes of cell against the search key)
	addq	%rdi, %r10                  # r10 = page + off  (base of the cell)
	movzwl	%r11w, %r11d                # r11 = m (zero-extended loop bound)
	xorl	%ebx, %ebx                   # rbx = i = 0
	.p2align	4
.LBB4_8:                                # ---- inner: for (i=0;i<m;i++) compare bytes ----
	movzbl	(%rdx,%rbx), %r14d          # r14b = key[i]
	cmpb	%r14b, 6(%r10,%rbx)         # mk[i] (at page+off+6+i) - key[i]
	jne	.LBB4_9                         # first differing byte -> decide by it
# %bb.6:  (bytes equal; advance)
	incq	%rbx                        # i++
	cmpq	%rbx, %r11                  # m - i
	jne	.LBB4_8                         # loop while i < m
	jmp	.LBB4_5                         # all m bytes equal -> length tiebreak
	.p2align	4
.LBB4_9:                                # ---- key bytes differ at i --------------
	setb	%r11b                       # r11b = (mk[i] < key[i]) : per-byte verdict
	movb	$1, %r10b                   # r10b = 1 : "a byte differed" flag = true
	jmp	.LBB4_10                        # merge to compute key_cmp<0 and update lo/hi
.LBB4_11:                               # ---- EPILOGUE (loop done) ---------------
	popq	%rbx                        # restore callee-saved regs, in reverse push order
	popq	%r14
	popq	%rbp
                                        # kill: def $eax killed $eax killed $rax
	retq                                # return lo in eax (the lower-bound index)
.LBB4_1:                                # ---- empty page early-out ---------------
	xorl	%eax, %eax                   # return 0
                                        # kill: def $eax killed $eax killed $rax
	retq
.Lfunc_end4:
	.size	slot_lower_bound, .Lfunc_end4-slot_lower_bound

# =============================================================================
# crc32_byte(u32 crc /*edi*/, u8 byte /*sil*/) -> u32 /*eax*/
#   crc ^= byte; repeat 8 times: crc = (crc&1) ? (0xEDB88320 ^ (crc>>1)) : (crc>>1);
# One byte of the reflected CRC32. The `if` inside the loop is compiled to a
# branchless `cmov`: compute BOTH the plain shift and the shift-XOR-polynomial,
# then select on the low bit. This is the un-tabled math the 256-entry table in
# ../crc32.c precomputes.
# =============================================================================
	.globl	crc32_byte
	.p2align	4
	.type	crc32_byte,@function
crc32_byte:
	pushq	%rbp                        # PROLOGUE
	movq	%rsp, %rbp
	movl	%edi, %eax                  # eax = crc
	xorl	%esi, %eax                  # eax = crc ^ byte  (mix the input byte in)
	movl	$8, %ecx                    # ecx = loop counter = 8 bits
	.p2align	4
.LBB5_1:                                # ---- for (k=0; k<8; k++) ----
	movl	%eax, %edx                  # edx = crc
	shrl	%edx                        # edx = crc >> 1                 (the "bit was 0" result)
	movl	%edx, %esi                  # esi = crc >> 1
	xorl	$-306674912, %esi           # esi = (crc>>1) ^ 0xEDB88320    (the "bit was 1" result)
	testb	$1, %al                     # low bit of crc set?
	movl	%esi, %eax                  # eax = the XOR result (assume bit was 1)...
	cmovel	%edx, %eax                  # ...but if low bit was 0, take the plain shift instead
	decl	%ecx                        # k++ (counting down)
	jne	.LBB5_1                         # repeat for all 8 bits
# %bb.2:
	popq	%rbp                        # EPILOGUE
	retq                                # return the updated crc in eax
.Lfunc_end5:
	.size	crc32_byte, .Lfunc_end5-crc32_byte

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits   # non-executable stack (security default)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * Hand-written little-endian decoders (le16/le32) cost NOTHING on a
#     little-endian CPU: clang folds the shift/OR chain into one load. You keep
#     portability and give up no speed.
#   * align_up turns an alignment mask into a single `neg` (~(a-1) == -a) — the
#     move that makes O_DIRECT/mmap offset rounding two instructions.
#   * key_cmp's length tiebreak is a branchless sign() via seta+sbb; the whole
#     function then INLINES into slot_lower_bound (no call overhead in the hot
#     search path).
#   * slot_lower_bound is a branchless binary search: lo stays in eax, hi in esi,
#     and the "lo=mid+1 else hi=mid" update is done with cmov instead of a
#     mispredict-prone branch. The slot arithmetic (16 + mid*2 -> off -> +6 for
#     the key) is visible as `movzwl 16(%rdi,%r9)` then `(%rdi,%r10)` then
#     `6(%r10,%rbx)`.
#   * crc32_byte's inner `if` is a cmov too: compute both candidates, select on
#     the shifted-out bit. The table-driven crc32.c just caches these 8 steps.
#   * Compare with demo.O0.s (every value spilled to the stack, every branch
#     real) and demo.O2.s (same logic, tuned harder for the branch predictor).
# =============================================================================
