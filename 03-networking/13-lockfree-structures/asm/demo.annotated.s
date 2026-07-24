# =============================================================================
# demo.annotated.s — clang -O1 output for demo.c, explained instruction by
#                    instruction. The whole lock-free stack is TWO `lock
#                    cmpxchgq` loops; everything else is packing bits.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# AT&T syntax:  op  source, destination     (e.g. `movq (%rdi), %rax` => rax = *rdi)
#   %reg         a register (rax/eax/ax/al are the same reg at 64/32/16/8 bits)
#   $imm         an immediate constant
#   N(%reg)      memory at address reg+N
#   movabsq      load a full 64-bit immediate (the 32-bit `mov` can't hold it)
#
# THE SysV AMD64 ABI (what these functions obey)
# ----------------------------------------------
#   integer/pointer args, in order : rdi, rsi, rdx, rcx, r8, r9   (then stack)
#   return value                   : rax
#   callee-saved (must preserve)   : rbx, rbp, r12-r15
#   caller-saved (scratch)         : rax, rcx, rdx, rsi, rdi, r8-r11
#   red zone                       : 128 bytes below rsp, free for leaf funcs
#   stack alignment                : rsp % 16 == 0 at every `call`
# Both functions here are leaves (they call nothing), so they touch no stack
# beyond the textbook `push %rbp` frame that -O1 keeps for a clean backtrace.
#
# THE PACKED TAGGED POINTER (recap from demo.c)
# ---------------------------------------------
#   a head word = [ tag : bits 63..48 ] [ pointer : bits 47..0 ]
#   PTR_MASK = 0x0000FFFFFFFFFFFF   (low 48 bits — the node address)
#   TAG_MASK = 0xFFFF000000000000   (high 16 bits — the ABA version counter)
#   pack(p,tag) = (tag << 48) | (p & PTR_MASK)
# One 8-byte word holds both, so ONE `lock cmpxchgq` swaps pointer+tag together
# atomically. `lock` asserts the bus/cache lock so the read-compare-write is
# indivisible across cores; that is the hardware primitive the whole file rests
# on. The ABA counter rides in the top 16 bits precisely so a "same pointer,
# but the world moved on" CAS is a DIFFERENT 64-bit value and therefore fails.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# demo_push(u64 *head /rdi, struct node *n /rsi)  ->  void
#
# C:   old = load(head, relaxed)
#      do { n->next = ptr_of(old);
#           neu     = pack(n, tag_of(old)+1);
#      } while(!CAS(head,&old,neu, release, relaxed));
#
# The optimizer split `neu` into a loop-INVARIANT part (n's pointer bits plus a
# single tag increment — n never changes) computed once before the loop, and a
# loop-VARIANT part (old's tag bits) folded in each iteration.
# =============================================================================
	.globl	demo_push
	.p2align	4
	.type	demo_push,@function
demo_push:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp                    # save caller's frame pointer
	movq	%rsp, %rbp              # rbp = frame base (kept only for backtraces)

# ---- constants + the relaxed initial load ----------------------------------
	movabsq	$281474976710655, %rcx  # rcx = 0x0000FFFFFFFFFFFF = PTR_MASK
	movq	(%rdi), %rax            # rax = old = *head. This is the RELAXED
                                        #   atomic load: on x86 an aligned 8-byte
                                        #   load is already atomic, so it is a
                                        #   plain `movq` — no fence needed. rax is
                                        #   the CAS "expected" reg from here on.

# ---- precompute the loop-invariant half of `neu` ---------------------------
# neu = (old & TAG_MASK) + (n & PTR_MASK) + (1<<48). Only the first term depends
# on `old`; the rest is constant, so hoist it into rdx once:
	movq	%rsi, %rdx              # rdx = n
	andq	%rcx, %rdx              # rdx = n & PTR_MASK  (n's 48-bit pointer)
	addq	%rcx, %rdx              # rdx += 0xFFFFFFFFFFFF
	incq	%rdx                    # rdx += 1  => rdx = (n&PTR_MASK) + 0x1_0000_0000_0000
                                        #   i.e. n's pointer with ONE tag unit (bit 48)
                                        #   already added. (0xFFFFFFFFFFFF + 1 = 1<<48.)
	movabsq	$-281474976710656, %r8  # r8 = 0xFFFF000000000000 = TAG_MASK
	.p2align	4

# ---- the CAS retry loop -----------------------------------------------------
.LBB0_1:                                # do { ... } while(CAS fails)
	movq	%rax, %r9              # r9  = old               (will become `neu`)
	movq	%rax, %r10             # r10 = old               (will become top ptr)
	andq	%rcx, %r10             # r10 = old & PTR_MASK = ptr_of(old) = current top
	movq	%r10, (%rsi)           # n->next = current top.  (%rsi)=n, offset 0 = ->next.
                                       #   Re-done every spin because `old` was
                                       #   reloaded by a failed CAS.
	andq	%r8, %r9              # r9 = old & TAG_MASK   (old's tag, in place)
	addq	%rdx, %r9              # r9 = (old&TAG_MASK) + (n&PTR_MASK) + (1<<48)
                                       #    = pack(n, tag_of(old)+1) = neu   ✓
	lock	cmpxchgq	%r9, (%rdi)
                                       # THE heart of push. Atomically:
                                       #   if (*head == rax/old) { *head = r9/neu; ZF=1 }
                                       #   else                  { rax = *head;    ZF=0 }
                                       # `lock` makes the load-compare-store one
                                       # indivisible RMW; RELEASE ordering means
                                       # every prior write (n->next, n->value) is
                                       # globally visible before the new head is —
                                       # a popper that acquire-loads head sees a
                                       # fully-built node.
	jne	.LBB0_1               # ZF==0 => another thread changed head; rax now
                                       #   holds the fresh value, so retry.

