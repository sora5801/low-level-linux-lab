	.file	"syscall_table.c"
	.text
	.globl	syscall_detail                  # -- Begin function syscall_detail
	.p2align	4
	.type	syscall_detail,@function
syscall_detail:                         # @syscall_detail
# %bb.0:
	cmpq	$334, %rdi                      # imm = 0x14E
	jbe	.LBB0_2
# %bb.1:
	xorl	%eax, %eax
	retq
.LBB0_2:
	shlq	$4, %rdi
	leaq	detail(%rip), %rcx
	leaq	(%rcx,%rdi), %rax
	movq	(%rdi,%rcx), %rcx
	testq	%rcx, %rcx
	cmoveq	%rcx, %rax
	retq
.Lfunc_end0:
	.size	syscall_detail, .Lfunc_end0-syscall_detail
                                        # -- End function
	.globl	syscall_name                    # -- Begin function syscall_name
	.p2align	4
	.type	syscall_name,@function
syscall_name:                           # @syscall_name
# %bb.0:
	cmpq	$439, %rdi                      # imm = 0x1B7
	jbe	.LBB1_2
# %bb.1:
	xorl	%eax, %eax
	retq
.LBB1_2:
	leaq	names(%rip), %rax
	movq	(%rax,%rdi,8), %rax
	retq
.Lfunc_end1:
	.size	syscall_name, .Lfunc_end1-syscall_name
                                        # -- End function
	.globl	syscall_table_size              # -- Begin function syscall_table_size
	.p2align	4
	.type	syscall_table_size,@function
syscall_table_size:                     # @syscall_table_size
# %bb.0:
	movl	$440, %eax                      # imm = 0x1B8
	retq
.Lfunc_end2:
	.size	syscall_table_size, .Lfunc_end2-syscall_table_size
                                        # -- End function
	.type	names,@object                   # @names
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
names:
	.quad	.L.str
	.quad	.L.str.1
	.quad	.L.str.2
	.quad	.L.str.3
	.quad	.L.str.4
	.quad	.L.str.5
	.quad	.L.str.6
	.quad	.L.str.45
	.quad	.L.str.7
	.quad	.L.str.8
	.quad	.L.str.9
	.quad	.L.str.10
	.quad	.L.str.11
	.quad	.L.str.12
	.quad	.L.str.13
	.quad	.L.str.46
	.quad	.L.str.14
	.quad	.L.str.15
	.quad	.L.str.47
	.quad	.L.str.48
	.quad	.L.str.16
	.quad	.L.str.17
	.quad	.L.str.49
	.quad	.L.str.50
	.quad	.L.str.51
	.quad	.L.str.52
	.quad	.L.str.53
	.quad	.L.str.54
	.quad	.L.str.55
	.quad	.L.str.56
	.quad	.L.str.57
	.quad	.L.str.58
	.quad	.L.str.59
	.quad	.L.str.60
	.quad	.L.str.61
	.quad	.L.str.62
	.quad	.L.str.63
	.quad	.L.str.64
	.quad	.L.str.65
	.quad	.L.str.18
	.quad	.L.str.66
	.quad	.L.str.67
	.quad	.L.str.68
	.quad	.L.str.69
	.quad	.L.str.70
	.quad	.L.str.71
	.quad	.L.str.72
	.quad	.L.str.73
	.quad	.L.str.74
	.quad	.L.str.75
	.quad	.L.str.76
	.quad	.L.str.77
	.quad	.L.str.78
	.quad	.L.str.79
	.quad	.L.str.80
	.quad	.L.str.81
	.quad	.L.str.82
	.quad	.L.str.83
	.quad	.L.str.84
	.quad	.L.str.19
	.quad	.L.str.20
	.quad	.L.str.85
	.quad	.L.str.21
	.quad	.L.str.22
	.quad	.L.str.86
	.quad	.L.str.87
	.quad	.L.str.88
	.quad	.L.str.89
	.quad	.L.str.90
	.quad	.L.str.91
	.quad	.L.str.92
	.quad	.L.str.93
	.quad	.L.str.23
	.quad	.L.str.94
	.quad	.L.str.95
	.quad	.L.str.96
	.quad	.L.str.97
	.quad	.L.str.98
	.quad	.L.str.99
	.quad	.L.str.24
	.quad	.L.str.100
	.quad	.L.str.101
	.quad	.L.str.102
	.quad	.L.str.103
	.quad	.L.str.104
	.quad	.L.str.105
	.quad	.L.str.106
	.quad	.L.str.107
	.quad	.L.str.108
	.quad	.L.str.25
	.quad	.L.str.109
	.quad	.L.str.110
	.quad	.L.str.111
	.quad	.L.str.112
	.quad	.L.str.113
	.quad	.L.str.114
	.quad	.L.str.26
	.quad	.L.str.115
	.quad	.L.str.116
	.quad	.L.str.117
	.quad	.L.str.118
	.quad	.L.str.119
	.quad	.L.str.27
	.quad	.L.str.120
	.quad	.L.str.28
	.quad	.L.str.121
	.quad	.L.str.122
	.quad	.L.str.29
	.quad	.L.str.30
	.quad	.L.str.123
	.quad	.L.str.124
	.quad	.L.str.125
	.quad	.L.str.126
	.quad	.L.str.127
	.quad	.L.str.128
	.quad	.L.str.129
	.quad	.L.str.130
	.quad	.L.str.131
	.quad	.L.str.132
	.quad	.L.str.133
	.quad	.L.str.134
	.quad	.L.str.135
	.quad	.L.str.136
	.quad	.L.str.137
	.quad	.L.str.138
	.quad	.L.str.139
	.quad	.L.str.140
	.quad	.L.str.141
	.quad	.L.str.142
	.quad	.L.str.143
	.quad	.L.str.144
	.quad	.L.str.145
	.quad	.L.str.146
	.quad	.L.str.147
	.quad	.L.str.148
	.quad	.L.str.149
	.quad	.L.str.150
	.quad	.L.str.151
	.quad	.L.str.152
	.quad	.L.str.153
	.quad	.L.str.154
	.quad	.L.str.155
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.156
	.quad	.L.str.31
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.157
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.158
	.quad	.L.str.32
	.quad	0
	.quad	.L.str.159
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.160
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.33
	.quad	.L.str.34
	.quad	0
	.quad	0
	.quad	.L.str.161
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.35
	.quad	.L.str.162
	.quad	.L.str.163
	.quad	.L.str.36
	.quad	.L.str.164
	.quad	.L.str.165
	.quad	.L.str.166
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.37
	.quad	.L.str.167
	.quad	.L.str.168
	.quad	.L.str.169
	.quad	0
	.quad	.L.str.38
	.quad	.L.str.170
	.quad	.L.str.171
	.quad	.L.str.172
	.quad	.L.str.173
	.quad	.L.str.174
	.quad	.L.str.175
	.quad	.L.str.176
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.39
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.177
	.quad	.L.str.178
	.quad	0
	.quad	0
	.quad	.L.str.179
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.180
	.quad	0
	.quad	.L.str.181
	.quad	.L.str.182
	.quad	.L.str.183
	.quad	.L.str.184
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.40
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.185
	.quad	0
	.quad	.L.str.41
	.quad	.L.str.186
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.42
	.quad	0
	.quad	.L.str.43
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.187
	.quad	0
	.quad	0
	.quad	0
	.quad	.L.str.188
	.size	names, 3520

	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"read"
	.size	.L.str, 5

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"write"
	.size	.L.str.1, 6

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"open"
	.size	.L.str.2, 5

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"close"
	.size	.L.str.3, 6

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"stat"
	.size	.L.str.4, 5

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"fstat"
	.size	.L.str.5, 6

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"lstat"
	.size	.L.str.6, 6

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"lseek"
	.size	.L.str.7, 6

	.type	.L.str.8,@object                # @.str.8
.L.str.8:
	.asciz	"mmap"
	.size	.L.str.8, 5

	.type	.L.str.9,@object                # @.str.9
.L.str.9:
	.asciz	"mprotect"
	.size	.L.str.9, 9

	.type	.L.str.10,@object               # @.str.10
.L.str.10:
	.asciz	"munmap"
	.size	.L.str.10, 7

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"brk"
	.size	.L.str.11, 4

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"rt_sigaction"
	.size	.L.str.12, 13

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"rt_sigprocmask"
	.size	.L.str.13, 15

	.type	.L.str.14,@object               # @.str.14
.L.str.14:
	.asciz	"ioctl"
	.size	.L.str.14, 6

	.type	.L.str.15,@object               # @.str.15
.L.str.15:
	.asciz	"pread64"
	.size	.L.str.15, 8

	.type	.L.str.16,@object               # @.str.16
.L.str.16:
	.asciz	"writev"
	.size	.L.str.16, 7

	.type	.L.str.17,@object               # @.str.17
.L.str.17:
	.asciz	"access"
	.size	.L.str.17, 7

	.type	.L.str.18,@object               # @.str.18
.L.str.18:
	.asciz	"getpid"
	.size	.L.str.18, 7

	.type	.L.str.19,@object               # @.str.19
.L.str.19:
	.asciz	"execve"
	.size	.L.str.19, 7

	.type	.L.str.20,@object               # @.str.20