# ---- EPILOGUE ---------------------------------------------------------------
	popq	%rbp                   # restore caller frame pointer
	retq                           # return (void)
.Lfunc_end0:
	.size	demo_push, .Lfunc_end0-demo_push

# =============================================================================
# demo_pop(u64 *head /rdi)  ->  struct node * /rax
#
# C:   old = load(head, acquire)
#      do { top = ptr_of(old);
#           if(!top) return 0;
#           neu = pack(top->next, tag_of(old)+1);
#      } while(!CAS(head,&old,neu, acquire, acquire));
#      return top;
# =============================================================================
	.globl	demo_pop
	.p2align	4
	.type	demo_pop,@function
demo_pop:
# ---- PROLOGUE ---------------------------------------------------------------
	pushq	%rbp
	movq	%rsp, %rbp

# ---- constants + the acquire initial load ----------------------------------
	movabsq	$281474976710655, %rdx  # rdx = PTR_MASK
	movq	(%rdi), %rax            # rax = old = *head. ACQUIRE load; again a
                                        #   plain aligned `movq` on x86 (loads have
                                        #   acquire semantics in the TSO model), but
                                        #   the C ordering forbids the compiler from
                                        #   hoisting the coming top->next read above it.
	xorl	%ecx, %ecx              # rcx = 0 = the return value for the EMPTY path
                                        #   (xor-zeroing: 2 bytes, breaks deps).
	movabsq	$-281474976710656, %rsi # rsi = TAG_MASK
	.p2align	4

# ---- the CAS retry loop -----------------------------------------------------
.LBB1_1:
	movq	%rax, %r8             # r8 = old
	andq	%rdx, %r8             # r8 = old & PTR_MASK = ptr_of(old) = top
	je	.LBB1_4               # if top == 0 the AND set ZF => stack empty;
                                       #   jump out with rcx(=0) as the result.
# %bb.2  (top != NULL): build neu = pack(top->next, tag_of(old)+1)
	movq	%rax, %r9             # r9 = old
	andq	%rsi, %r9             # r9 = old & TAG_MASK  (old's tag bits)
	orq	%rdx, %r9             # r9 = (old&TAG_MASK) | PTR_MASK  (set all 48 low bits)
	movq	(%r8), %r10            # r10 = *(top) = top->next  (offset 0 = ->next).
                                       #   Dereferencing top is safe only because the
                                       #   library keeps nodes type-stable (see README);
                                       #   the tag guards the CAS, not this load.
	andq	%rdx, %r10            # r10 = top->next & PTR_MASK
	addq	%r10, %r9             # r9 = ((old&TAG_MASK)|PTR_MASK) + (top->next&PTR_MASK)
	incq	%r9                   # +1. Optimizer trick: low 48 bits were all 1s, so
                                       #   adding (next&mask)+1 carries EXACTLY into bit
                                       #   48 — result = (old tag + 1)<<48 | (next&mask)
                                       #   = pack(top->next, tag_of(old)+1) = neu.  ✓
	lock	cmpxchgq	%r9, (%rdi)
                                       # if(*head==old){*head=neu; ZF=1} else{rax=*head; ZF=0}
                                       # ACQUIRE on success: the winner goes on to read
                                       # top->value, and acquire guarantees that read
                                       # sees the pusher's published node. ACQUIRE on
                                       # failure too: rax now names a DIFFERENT top we
                                       # will dereference next iteration, so it must be
                                       # acquired as well.
	jne	.LBB1_1               # lost the race -> retry with the reloaded old.
# %bb.3  (CAS won):
	movq	%r8, %rcx             # rcx = top  (the node we unlinked = return value)
.LBB1_4:                               # common exit (rcx holds top, or 0 if empty)
	movq	%rcx, %rax             # rax = return value (SysV: result in rax)
	popq	%rbp
	retq
.Lfunc_end1:
	.size	demo_pop, .Lfunc_end1-demo_pop

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits # non-executable stack (security default)
	.addrsig
# =============================================================================
# WHAT TO TAKE AWAY
#   * A lock-free stack is literally a `lock cmpxchg` in a retry loop. The `lock`
#     prefix is the atomicity; the loop is the "optimistic concurrency" (assume
#     no conflict, verify with the CAS, retry the rare conflict).
#   * The ABA tag is just the high 16 bits of the same word. It costs a couple
#     of `and`/`add` instructions and turns a silently-wrong CAS into a correct
#     retry. Compare demo.O0.s to see the same logic before the optimizer folds
#     the tag math into three instructions.
#   * Memory order is invisible in the x86 asm here (TSO makes plain movs do the
#     work), but it is NOT free: it constrains what the *compiler* may reorder,
#     and on weakly-ordered ISAs (arm64) release/acquire become real barriers
#     (stlr/ldar). Never omit it just because x86 "seems" to work.
# =============================================================================