.L.str.20:
	.asciz	"exit"
	.size	.L.str.20, 5

	.type	.L.str.21,@object               # @.str.21
.L.str.21:
	.asciz	"kill"
	.size	.L.str.21, 5

	.type	.L.str.22,@object               # @.str.22
.L.str.22:
	.asciz	"uname"
	.size	.L.str.22, 6

	.type	.L.str.23,@object               # @.str.23
.L.str.23:
	.asciz	"fcntl"
	.size	.L.str.23, 6

	.type	.L.str.24,@object               # @.str.24
.L.str.24:
	.asciz	"getcwd"
	.size	.L.str.24, 7

	.type	.L.str.25,@object               # @.str.25
.L.str.25:
	.asciz	"readlink"
	.size	.L.str.25, 9

	.type	.L.str.26,@object               # @.str.26
.L.str.26:
	.asciz	"gettimeofday"
	.size	.L.str.26, 13

	.type	.L.str.27,@object               # @.str.27
.L.str.27:
	.asciz	"getuid"
	.size	.L.str.27, 7

	.type	.L.str.28,@object               # @.str.28
.L.str.28:
	.asciz	"getgid"
	.size	.L.str.28, 7

	.type	.L.str.29,@object               # @.str.29
.L.str.29:
	.asciz	"geteuid"
	.size	.L.str.29, 8

	.type	.L.str.30,@object               # @.str.30
.L.str.30:
	.asciz	"getegid"
	.size	.L.str.30, 8

	.type	.L.str.31,@object               # @.str.31
.L.str.31:
	.asciz	"arch_prctl"
	.size	.L.str.31, 11

	.type	.L.str.32,@object               # @.str.32
.L.str.32:
	.asciz	"futex"
	.size	.L.str.32, 6

	.type	.L.str.33,@object               # @.str.33
.L.str.33:
	.asciz	"getdents64"
	.size	.L.str.33, 11

	.type	.L.str.34,@object               # @.str.34
.L.str.34:
	.asciz	"set_tid_address"
	.size	.L.str.34, 16

	.type	.L.str.35,@object               # @.str.35
.L.str.35:
	.asciz	"clock_gettime"
	.size	.L.str.35, 14

	.type	.L.str.36,@object               # @.str.36
.L.str.36:
	.asciz	"exit_group"
	.size	.L.str.36, 11

	.type	.L.str.37,@object               # @.str.37
.L.str.37:
	.asciz	"openat"
	.size	.L.str.37, 7

	.type	.L.str.38,@object               # @.str.38
.L.str.38:
	.asciz	"newfstatat"
	.size	.L.str.38, 11

	.type	.L.str.39,@object               # @.str.39
.L.str.39:
	.asciz	"set_robust_list"
	.size	.L.str.39, 16

	.type	.L.str.40,@object               # @.str.40
.L.str.40:
	.asciz	"prlimit64"
	.size	.L.str.40, 10

	.type	.L.str.41,@object               # @.str.41
.L.str.41:
	.asciz	"getrandom"
	.size	.L.str.41, 10

	.type	.L.str.42,@object               # @.str.42
.L.str.42:
	.asciz	"statx"
	.size	.L.str.42, 6

	.type	.L.str.43,@object               # @.str.43
.L.str.43:
	.asciz	"rseq"
	.size	.L.str.43, 5

	.type	detail,@object                  # @detail
	.section	.data.rel.ro,"aw",@progbits
	.p2align	4, 0x0
detail:
	.quad	.L.str
	.byte	3                               # 0x3
	.asciz	"\006\004\007\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.1
	.byte	3                               # 0x3
	.asciz	"\006\005\007\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.2
	.byte	3                               # 0x3
	.asciz	"\005\b\t\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.3
	.byte	1                               # 0x1
	.asciz	"\006\000\000\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.4
	.byte	2                               # 0x2
	.asciz	"\005\004\000\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.5
	.byte	2                               # 0x2
	.asciz	"\006\004\000\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.6
	.byte	2                               # 0x2
	.asciz	"\005\004\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.quad	.L.str.7
	.byte	3                               # 0x3
	.asciz	"\006\002\r\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.8
	.byte	6                               # 0x6
	.ascii	"\004\007\n\013\006\002"
	.byte	0                               # 0x0
	.quad	.L.str.9
	.byte	3                               # 0x3
	.asciz	"\004\007\n\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.10
	.byte	2                               # 0x2
	.asciz	"\004\007\000\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.11
	.byte	1                               # 0x1
	.asciz	"\004\000\000\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.12
	.byte	4                               # 0x4
	.asciz	"\f\004\004\007\000"
	.byte	0                               # 0x0
	.quad	.L.str.13
	.byte	4                               # 0x4
	.asciz	"\001\004\004\007\000"
	.byte	0                               # 0x0
	.zero	16
	.quad	.L.str.14
	.byte	3                               # 0x3
	.asciz	"\006\003\003\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.15
	.byte	4                               # 0x4
	.asciz	"\006\004\007\002\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.quad	.L.str.16
	.byte	3                               # 0x3
	.asciz	"\006\004\001\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.17
	.byte	2                               # 0x2
	.asciz	"\005\001\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.18
	.byte	0                               # 0x0
	.zero	6
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.19
	.byte	3                               # 0x3
	.asciz	"\005\004\004\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.20
	.byte	1                               # 0x1
	.asciz	"\001\000\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.quad	.L.str.21
	.byte	2                               # 0x2
	.asciz	"\001\f\000\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.22
	.byte	1                               # 0x1
	.asciz	"\004\000\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.23
	.byte	3                               # 0x3
	.asciz	"\006\001\003\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.24
	.byte	2                               # 0x2
	.asciz	"\004\007\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.25
	.byte	3                               # 0x3
	.asciz	"\005\004\007\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.26
	.byte	2                               # 0x2
	.asciz	"\004\004\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.27
	.byte	0                               # 0x0
	.zero	6
	.byte	0                               # 0x0
	.zero	16
	.quad	.L.str.28
	.byte	0                               # 0x0
	.zero	6
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.quad	.L.str.29
	.byte	0                               # 0x0
	.zero	6
	.byte	0                               # 0x0
	.quad	.L.str.30
	.byte	0                               # 0x0
	.zero	6
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.31
	.byte	2                               # 0x2
	.asciz	"\001\003\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.32
	.byte	6                               # 0x6
	.ascii	"\004\001\001\004\004\001"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.33
	.byte	3                               # 0x3
	.asciz	"\006\004\007\000\000"
	.byte	0                               # 0x0
	.quad	.L.str.34
	.byte	1                               # 0x1
	.asciz	"\004\000\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.35
	.byte	2                               # 0x2
	.asciz	"\001\004\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.quad	.L.str.36
	.byte	1                               # 0x1
	.asciz	"\001\000\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.37
	.byte	4                               # 0x4
	.asciz	"\006\005\b\t\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.38
	.byte	4                               # 0x4
	.asciz	"\006\005\004\001\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.39
	.byte	2                               # 0x2
	.asciz	"\004\007\000\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.40
	.byte	4                               # 0x4
	.asciz	"\001\001\004\004\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.41
	.byte	3                               # 0x3
	.asciz	"\004\007\003\000\000"
	.byte	0                               # 0x0
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.zero	16
	.quad	.L.str.42
	.byte	5                               # 0x5
	.asciz	"\006\005\001\003\004"
	.byte	0                               # 0x0
	.zero	16
	.quad	.L.str.43
	.byte	4                               # 0x4
	.asciz	"\004\007\001\003\000"
	.byte	0                               # 0x0
	.size	detail, 5360

	.type	.L.str.45,@object               # @.str.45
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.45:
	.asciz	"poll"
	.size	.L.str.45, 5

	.type	.L.str.46,@object               # @.str.46
.L.str.46:
	.asciz	"rt_sigreturn"
	.size	.L.str.46, 13

	.type	.L.str.47,@object               # @.str.47
.L.str.47:
	.asciz	"pwrite64"
	.size	.L.str.47, 9

	.type	.L.str.48,@object               # @.str.48
.L.str.48:
	.asciz	"readv"
	.size	.L.str.48, 6

	.type	.L.str.49,@object               # @.str.49
.L.str.49:
	.asciz	"pipe"
	.size	.L.str.49, 5

	.type	.L.str.50,@object               # @.str.50
.L.str.50:
	.asciz	"select"
	.size	.L.str.50, 7

	.type	.L.str.51,@object               # @.str.51
.L.str.51:
	.asciz	"sched_yield"
	.size	.L.str.51, 12

	.type	.L.str.52,@object               # @.str.52
.L.str.52:
	.asciz	"mremap"
	.size	.L.str.52, 7

	.type	.L.str.53,@object               # @.str.53
.L.str.53:
	.asciz	"msync"
	.size	.L.str.53, 6

	.type	.L.str.54,@object               # @.str.54
.L.str.54:
	.asciz	"mincore"
	.size	.L.str.54, 8

	.type	.L.str.55,@object               # @.str.55
.L.str.55:
	.asciz	"madvise"
	.size	.L.str.55, 8

	.type	.L.str.56,@object               # @.str.56
.L.str.56:
	.asciz	"shmget"
	.size	.L.str.56, 7

	.type	.L.str.57,@object               # @.str.57
.L.str.57:
	.asciz	"shmat"
	.size	.L.str.57, 6

	.type	.L.str.58,@object               # @.str.58
.L.str.58:
	.asciz	"shmctl"
	.size	.L.str.58, 7

	.type	.L.str.59,@object               # @.str.59
.L.str.59:
	.asciz	"dup"
	.size	.L.str.59, 4

	.type	.L.str.60,@object               # @.str.60
.L.str.60:
	.asciz	"dup2"
	.size	.L.str.60, 5

	.type	.L.str.61,@object               # @.str.61
.L.str.61:
	.asciz	"pause"
	.size	.L.str.61, 6

	.type	.L.str.62,@object               # @.str.62
.L.str.62:
	.asciz	"nanosleep"
	.size	.L.str.62, 10

	.type	.L.str.63,@object               # @.str.63
.L.str.63:
	.asciz	"getitimer"
	.size	.L.str.63, 10

	.type	.L.str.64,@object               # @.str.64
.L.str.64:
	.asciz	"alarm"
	.size	.L.str.64, 6

	.type	.L.str.65,@object               # @.str.65
.L.str.65:
	.asciz	"setitimer"
	.size	.L.str.65, 10

	.type	.L.str.66,@object               # @.str.66
.L.str.66:
	.asciz	"sendfile"
	.size	.L.str.66, 9

	.type	.L.str.67,@object               # @.str.67
.L.str.67:
	.asciz	"socket"
	.size	.L.str.67, 7

	.type	.L.str.68,@object               # @.str.68
.L.str.68:
	.asciz	"connect"
	.size	.L.str.68, 8

	.type	.L.str.69,@object               # @.str.69
.L.str.69:
	.asciz	"accept"
	.size	.L.str.69, 7

	.type	.L.str.70,@object               # @.str.70
.L.str.70:
	.asciz	"sendto"
	.size	.L.str.70, 7

	.type	.L.str.71,@object               # @.str.71
.L.str.71:
	.asciz	"recvfrom"
	.size	.L.str.71, 9

	.type	.L.str.72,@object               # @.str.72
.L.str.72:
	.asciz	"sendmsg"
	.size	.L.str.72, 8

	.type	.L.str.73,@object               # @.str.73
.L.str.73:
	.asciz	"recvmsg"
	.size	.L.str.73, 8

	.type	.L.str.74,@object               # @.str.74
.L.str.74:
	.asciz	"shutdown"
	.size	.L.str.74, 9

	.type	.L.str.75,@object               # @.str.75
.L.str.75:
	.asciz	"bind"
	.size	.L.str.75, 5

	.type	.L.str.76,@object               # @.str.76
.L.str.76:
	.asciz	"listen"
	.size	.L.str.76, 7

	.type	.L.str.77,@object               # @.str.77
.L.str.77:
	.asciz	"getsockname"
	.size	.L.str.77, 12

	.type	.L.str.78,@object               # @.str.78
.L.str.78:
	.asciz	"getpeername"
	.size	.L.str.78, 12

	.type	.L.str.79,@object               # @.str.79
.L.str.79:
	.asciz	"socketpair"
	.size	.L.str.79, 11

	.type	.L.str.80,@object               # @.str.80
.L.str.80:
	.asciz	"setsockopt"
	.size	.L.str.80, 11

	.type	.L.str.81,@object               # @.str.81
.L.str.81:
	.asciz	"getsockopt"
	.size	.L.str.81, 11

	.type	.L.str.82,@object               # @.str.82
.L.str.82:
	.asciz	"clone"
	.size	.L.str.82, 6

	.type	.L.str.83,@object               # @.str.83
.L.str.83:
	.asciz	"fork"
	.size	.L.str.83, 5

	.type	.L.str.84,@object               # @.str.84
.L.str.84:
	.asciz	"vfork"
	.size	.L.str.84, 6

	.type	.L.str.85,@object               # @.str.85
.L.str.85:
	.asciz	"wait4"
	.size	.L.str.85, 6

	.type	.L.str.86,@object               # @.str.86
.L.str.86:
	.asciz	"semget"
	.size	.L.str.86, 7

	.type	.L.str.87,@object               # @.str.87
.L.str.87:
	.asciz	"semop"
	.size	.L.str.87, 6

	.type	.L.str.88,@object               # @.str.88
.L.str.88:
	.asciz	"semctl"
	.size	.L.str.88, 7

	.type	.L.str.89,@object               # @.str.89
.L.str.89:
	.asciz	"shmdt"
	.size	.L.str.89, 6

	.type	.L.str.90,@object               # @.str.90
.L.str.90:
	.asciz	"msgget"
	.size	.L.str.90, 7

	.type	.L.str.91,@object               # @.str.91
.L.str.91:
	.asciz	"msgsnd"
	.size	.L.str.91, 7

	.type	.L.str.92,@object               # @.str.92
.L.str.92:
	.asciz	"msgrcv"
	.size	.L.str.92, 7

	.type	.L.str.93,@object               # @.str.93
.L.str.93:
	.asciz	"msgctl"
	.size	.L.str.93, 7

	.type	.L.str.94,@object               # @.str.94
.L.str.94:
	.asciz	"flock"
	.size	.L.str.94, 6

	.type	.L.str.95,@object               # @.str.95
.L.str.95:
	.asciz	"fsync"
	.size	.L.str.95, 6

	.type	.L.str.96,@object               # @.str.96
.L.str.96:
	.asciz	"fdatasync"
	.size	.L.str.96, 10

	.type	.L.str.97,@object               # @.str.97
.L.str.97:
	.asciz	"truncate"
	.size	.L.str.97, 9

	.type	.L.str.98,@object               # @.str.98
.L.str.98:
	.asciz	"ftruncate"
	.size	.L.str.98, 10

	.type	.L.str.99,@object               # @.str.99
.L.str.99:
	.asciz	"getdents"
	.size	.L.str.99, 9

	.type	.L.str.100,@object              # @.str.100
.L.str.100:
	.asciz	"chdir"
	.size	.L.str.100, 6

	.type	.L.str.101,@object              # @.str.101
.L.str.101:
	.asciz	"fchdir"
	.size	.L.str.101, 7

	.type	.L.str.102,@object              # @.str.102
.L.str.102:
	.asciz	"rename"
	.size	.L.str.102, 7

	.type	.L.str.103,@object              # @.str.103
.L.str.103:
	.asciz	"mkdir"
	.size	.L.str.103, 6

	.type	.L.str.104,@object              # @.str.104
.L.str.104:
	.asciz	"rmdir"
	.size	.L.str.104, 6

	.type	.L.str.105,@object              # @.str.105
.L.str.105:
	.asciz	"creat"
	.size	.L.str.105, 6

	.type	.L.str.106,@object              # @.str.106
.L.str.106:
	.asciz	"link"
	.size	.L.str.106, 5

	.type	.L.str.107,@object              # @.str.107
.L.str.107:
	.asciz	"unlink"
	.size	.L.str.107, 7

	.type	.L.str.108,@object              # @.str.108
.L.str.108:
	.asciz	"symlink"
	.size	.L.str.108, 8

	.type	.L.str.109,@object              # @.str.109
.L.str.109:
	.asciz	"chmod"
	.size	.L.str.109, 6

	.type	.L.str.110,@object              # @.str.110
.L.str.110:
	.asciz	"fchmod"
	.size	.L.str.110, 7

	.type	.L.str.111,@object              # @.str.111
.L.str.111:
	.asciz	"chown"
	.size	.L.str.111, 6

	.type	.L.str.112,@object              # @.str.112
.L.str.112:
	.asciz	"fchown"
	.size	.L.str.112, 7

	.type	.L.str.113,@object              # @.str.113
.L.str.113:
	.asciz	"lchown"
	.size	.L.str.113, 7

	.type	.L.str.114,@object              # @.str.114
.L.str.114:
	.asciz	"umask"
	.size	.L.str.114, 6

	.type	.L.str.115,@object              # @.str.115
.L.str.115:
	.asciz	"getrlimit"
	.size	.L.str.115, 10

	.type	.L.str.116,@object              # @.str.116
.L.str.116:
	.asciz	"getrusage"
	.size	.L.str.116, 10

	.type	.L.str.117,@object              # @.str.117
.L.str.117:
	.asciz	"sysinfo"
	.size	.L.str.117, 8

	.type	.L.str.118,@object              # @.str.118
.L.str.118:
	.asciz	"times"
	.size	.L.str.118, 6

	.type	.L.str.119,@object              # @.str.119
.L.str.119:
	.asciz	"ptrace"
	.size	.L.str.119, 7

	.type	.L.str.120,@object              # @.str.120
.L.str.120:
	.asciz	"syslog"
	.size	.L.str.120, 7

	.type	.L.str.121,@object              # @.str.121
.L.str.121:
	.asciz	"setuid"
	.size	.L.str.121, 7

	.type	.L.str.122,@object              # @.str.122
.L.str.122:
	.asciz	"setgid"
	.size	.L.str.122, 7

	.type	.L.str.123,@object              # @.str.123
.L.str.123:
	.asciz	"setpgid"
	.size	.L.str.123, 8

	.type	.L.str.124,@object              # @.str.124
.L.str.124:
	.asciz	"getppid"
	.size	.L.str.124, 8

	.type	.L.str.125,@object              # @.str.125
.L.str.125:
	.asciz	"getpgrp"
	.size	.L.str.125, 8

	.type	.L.str.126,@object              # @.str.126
.L.str.126:
	.asciz	"setsid"
	.size	.L.str.126, 7

	.type	.L.str.127,@object              # @.str.127
.L.str.127:
	.asciz	"setreuid"
	.size	.L.str.127, 9

	.type	.L.str.128,@object              # @.str.128
.L.str.128:
	.asciz	"setregid"
	.size	.L.str.128, 9

	.type	.L.str.129,@object              # @.str.129
.L.str.129:
	.asciz	"getgroups"
	.size	.L.str.129, 10

	.type	.L.str.130,@object              # @.str.130
.L.str.130:
	.asciz	"setgroups"
	.size	.L.str.130, 10

	.type	.L.str.131,@object              # @.str.131
.L.str.131:
	.asciz	"setresuid"
	.size	.L.str.131, 10

	.type	.L.str.132,@object              # @.str.132
.L.str.132:
	.asciz	"getresuid"
	.size	.L.str.132, 10

	.type	.L.str.133,@object              # @.str.133
.L.str.133:
	.asciz	"setresgid"
	.size	.L.str.133, 10

	.type	.L.str.134,@object              # @.str.134
.L.str.134:
	.asciz	"getresgid"
	.size	.L.str.134, 10

	.type	.L.str.135,@object              # @.str.135
.L.str.135:
	.asciz	"getpgid"
	.size	.L.str.135, 8

	.type	.L.str.136,@object              # @.str.136
.L.str.136:
	.asciz	"setfsuid"
	.size	.L.str.136, 9

	.type	.L.str.137,@object              # @.str.137
.L.str.137:
	.asciz	"setfsgid"
	.size	.L.str.137, 9

	.type	.L.str.138,@object              # @.str.138
.L.str.138:
	.asciz	"getsid"
	.size	.L.str.138, 7

	.type	.L.str.139,@object              # @.str.139
.L.str.139:
	.asciz	"capget"
	.size	.L.str.139, 7

	.type	.L.str.140,@object              # @.str.140
.L.str.140:
	.asciz	"capset"
	.size	.L.str.140, 7

	.type	.L.str.141,@object              # @.str.141
.L.str.141:
	.asciz	"rt_sigpending"
	.size	.L.str.141, 14

	.type	.L.str.142,@object              # @.str.142
.L.str.142:
	.asciz	"rt_sigtimedwait"
	.size	.L.str.142, 16

	.type	.L.str.143,@object              # @.str.143
.L.str.143:
	.asciz	"rt_sigqueueinfo"
	.size	.L.str.143, 16

	.type	.L.str.144,@object              # @.str.144
.L.str.144:
	.asciz	"rt_sigsuspend"
	.size	.L.str.144, 14

	.type	.L.str.145,@object              # @.str.145
.L.str.145:
	.asciz	"sigaltstack"
	.size	.L.str.145, 12

	.type	.L.str.146,@object              # @.str.146
.L.str.146:
	.asciz	"utime"
	.size	.L.str.146, 6

	.type	.L.str.147,@object              # @.str.147
.L.str.147:
	.asciz	"mknod"
	.size	.L.str.147, 6

	.type	.L.str.148,@object              # @.str.148
.L.str.148:
	.asciz	"uselib"
	.size	.L.str.148, 7

	.type	.L.str.149,@object              # @.str.149
.L.str.149:
	.asciz	"personality"
	.size	.L.str.149, 12

	.type	.L.str.150,@object              # @.str.150
.L.str.150:
	.asciz	"ustat"
	.size	.L.str.150, 6

	.type	.L.str.151,@object              # @.str.151
.L.str.151:
	.asciz	"statfs"
	.size	.L.str.151, 7

	.type	.L.str.152,@object              # @.str.152
.L.str.152:
	.asciz	"fstatfs"
	.size	.L.str.152, 8

	.type	.L.str.153,@object              # @.str.153
.L.str.153:
	.asciz	"sysfs"
	.size	.L.str.153, 6

	.type	.L.str.154,@object              # @.str.154
.L.str.154:
	.asciz	"getpriority"
	.size	.L.str.154, 12

	.type	.L.str.155,@object              # @.str.155
.L.str.155:
	.asciz	"setpriority"
	.size	.L.str.155, 12

	.type	.L.str.156,@object              # @.str.156
.L.str.156:
	.asciz	"prctl"
	.size	.L.str.156, 6

	.type	.L.str.157,@object              # @.str.157
.L.str.157:
	.asciz	"gettid"
	.size	.L.str.157, 7

	.type	.L.str.158,@object              # @.str.158
.L.str.158:
	.asciz	"time"
	.size	.L.str.158, 5

	.type	.L.str.159,@object              # @.str.159
.L.str.159:
	.asciz	"sched_getaffinity"
	.size	.L.str.159, 18

	.type	.L.str.160,@object              # @.str.160
.L.str.160:
	.asciz	"epoll_create"
	.size	.L.str.160, 13

	.type	.L.str.161,@object              # @.str.161
.L.str.161:
	.asciz	"fadvise64"
	.size	.L.str.161, 10

	.type	.L.str.162,@object              # @.str.162
.L.str.162:
	.asciz	"clock_getres"
	.size	.L.str.162, 13

	.type	.L.str.163,@object              # @.str.163
.L.str.163:
	.asciz	"clock_nanosleep"
	.size	.L.str.163, 16

	.type	.L.str.164,@object              # @.str.164
.L.str.164:
	.asciz	"epoll_wait"
	.size	.L.str.164, 11

	.type	.L.str.165,@object              # @.str.165
.L.str.165:
	.asciz	"epoll_ctl"
	.size	.L.str.165, 10

	.type	.L.str.166,@object              # @.str.166
.L.str.166:
	.asciz	"tgkill"
	.size	.L.str.166, 7

	.type	.L.str.167,@object              # @.str.167
.L.str.167:
	.asciz	"mkdirat"
	.size	.L.str.167, 8

	.type	.L.str.168,@object              # @.str.168
.L.str.168:
	.asciz	"mknodat"
	.size	.L.str.168, 8

	.type	.L.str.169,@object              # @.str.169
.L.str.169:
	.asciz	"fchownat"
	.size	.L.str.169, 9

	.type	.L.str.170,@object              # @.str.170
.L.str.170:
	.asciz	"unlinkat"
	.size	.L.str.170, 9

	.type	.L.str.171,@object              # @.str.171
.L.str.171:
	.asciz	"renameat"
	.size	.L.str.171, 9

	.type	.L.str.172,@object              # @.str.172
.L.str.172:
	.asciz	"linkat"
	.size	.L.str.172, 7

	.type	.L.str.173,@object              # @.str.173
.L.str.173:
	.asciz	"symlinkat"
	.size	.L.str.173, 10

	.type	.L.str.174,@object              # @.str.174
.L.str.174:
	.asciz	"readlinkat"
	.size	.L.str.174, 11

	.type	.L.str.175,@object              # @.str.175
.L.str.175:
	.asciz	"fchmodat"
	.size	.L.str.175, 9

	.type	.L.str.176,@object              # @.str.176
.L.str.176:
	.asciz	"faccessat"
	.size	.L.str.176, 10

	.type	.L.str.177,@object              # @.str.177
.L.str.177:
	.asciz	"utimensat"
	.size	.L.str.177, 10

	.type	.L.str.178,@object              # @.str.178
.L.str.178:
	.asciz	"epoll_pwait"
	.size	.L.str.178, 12

	.type	.L.str.179,@object              # @.str.179
.L.str.179:
	.asciz	"eventfd"
	.size	.L.str.179, 8

	.type	.L.str.180,@object              # @.str.180
.L.str.180:
	.asciz	"accept4"
	.size	.L.str.180, 8

	.type	.L.str.181,@object              # @.str.181
.L.str.181:
	.asciz	"eventfd2"
	.size	.L.str.181, 9

	.type	.L.str.182,@object              # @.str.182
.L.str.182:
	.asciz	"epoll_create1"
	.size	.L.str.182, 14

	.type	.L.str.183,@object              # @.str.183
.L.str.183:
	.asciz	"dup3"
	.size	.L.str.183, 5

	.type	.L.str.184,@object              # @.str.184
.L.str.184:
	.asciz	"pipe2"
	.size	.L.str.184, 6

	.type	.L.str.185,@object              # @.str.185
.L.str.185:
	.asciz	"renameat2"
	.size	.L.str.185, 10

	.type	.L.str.186,@object              # @.str.186
.L.str.186:
	.asciz	"memfd_create"
	.size	.L.str.186, 13

	.type	.L.str.187,@object              # @.str.187
.L.str.187:
	.asciz	"clone3"
	.size	.L.str.187, 7

	.type	.L.str.188,@object              # @.str.188
.L.str.188:
	.asciz	"faccessat2"
	.size	.L.str.188, 11

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym detail
