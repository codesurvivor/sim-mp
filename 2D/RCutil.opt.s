	.file	"RCutil.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"singular matrix in lupdcmp\n"
	.text
	.p2align 4,,15
.globl lupdcmp
	.type	lupdcmp,@function
lupdcmp:
	pushl	%ebp
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	movl	68(%esp), %eax
	movl	$0, 32(%esp)
	cmpl	%eax, 32(%esp)
	movl	$0, 24(%esp)
	jge	.L40
	andl	$3, %eax
	cmpl	$1, 68(%esp)
	jg	.L147
.L118:
	movl	32(%esp), %edi
	movl	72(%esp), %eax
	movl	%edi, (%eax,%edi,4)
	incl	%edi
	movl	68(%esp), %eax
	movl	%edi, 32(%esp)
	cmpl	%eax, %edi
	jge	.L40
.L6:
	movl	32(%esp), %ebp
	movl	72(%esp), %ebx
	movl	%ebp, %esi
	movl	%ebp, (%ebx,%ebp,4)
	incl	%esi
	addl	$4, %ebp
	movl	%esi, (%ebx,%esi,4)
	incl	%esi
	movl	%esi, (%ebx,%esi,4)
	incl	%esi
	movl	%esi, (%ebx,%esi,4)
	movl	68(%esp), %eax
	movl	%ebp, 32(%esp)
	cmpl	%eax, %ebp
	jl	.L6
.L40:
	movl	$0, 28(%esp)
	movl	68(%esp), %edi
	decl	%edi
	cmpl	%edi, 28(%esp)
	movl	%edi, 20(%esp)
	jge	.L42
.L38:
	movl	%edx, 8(%esp)
	movl	28(%esp), %esi
	movl	68(%esp), %eax
	movl	%ecx, 12(%esp)
	fldl	8(%esp)
	cmpl	%eax, %esi
	movl	%esi, 32(%esp)
	jge	.L44
	movl	%eax, %edx
	movl	%esi, %ecx
	subl	%esi, %edx
	incl	%ecx
	andl	$3, %edx
	cmpl	68(%esp), %ecx
	jl	.L148
.L92:
	movl	32(%esp), %edx
	movl	64(%esp), %ebx
	movl	28(%esp), %edi
	movl	(%ebx,%edx,4), %eax
	fldl	(%eax,%edi,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L137
	fstp	%st(1)
	movl	%edx, 24(%esp)
.L101:
	incl	32(%esp)
	movl	68(%esp), %eax
	cmpl	%eax, 32(%esp)
	jge	.L44
	.p2align 4,,15
.L19:
	movl	32(%esp), %edx
	movl	64(%esp), %ebp
	movl	28(%esp), %ecx
	movl	(%ebp,%edx,4), %eax
	fldl	(%eax,%ecx,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L138
	fstp	%st(1)
	movl	%edx, 24(%esp)
.L104:
	movl	32(%esp), %edx
	movl	64(%esp), %edi
	movl	28(%esp), %esi
	incl	%edx
	movl	(%edi,%edx,4), %eax
	fldl	(%eax,%esi,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L139
	fstp	%st(1)
	movl	%edx, 24(%esp)
.L107:
	movl	32(%esp), %edx
	movl	64(%esp), %ecx
	movl	28(%esp), %ebx
	addl	$2, %edx
	movl	(%ecx,%edx,4), %eax
	fldl	(%eax,%ebx,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L140
	fstp	%st(1)
	movl	%edx, 24(%esp)
.L110:
	movl	32(%esp), %edx
	movl	64(%esp), %esi
	movl	28(%esp), %ebp
	addl	$3, %edx
	movl	(%esi,%edx,4), %eax
	fldl	(%eax,%ebp,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L141
	fstp	%st(1)
	movl	%edx, 24(%esp)
.L113:
	addl	$4, 32(%esp)
	movl	68(%esp), %edx
	cmpl	%edx, 32(%esp)
	jl	.L19
.L44:
	movl	$0x0, 4(%esp)
	fstps	(%esp)
	call	eq
	testl	%eax, %eax
	jne	.L149
.L20:
	movl	$0, 32(%esp)
	movl	28(%esp), %eax
	movl	72(%esp), %edx
	leal	(%edx,%eax,4), %esi
	movl	24(%esp), %eax
	movl	(%esi), %ebp
	leal	(%edx,%eax,4), %edi
	movl	(%edi), %eax
	movl	%eax, (%esi)
	movl	%ebp, (%edi)
	movl	68(%esp), %esi
	cmpl	%esi, 32(%esp)
	jge	.L46
	movl	64(%esp), %ecx
	movl	24(%esp), %edi
	movl	28(%esp), %eax
	movl	(%ecx,%edi,4), %ebx
	movl	(%ecx,%eax,4), %ebp
	movl	%esi, %eax
	andl	$3, %eax
	decl	%esi
	movl	%ebx, 16(%esp)
	jg	.L150
.L73:
	movl	32(%esp), %ecx
	movl	16(%esp), %eax
	incl	32(%esp)
	sall	$3, %ecx
	leal	(%ecx,%ebp), %edi
	addl	%eax, %ecx
	movl	(%ecx), %eax
	movl	(%edi), %ebx
	movl	4(%ecx), %edx
	movl	4(%edi), %esi
	movl	%eax, (%edi)
	movl	68(%esp), %eax
	cmpl	%eax, 32(%esp)
	movl	%edx, 4(%edi)
	movl	%ebx, (%ecx)
	movl	%esi, 4(%ecx)
	jge	.L46
	.p2align 4,,15
.L27:
	movl	32(%esp), %ecx
	movl	16(%esp), %edx
	sall	$3, %ecx
	leal	(%ecx,%ebp), %edi
	addl	%edx, %ecx
	movl	(%edi), %ebx
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	4(%edi), %esi
	movl	%eax, (%edi)
	movl	%edx, 4(%edi)
	movl	%ebx, (%ecx)
	movl	32(%esp), %ebx
	movl	%esi, 4(%ecx)
	movl	16(%esp), %esi
	leal	8(,%ebx,8), %ecx
	leal	(%ecx,%ebp), %edi
	addl	%esi, %ecx
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	(%edi), %ebx
	movl	4(%edi), %esi
	movl	%eax, (%edi)
	movl	16(%esp), %eax
	movl	%edx, 4(%edi)
	movl	32(%esp), %edi
	movl	%ebx, (%ecx)
	movl	%esi, 4(%ecx)
	leal	16(,%edi,8), %ecx
	leal	(%ecx,%ebp), %edi
	addl	%eax, %ecx
	movl	4(%ecx), %edx
	movl	(%ecx), %eax
	movl	(%edi), %ebx
	movl	4(%edi), %esi
	movl	%edx, 4(%edi)
	movl	32(%esp), %edx
	movl	%eax, (%edi)
	movl	16(%esp), %eax
	movl	%ebx, (%ecx)
	movl	%esi, 4(%ecx)
	leal	24(,%edx,8), %ecx
	leal	(%ecx,%ebp), %edi
	addl	%eax, %ecx
	movl	(%ecx), %eax
	movl	(%edi), %ebx
	movl	4(%edi), %esi
	movl	4(%ecx), %edx
	movl	%eax, (%edi)
	movl	68(%esp), %eax
	addl	$4, 32(%esp)
	movl	%edx, 4(%edi)
	movl	%ebx, (%ecx)
	cmpl	%eax, 32(%esp)
	movl	%esi, 4(%ecx)
	jl	.L27
.L46:
	movl	28(%esp), %ebp
	incl	%ebp
	movl	%ebp, %ebx
	cmpl	68(%esp), %ebp
	movl	%ebp, 32(%esp)
	jge	.L48
	movl	28(%esp), %edi
	movl	64(%esp), %eax
	movl	(%eax,%edi,4), %ebp
	.p2align 4,,15
.L37:
	cmpl	68(%esp), %ebx
	movl	32(%esp), %eax
	movl	64(%esp), %esi
	movl	28(%esp), %ecx
	movl	(%esi,%eax,4), %edx
	fldl	(%ebp,%ecx,8)
	fdivrl	(%edx,%ecx,8)
	fstl	(%edx,%ecx,8)
	movl	%ebx, %ecx
	jge	.L142
	movl	68(%esp), %eax
	leal	1(%ebx), %edi
	subl	%ebx, %eax
	andl	$3, %eax
	cmpl	68(%esp), %edi
	jl	.L151
	fstp	%st(0)
.L54:
	fldl	(%ebp,%ecx,8)
	movl	28(%esp), %eax
	fmull	(%edx,%eax,8)
	fsubrl	(%edx,%ecx,8)
	fstpl	(%edx,%ecx,8)
	incl	%ecx
	cmpl	68(%esp), %ecx
	jge	.L50
	.p2align 4,,15
.L36:
	fldl	(%ebp,%ecx,8)
	movl	28(%esp), %eax
	leal	1(%ecx), %edi
	movl	28(%esp), %esi
	fmull	(%edx,%eax,8)
	fsubrl	(%edx,%ecx,8)
	fstpl	(%edx,%ecx,8)
	fldl	(%ebp,%edi,8)
	fmull	(%edx,%esi,8)
	fsubrl	(%edx,%edi,8)
	fstpl	(%edx,%edi,8)
	leal	2(%ecx), %edi
	fldl	(%ebp,%edi,8)
	fmull	(%edx,%esi,8)
	fsubrl	(%edx,%edi,8)
	fstpl	(%edx,%edi,8)
	leal	3(%ecx), %edi
	addl	$4, %ecx
	fldl	(%ebp,%edi,8)
	fmull	(%edx,%esi,8)
	cmpl	68(%esp), %ecx
	fsubrl	(%edx,%edi,8)
	fstpl	(%edx,%edi,8)
	jl	.L36
.L50:
	incl	32(%esp)
	movl	68(%esp), %eax
	cmpl	%eax, 32(%esp)
	jl	.L37
.L48:
	movl	%ebx, 28(%esp)
	movl	20(%esp), %ebp
	cmpl	%ebp, %ebx
	jge	.L42
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	jmp	.L38
.L42:
	addl	$44, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L151:
	testl	%eax, %eax
	je	.L144
	cmpl	$1, %eax
	jle	.L145
	cmpl	$2, %eax
	jle	.L146
	fmull	(%ebp,%ebx,8)
	movl	%edi, %ecx
	fsubrl	(%edx,%ebx,8)
	fstpl	(%edx,%ebx,8)
.L55:
	fldl	(%ebp,%ecx,8)
	movl	28(%esp), %eax
	fmull	(%edx,%eax,8)
	fsubrl	(%edx,%ecx,8)
	fstpl	(%edx,%ecx,8)
	incl	%ecx
	jmp	.L54
.L146:
	fstp	%st(0)
	jmp	.L55
.L145:
	fstp	%st(0)
	jmp	.L54
.L144:
	fstp	%st(0)
	jmp	.L36
	.p2align 4,,7
.L142:
	fstp	%st(0)
	jmp	.L50
.L150:
	testl	%eax, %eax
	je	.L27
	cmpl	$1, %eax
	jle	.L73
	cmpl	$2, %eax
	jle	.L74
	movl	$1, 32(%esp)
	movl	16(%esp), %edi
	movl	(%ebp), %ebx
	movl	4(%ebp), %esi
	movl	(%edi), %eax
	movl	4(%edi), %ecx
	movl	%eax, (%ebp)
	movl	%ecx, 4(%ebp)
	movl	%ebx, (%edi)
	movl	%esi, 4(%edi)
.L74:
	movl	32(%esp), %ecx
	movl	16(%esp), %eax
	incl	32(%esp)
	sall	$3, %ecx
	leal	(%ecx,%ebp), %edi
	addl	%eax, %ecx
	movl	(%edi), %ebx
	movl	4(%edi), %esi
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	%eax, (%edi)
	movl	%edx, 4(%edi)
	movl	%ebx, (%ecx)
	movl	%esi, 4(%ecx)
	jmp	.L73
.L149:
	movl	$.LC1, (%esp)
	call	fatal
	jmp	.L20
.L141:
	fstp	%st(0)
	jmp	.L113
.L140:
	fstp	%st(0)
	jmp	.L110
.L139:
	fstp	%st(0)
	jmp	.L107
.L138:
	fstp	%st(0)
	jmp	.L104
.L137:
	fstp	%st(0)
	jmp	.L101
.L148:
	testl	%edx, %edx
	je	.L19
	cmpl	$1, %edx
	jle	.L92
	cmpl	$2, %edx
	jle	.L93
	movl	64(%esp), %ebp
	movl	(%ebp,%esi,4), %eax
	fldl	(%eax,%esi,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L135
	fstp	%st(1)
	movl	%esi, 24(%esp)
.L95:
	incl	32(%esp)
.L93:
	movl	32(%esp), %ecx
	movl	64(%esp), %esi
	movl	28(%esp), %edx
	movl	(%esi,%ecx,4), %eax
	fldl	(%eax,%edx,8)
	fabs
	fucomi	%st(1), %st
	jbe	.L136
	fstp	%st(1)
	movl	%ecx, 24(%esp)
.L98:
	incl	32(%esp)
	jmp	.L92
.L136:
	fstp	%st(0)
	jmp	.L98
.L135:
	fstp	%st(0)
	jmp	.L95
.L147:
	testl	%eax, %eax
	je	.L6
	cmpl	$1, %eax
	jle	.L118
	cmpl	$2, %eax
	jle	.L119
	movl	$1, 32(%esp)
	movl	72(%esp), %eax
	movl	$0, (%eax)
.L119:
	movl	32(%esp), %ebx
	movl	72(%esp), %eax
	movl	%ebx, (%eax,%ebx,4)
	incl	%ebx
	movl	%ebx, 32(%esp)
	jmp	.L118
.Lfe1:
	.size	lupdcmp,.Lfe1-lupdcmp
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC3:
	.long	0
	.long	1071644672
	.align 8
.LC4:
	.long	0
	.long	1075314688
	.text
	.p2align 4,,15
.globl rk4
	.type	rk4,@function
rk4:
	pushl	%ebp
	xorl	%ebp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$76, %esp
	movl	96(%esp), %eax
	fldl	112(%esp)
	movl	108(%esp), %edi
	movl	100(%esp), %esi
	movl	%eax, 68(%esp)
	movl	104(%esp), %eax
	fstpl	56(%esp)
	movl	%edi, (%esp)
	movl	%eax, 64(%esp)
	movl	120(%esp), %eax
	movl	%eax, 52(%esp)
	call	vector
	movl	%eax, 48(%esp)
	movl	%edi, (%esp)
	call	vector
	movl	%eax, 44(%esp)
	movl	%edi, (%esp)
	call	vector
	movl	%eax, 40(%esp)
	movl	%edi, (%esp)
	call	vector
	movl	%eax, 36(%esp)
	movl	%edi, (%esp)
	call	vector
	cmpl	%edi, %ebp
	movl	%eax, 32(%esp)
	jge	.L215
	movl	48(%esp), %eax
	.p2align 4,,15
.L162:
	movl	$0, (%eax)
	xorl	%edx, %edx
	cmpl	%edi, %edx
	movl	$0, 4(%eax)
	jge	.L201
	movl	68(%esp), %ecx
	movl	(%ecx,%ebp,4), %ebx
	movl	%edi, %ecx
	andl	$3, %ecx
	cmpl	$1, %edi
	jg	.L314
.L294:
	fldl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	cmpl	%edi, %edx
	faddl	(%eax)
	fstpl	(%eax)
	jge	.L201
	.p2align 4,,15
.L161:
	fldl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	faddl	(%eax)
	fstl	(%eax)
	fldl	8(%ebx,%edx,8)
	fmull	8(%esi,%edx,8)
	faddp	%st, %st(1)
	fstl	(%eax)
	fldl	16(%ebx,%edx,8)
	fmull	16(%esi,%edx,8)
	faddp	%st, %st(1)
	fstl	(%eax)
	fldl	24(%ebx,%edx,8)
	fmull	24(%esi,%edx,8)
	addl	$4, %edx
	cmpl	%edi, %edx
	faddp	%st, %st(1)
	fstpl	(%eax)
	jl	.L161
.L201:
	fldl	(%eax)
	addl	$8, %eax
	movl	64(%esp), %ecx
	movl	44(%esp), %edx
	fsubrl	(%ecx,%ebp,8)
	fmull	56(%esp)
	fstpl	(%edx,%ebp,8)
	incl	%ebp
	cmpl	%edi, %ebp
	jl	.L162
	xorl	%ebp, %ebp
	cmpl	%edi, %ebp
	jge	.L215
	fldl	.LC3
	movl	48(%esp), %eax
	movl	%eax, 20(%esp)
	.p2align 4,,15
.L172:
	movl	20(%esp), %eax
	xorl	%edx, %edx
	cmpl	%edi, %edx
	movl	$0, (%eax)
	movl	$0, 4(%eax)
	jge	.L205
	movl	68(%esp), %eax
	fld	%st(0)
	movl	(%eax,%ebp,4), %ebx
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L315
.L275:
	movl	44(%esp), %eax
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	fmul	%st(1), %st
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	cmpl	%edi, %edx
	faddl	(%eax)
	fstpl	(%eax)
	jge	.L311
	.p2align 4,,15
.L171:
	movl	44(%esp), %eax
	leal	1(%edx), %ecx
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	fmul	%st(1), %st
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	faddl	(%eax)
	fstl	(%eax)
	movl	44(%esp), %eax
	fldl	(%eax,%ecx,8)
	movl	20(%esp), %eax
	fmul	%st(2), %st
	faddl	(%esi,%ecx,8)
	fmull	(%ebx,%ecx,8)
	movl	44(%esp), %ecx
	faddp	%st, %st(1)
	fstl	(%eax)
	leal	2(%edx), %eax
	fldl	(%ecx,%eax,8)
	leal	3(%edx), %ecx
	addl	$4, %edx
	cmpl	%edi, %edx
	fmul	%st(2), %st
	faddl	(%esi,%eax,8)
	fmull	(%ebx,%eax,8)
	movl	20(%esp), %eax
	faddp	%st, %st(1)
	fstl	(%eax)
	movl	44(%esp), %eax
	fldl	(%eax,%ecx,8)
	movl	20(%esp), %eax
	fmul	%st(2), %st
	faddl	(%esi,%ecx,8)
	fmull	(%ebx,%ecx,8)
	faddp	%st, %st(1)
	fstpl	(%eax)
	jl	.L171
	fstp	%st(0)
.L205:
	movl	20(%esp), %eax
	movl	64(%esp), %ecx
	movl	40(%esp), %ebx
	fldl	(%eax)
	addl	$8, %eax
	fsubrl	(%ecx,%ebp,8)
	movl	%eax, 20(%esp)
	fmull	56(%esp)
	fstpl	(%ebx,%ebp,8)
	incl	%ebp
	cmpl	%edi, %ebp
	jl	.L172
	fstp	%st(0)
	xorl	%ebp, %ebp
	cmpl	%edi, %ebp
	jge	.L215
	fldl	.LC3
	movl	48(%esp), %eax
	movl	%eax, 20(%esp)
	.p2align 4,,15
.L182:
	movl	20(%esp), %eax
	xorl	%edx, %edx
	cmpl	%edi, %edx
	movl	$0, (%eax)
	movl	$0, 4(%eax)
	jge	.L209
	movl	68(%esp), %eax
	fld	%st(0)
	movl	(%eax,%ebp,4), %ebx
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L316
.L256:
	movl	40(%esp), %eax
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	fmul	%st(1), %st
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	cmpl	%edi, %edx
	faddl	(%eax)
	fstpl	(%eax)
	jge	.L312
	.p2align 4,,15
.L181:
	movl	40(%esp), %eax
	leal	1(%edx), %ecx
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	fmul	%st(1), %st
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	faddl	(%eax)
	fstl	(%eax)
	movl	40(%esp), %eax
	fldl	(%eax,%ecx,8)
	movl	20(%esp), %eax
	fmul	%st(2), %st
	faddl	(%esi,%ecx,8)
	fmull	(%ebx,%ecx,8)
	movl	40(%esp), %ecx
	faddp	%st, %st(1)
	fstl	(%eax)
	leal	2(%edx), %eax
	fldl	(%ecx,%eax,8)
	leal	3(%edx), %ecx
	addl	$4, %edx
	cmpl	%edi, %edx
	fmul	%st(2), %st
	faddl	(%esi,%eax,8)
	fmull	(%ebx,%eax,8)
	movl	20(%esp), %eax
	faddp	%st, %st(1)
	fstl	(%eax)
	movl	40(%esp), %eax
	fldl	(%eax,%ecx,8)
	movl	20(%esp), %eax
	fmul	%st(2), %st
	faddl	(%esi,%ecx,8)
	fmull	(%ebx,%ecx,8)
	faddp	%st, %st(1)
	fstpl	(%eax)
	jl	.L181
	fstp	%st(0)
.L209:
	movl	20(%esp), %eax
	movl	64(%esp), %ebx
	movl	36(%esp), %edx
	fldl	(%eax)
	addl	$8, %eax
	fsubrl	(%ebx,%ebp,8)
	movl	%eax, 20(%esp)
	fmull	56(%esp)
	fstpl	(%edx,%ebp,8)
	incl	%ebp
	cmpl	%edi, %ebp
	jl	.L182
	fstp	%st(0)
	xorl	%ebp, %ebp
	cmpl	%edi, %ebp
	jge	.L215
	movl	48(%esp), %eax
	movl	%eax, 20(%esp)
.L192:
	movl	20(%esp), %ecx
	xorl	%edx, %edx
	cmpl	%edi, %edx
	movl	$0, (%ecx)
	movl	$0, 4(%ecx)
	jge	.L213
	movl	68(%esp), %eax
	movl	(%eax,%ebp,4), %ebx
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L317
.L237:
	movl	36(%esp), %eax
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	cmpl	%edi, %edx
	faddl	(%eax)
	fstpl	(%eax)
	jge	.L213
	.p2align 4,,15
.L191:
	movl	36(%esp), %eax
	leal	1(%edx), %ecx
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	faddl	(%eax)
	fstl	(%eax)
	movl	36(%esp), %eax
	fldl	(%eax,%ecx,8)
	movl	20(%esp), %eax
	faddl	(%esi,%ecx,8)
	fmull	(%ebx,%ecx,8)
	movl	36(%esp), %ecx
	faddp	%st, %st(1)
	fstl	(%eax)
	leal	2(%edx), %eax
	fldl	(%ecx,%eax,8)
	leal	3(%edx), %ecx
	addl	$4, %edx
	faddl	(%esi,%eax,8)
	cmpl	%edi, %edx
	fmull	(%ebx,%eax,8)
	movl	20(%esp), %eax
	faddp	%st, %st(1)
	fstl	(%eax)
	movl	36(%esp), %eax
	fldl	(%eax,%ecx,8)
	movl	20(%esp), %eax
	faddl	(%esi,%ecx,8)
	fmull	(%ebx,%ecx,8)
	faddp	%st, %st(1)
	fstpl	(%eax)
	jl	.L191
.L213:
	movl	20(%esp), %eax
	movl	64(%esp), %ebx
	movl	32(%esp), %edx
	fldl	(%eax)
	addl	$8, %eax
	fsubrl	(%ebx,%ebp,8)
	movl	%eax, 20(%esp)
	fmull	56(%esp)
	fstpl	(%edx,%ebp,8)
	incl	%ebp
	cmpl	%edi, %ebp
	jl	.L192
	xorl	%ebp, %ebp
	cmpl	%edi, %ebp
	jge	.L215
	fldl	.LC4
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L318
.L218:
	movl	40(%esp), %eax
	movl	44(%esp), %ebx
	movl	32(%esp), %ecx
	fldl	(%eax,%ebp,8)
	movl	36(%esp), %eax
	movl	52(%esp), %edx
	fadd	%st(0), %st
	fldl	(%eax,%ebp,8)
	fxch	%st(1)
	faddl	(%ebx,%ebp,8)
	fxch	%st(1)
	fadd	%st(0), %st
	faddp	%st, %st(1)
	faddl	(%ecx,%ebp,8)
	fdiv	%st(1), %st
	faddl	(%esi,%ebp,8)
	fstpl	(%edx,%ebp,8)
	incl	%ebp
	cmpl	%edi, %ebp
	jge	.L313
.L197:
	movl	40(%esp), %eax
	movl	36(%esp), %edx
	movl	44(%esp), %ecx
	fldl	(%eax,%ebp,8)
	movl	32(%esp), %eax
	movl	52(%esp), %ebx
	fldl	(%edx,%ebp,8)
	fxch	%st(1)
	fadd	%st(0), %st
	movl	36(%esp), %edx
	faddl	(%ecx,%ebp,8)
	fxch	%st(1)
	fadd	%st(0), %st
	movl	44(%esp), %ecx
	faddp	%st, %st(1)
	faddl	(%eax,%ebp,8)
	movl	40(%esp), %eax
	fdiv	%st(1), %st
	faddl	(%esi,%ebp,8)
	fstpl	(%ebx,%ebp,8)
	leal	1(%ebp), %ebx
	fldl	(%eax,%ebx,8)
	movl	32(%esp), %eax
	fldl	(%edx,%ebx,8)
	fxch	%st(1)
	fadd	%st(0), %st
	movl	52(%esp), %edx
	faddl	(%ecx,%ebx,8)
	fxch	%st(1)
	fadd	%st(0), %st
	movl	40(%esp), %ecx
	faddp	%st, %st(1)
	faddl	(%eax,%ebx,8)
	leal	2(%ebp), %eax
	fdiv	%st(1), %st
	faddl	(%esi,%ebx,8)
	fstpl	(%edx,%ebx,8)
	movl	32(%esp), %edx
	movl	44(%esp), %ebx
	fldl	(%ecx,%eax,8)
	movl	36(%esp), %ecx
	fadd	%st(0), %st
	fldl	(%ecx,%eax,8)
	fxch	%st(1)
	movl	44(%esp), %ecx
	faddl	(%ebx,%eax,8)
	fxch	%st(1)
	fadd	%st(0), %st
	movl	52(%esp), %ebx
	faddp	%st, %st(1)
	faddl	(%edx,%eax,8)
	movl	40(%esp), %edx
	fdiv	%st(1), %st
	faddl	(%esi,%eax,8)
	fstpl	(%ebx,%eax,8)
	movl	36(%esp), %eax
	leal	3(%ebp), %ebx
	addl	$4, %ebp
	cmpl	%edi, %ebp
	fldl	(%edx,%ebx,8)
	movl	52(%esp), %edx
	fldl	(%eax,%ebx,8)
	fxch	%st(1)
	fadd	%st(0), %st
	fxch	%st(1)
	fadd	%st(0), %st
	fxch	%st(1)
	faddl	(%ecx,%ebx,8)
	movl	32(%esp), %ecx
	faddp	%st, %st(1)
	faddl	(%ecx,%ebx,8)
	fdiv	%st(1), %st
	faddl	(%esi,%ebx,8)
	fstpl	(%edx,%ebx,8)
	jl	.L197
	fstp	%st(0)
.L215:
	movl	48(%esp), %eax
	movl	%eax, (%esp)
	call	free_vector
	movl	44(%esp), %ebp
	movl	%ebp, (%esp)
	call	free_vector
	movl	40(%esp), %edi
	movl	%edi, (%esp)
	call	free_vector
	movl	36(%esp), %eax
	movl	%eax, (%esp)
	call	free_vector
	movl	32(%esp), %esi
	movl	%esi, 96(%esp)
	addl	$76, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	jmp	free_vector
.L313:
	fstp	%st(0)
	jmp	.L215
.L318:
	testl	%eax, %eax
	je	.L197
	cmpl	$1, %eax
	jle	.L218
	cmpl	$2, %eax
	jle	.L219
	movl	40(%esp), %eax
	movl	44(%esp), %ecx
	movl	52(%esp), %ebp
	fldl	(%eax)
	movl	36(%esp), %eax
	fadd	%st(0), %st
	fldl	(%eax)
	fxch	%st(1)
	movl	32(%esp), %eax
	faddl	(%ecx)
	fxch	%st(1)
	fadd	%st(0), %st
	faddp	%st, %st(1)
	faddl	(%eax)
	fdiv	%st(1), %st
	faddl	(%esi)
	fstpl	(%ebp)
	movl	$1, %ebp
.L219:
	movl	40(%esp), %eax
	movl	36(%esp), %ebx
	movl	32(%esp), %edx
	fldl	(%eax,%ebp,8)
	movl	44(%esp), %eax
	fldl	(%ebx,%ebp,8)
	fxch	%st(1)
	fadd	%st(0), %st
	fxch	%st(1)
	fadd	%st(0), %st
	fxch	%st(1)
	faddl	(%eax,%ebp,8)
	movl	52(%esp), %eax
	faddp	%st, %st(1)
	faddl	(%edx,%ebp,8)
	fdiv	%st(1), %st
	faddl	(%esi,%ebp,8)
	fstpl	(%eax,%ebp,8)
	incl	%ebp
	jmp	.L218
	.p2align 4,,7
.L317:
	testl	%eax, %eax
	je	.L191
	cmpl	$1, %eax
	jle	.L237
	cmpl	$2, %eax
	jle	.L238
	movl	36(%esp), %eax
	movl	$1, %edx
	movl	20(%esp), %ecx
	fldl	(%eax)
	faddl	(%esi)
	fmull	(%ebx)
	faddl	(%ecx)
	fstpl	(%ecx)
.L238:
	movl	36(%esp), %eax
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	faddl	(%eax)
	fstpl	(%eax)
	jmp	.L237
	.p2align 4,,7
.L312:
	fstp	%st(0)
	jmp	.L209
.L316:
	testl	%eax, %eax
	je	.L181
	cmpl	$1, %eax
	jle	.L256
	cmpl	$2, %eax
	jle	.L257
	movl	40(%esp), %eax
	movl	$1, %edx
	movl	20(%esp), %ecx
	fldl	(%eax)
	fmul	%st(2), %st
	faddl	(%esi)
	fmull	(%ebx)
	faddl	(%ecx)
	fstpl	(%ecx)
.L257:
	movl	40(%esp), %eax
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	fmul	%st(1), %st
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	faddl	(%eax)
	fstpl	(%eax)
	jmp	.L256
	.p2align 4,,7
.L311:
	fstp	%st(0)
	jmp	.L205
.L315:
	testl	%eax, %eax
	je	.L171
	cmpl	$1, %eax
	jle	.L275
	cmpl	$2, %eax
	jle	.L276
	movl	44(%esp), %eax
	movl	$1, %edx
	movl	20(%esp), %ecx
	fldl	(%eax)
	fmul	%st(2), %st
	faddl	(%esi)
	fmull	(%ebx)
	faddl	(%ecx)
	fstpl	(%ecx)
.L276:
	movl	44(%esp), %eax
	fldl	(%eax,%edx,8)
	movl	20(%esp), %eax
	fmul	%st(1), %st
	faddl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	faddl	(%eax)
	fstpl	(%eax)
	jmp	.L275
.L314:
	testl	%ecx, %ecx
	je	.L161
	cmpl	$1, %ecx
	jle	.L294
	cmpl	$2, %ecx
	jle	.L295
	fldl	(%esi)
	movl	$1, %edx
	fmull	(%ebx)
	faddl	(%eax)
	fstpl	(%eax)
.L295:
	fldl	(%esi,%edx,8)
	fmull	(%ebx,%edx,8)
	incl	%edx
	faddl	(%eax)
	fstpl	(%eax)
	jmp	.L294
.Lfe2:
	.size	rk4,.Lfe2-rk4
	.section	.rodata.str1.32,"aMS",@progbits,1
	.align 32
.LC6:
	.string	"allocation failure in vector()"
	.text
	.p2align 4,,15
.globl vector
	.type	vector,@function
vector:
	subl	$12, %esp
	movl	16(%esp), %eax
	movl	%ebx, 8(%esp)
	movl	$8, 4(%esp)
	movl	%eax, (%esp)
	call	calloc
	testl	%eax, %eax
	movl	%eax, %ebx
	je	.L321
.L320:
	movl	%ebx, %eax
	movl	8(%esp), %ebx
	addl	$12, %esp
	ret
.L321:
	movl	$.LC6, (%esp)
	call	fatal
	jmp	.L320
.Lfe3:
	.size	vector,.Lfe3-vector
	.p2align 4,,15
.globl free_vector
	.type	free_vector,@function
free_vector:
	jmp	free
.Lfe4:
	.size	free_vector,.Lfe4-free_vector
	.section	.rodata.cst8
	.align 8
.LC41:
	.long	0
	.long	1071644672
	.align 8
.LC42:
	.long	1413754136
	.long	1074340347
	.text
	.p2align 4,,15
.globl getr
	.type	getr,@function
getr:
	pushl	%esi
	pushl	%ebx
	subl	$164, %esp
	fldl	176(%esp)
	fldl	184(%esp)
	fldl	192(%esp)
	fldl	208(%esp)
	fld	%st(2)
	fld	%st(2)
	fxch	%st(5)
	fstl	152(%esp)
	fmulp	%st, %st(4)
	fmul	%st(1), %st
	fxch	%st(4)
	fmull	.LC41
	fxch	%st(3)
	fmul	%st(1), %st
	fldl	.LC42
	fxch	%st(4)
	fdivp	%st, %st(1)
	fldl	200(%esp)
	fmulp	%st, %st(2)
	fxch	%st(1)
	fdiv	%st(3), %st
#APP
	fsqrt
#NO_APP
	fstpl	136(%esp)
	fxch	%st(3)
	fdiv	%st(2), %st
#APP
	fsqrt
#NO_APP
	fldl	136(%esp)
	fld	%st(1)
	fxch	%st(1)
	fdivp	%st, %st(2)
	fld	%st(3)
	fxch	%st(2)
	fstpl	144(%esp)
	fxch	%st(1)
#APP
	fsqrt
#NO_APP
	fxch	%st(3)
	fstl	96(%esp)
	fld1
	fxch	%st(4)
	fmull	144(%esp)
	fld	%st(4)
	fstl	80(%esp)
	fxch	%st(5)
	fdivp	%st, %st(1)
	fadd	%st(1), %st
	fldl	152(%esp)
	fld	%st(1)
	fxch	%st(1)
	fmulp	%st, %st(3)
	fxch	%st(1)
	fstpl	32(%esp)
	fxch	%st(1)
	fmul	%st(2), %st
	fxch	%st(2)
	fadd	%st(0), %st
	fdivrp	%st, %st(3)
	fxch	%st(1)
	fmulp	%st, %st(4)
	fmul	%st(1), %st
	fxch	%st(1)
	fstpl	48(%esp)
	fxch	%st(2)
	fdivrp	%st, %st(1)
	fxch	%st(1)
	fstpl	24(%esp)
	movl	24(%esp), %ebx
	fstpl	112(%esp)
	movl	28(%esp), %esi
	movl	%ebx, (%esp)
	movl	%esi, 4(%esp)
	call	tanh
	fldl	32(%esp)
	fldl	112(%esp)
	movl	%ebx, (%esp)
	movl	%esi, 4(%esp)
	fdivrp	%st, %st(1)
	fadd	%st, %st(1)
	fstpl	112(%esp)
	fstpl	64(%esp)
	call	tanh
	fldl	112(%esp)
	fldl	80(%esp)
	fldl	64(%esp)
	fxch	%st(2)
	fmulp	%st, %st(3)
	fldl	48(%esp)
	fmull	144(%esp)
	fxch	%st(3)
	fadd	%st(1), %st
	fxch	%st(1)
	fsubl	144(%esp)
	fldl	96(%esp)
	fxch	%st(3)
	fdivp	%st, %st(2)
	fabs
	fld	%st(2)
	fxch	%st(1)
	fmulp	%st, %st(2)
	fxch	%st(3)
	faddp	%st, %st(1)
	fxch	%st(2)
#APP
	fsqrt
#NO_APP
	fdivrp	%st, %st(2)
#APP
	fsqrt
#NO_APP
	fmull	152(%esp)
	fmull	136(%esp)
	addl	$164, %esp
	popl	%ebx
	popl	%esi
	fdivrp	%st, %st(1)
	ret
.Lfe5:
	.size	getr,.Lfe5-getr
	.p2align 4,,15
.globl iswap
	.type	iswap,@function
iswap:
	pushl	%ebx
	movl	8(%esp), %edx
	movl	12(%esp), %ecx
	movl	(%edx), %ebx
	movl	(%ecx), %eax
	movl	%eax, (%edx)
	movl	%ebx, (%ecx)
	popl	%ebx
	ret
.Lfe6:
	.size	iswap,.Lfe6-iswap
	.p2align 4,,15
.globl dswap
	.type	dswap,@function
dswap:
	subl	$12, %esp
	movl	%esi, 4(%esp)
	movl	16(%esp), %esi
	movl	%edi, 8(%esp)
	movl	20(%esp), %edi
	movl	%ebx, (%esp)
	movl	4(%esi), %ebx
	movl	(%esi), %ecx
	movl	(%edi), %eax
	movl	4(%edi), %edx
	movl	%eax, (%esi)
	movl	%edx, 4(%esi)
	movl	%ecx, (%edi)
	movl	%ebx, 4(%edi)
	movl	(%esp), %ebx
	movl	4(%esp), %esi
	movl	8(%esp), %edi
	addl	$12, %esp
	ret
.Lfe7:
	.size	dswap,.Lfe7-dswap
	.p2align 4,,15
.globl lusolve
	.type	lusolve,@function
lusolve:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	xorl	%ebx, %ebx
	subl	$28, %esp
	movl	48(%esp), %eax
	movl	52(%esp), %esi
	movl	56(%esp), %ecx
	movl	%eax, 24(%esp)
	movl	60(%esp), %eax
	movl	%esi, (%esp)
	movl	%ecx, 20(%esp)
	movl	%eax, 16(%esp)
	movl	64(%esp), %eax
	movl	%eax, 12(%esp)
	call	vector
	cmpl	%esi, %ebx
	movl	%eax, %edi
	jge	.L523
	movl	24(%esp), %ebp
	fldz
	.p2align 4,,15
.L509:
	xorl	%ecx, %ecx
	cmpl	%ebx, %ecx
	fld	%st(0)
	jge	.L525
	movl	%ebx, %eax
	andl	$3, %eax
	cmpl	$1, %ebx
	jg	.L599
.L558:
	fldl	(%edi,%ecx,8)
	cmpl	%ecx, %ebx
	jle	.L570
	movl	(%ebp), %eax
	fmull	(%eax,%ecx,8)
.L591:
	incl	%ecx
	cmpl	%ebx, %ecx
	faddp	%st, %st(1)
	jge	.L525
	.p2align 4,,15
.L508:
	fldl	(%edi,%ecx,8)
	cmpl	%ecx, %ebx
	jle	.L574
	movl	(%ebp), %eax
	fmull	(%eax,%ecx,8)
.L592:
	leal	1(%ecx), %edx
	cmpl	%edx, %ebx
	faddp	%st, %st(1)
	fldl	(%edi,%edx,8)
	jle	.L578
	movl	(%ebp), %eax
	fmull	(%eax,%edx,8)
.L593:
	leal	2(%ecx), %edx
	cmpl	%edx, %ebx
	faddp	%st, %st(1)
	fldl	(%edi,%edx,8)
	jle	.L582
	movl	(%ebp), %eax
	fmull	(%eax,%edx,8)
.L594:
	leal	3(%ecx), %edx
	cmpl	%edx, %ebx
	faddp	%st, %st(1)
	fldl	(%edi,%edx,8)
	jle	.L586
	movl	(%ebp), %eax
	fmull	(%eax,%edx,8)
.L595:
	addl	$4, %ecx
	cmpl	%ebx, %ecx
	faddp	%st, %st(1)
	jl	.L508
.L525:
	movl	20(%esp), %ecx
	addl	$4, %ebp
	movl	16(%esp), %edx
	movl	(%ecx,%ebx,4), %eax
	fldl	(%edx,%eax,8)
	fsubp	%st, %st(1)
	fstpl	(%edi,%ebx,8)
	incl	%ebx
	cmpl	%esi, %ebx
	jl	.L509
	fstp	%st(0)
.L523:
	movl	%esi, %ebx
	decl	%ebx
	js	.L527
	movl	%ebx, %eax
	movl	%esi, %edx
	notl	%eax
	andl	$1, %eax
	subl	$2, %edx
	fldz
	jns	.L600
.L534:
	fldl	(%edi,%ebx,8)
	movl	24(%esp), %eax
	movl	12(%esp), %ebp
	fsub	%st(1), %st
	movl	(%eax,%ebx,4), %ecx
	fdivl	(%ecx,%ebx,8)
	fstpl	(%ebp,%ebx,8)
	decl	%ebx
	js	.L598
	.p2align 4,,15
.L521:
	leal	1(%ebx), %ecx
	cmpl	%esi, %ecx
	fld	%st(0)
	jge	.L547
	movl	24(%esp), %eax
	movl	(%eax,%ebx,4), %edx
	.p2align 4,,15
.L544:
	movl	12(%esp), %eax
	cmpl	%ecx, %ebx
	fldl	(%eax,%ecx,8)
	jg	.L543
	fmull	(%edx,%ecx,8)
.L596:
	incl	%ecx
	cmpl	%esi, %ecx
	faddp	%st, %st(1)
	jl	.L544
.L546:
	fldl	(%edi,%ebx,8)
	cmpl	%esi, %ebx
	movl	%ebx, %ecx
	leal	-1(%ebx), %eax
	fsubp	%st, %st(1)
	fld	%st(1)
	fxch	%st(1)
	fdivl	(%edx,%ebx,8)
	movl	12(%esp), %edx
	fstpl	(%edx,%ebx,8)
	jge	.L554
	movl	24(%esp), %ebp
	movl	(%ebp,%eax,4), %edx
	.p2align 4,,15
.L551:
	movl	12(%esp), %ebp
	cmpl	%ecx, %eax
	fldl	(%ebp,%ecx,8)
	jg	.L550
	fmull	(%edx,%ecx,8)
.L597:
	incl	%ecx
	cmpl	%esi, %ecx
	faddp	%st, %st(1)
	jl	.L551
.L553:
	fldl	(%edi,%eax,8)
	subl	$2, %ebx
	movl	12(%esp), %ecx
	fsubp	%st, %st(1)
	fdivl	(%edx,%eax,8)
	fstpl	(%ecx,%eax,8)
	jns	.L521
	fstp	%st(0)
.L527:
	movl	%edi, 48(%esp)
	addl	$28, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	jmp	free_vector
	.p2align 4,,7
.L550:
	fmul	%st(2), %st
	jmp	.L597
.L554:
	movl	24(%esp), %ebp
	movl	(%ebp,%eax,4), %edx
	jmp	.L553
	.p2align 4,,7
.L543:
	fmul	%st(2), %st
	jmp	.L596
.L547:
	movl	24(%esp), %ebp
	movl	(%ebp,%ebx,4), %edx
	jmp	.L546
.L598:
	fstp	%st(0)
	jmp	.L527
.L600:
	testl	%eax, %eax
	je	.L521
	jmp	.L534
	.p2align 4,,7
.L586:
	fmul	%st(2), %st
	jmp	.L595
	.p2align 4,,7
.L582:
	fmul	%st(2), %st
	jmp	.L594
	.p2align 4,,7
.L578:
	fmul	%st(2), %st
	jmp	.L593
	.p2align 4,,7
.L574:
	fmul	%st(2), %st
	jmp	.L592
.L570:
	fmul	%st(2), %st
	jmp	.L591
.L599:
	testl	%eax, %eax
	je	.L508
	cmpl	$1, %eax
	jle	.L558
	cmpl	$2, %eax
	jle	.L559
	fldl	(%edi)
	cmpl	$0, %ebx
	jle	.L562
	movl	(%ebp), %eax
	fmull	(%eax)
.L589:
	faddp	%st, %st(1)
	incl	%ecx
.L559:
	fldl	(%edi,%ecx,8)
	cmpl	%ecx, %ebx
	jle	.L566
	movl	(%ebp), %eax
	fmull	(%eax,%ecx,8)
.L590:
	faddp	%st, %st(1)
	incl	%ecx
	jmp	.L558
.L566:
	fmul	%st(2), %st
	jmp	.L590
.L562:
	fmul	%st(2), %st
	jmp	.L589
.Lfe8:
	.size	lusolve,.Lfe8-lusolve
	.section	.rodata.str1.1
.LC47:
	.string	"%.5f\t"
	.text
	.p2align 4,,15
.globl dump_vector
	.type	dump_vector,@function
dump_vector:
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	xorl	%ebx, %ebx
	subl	$16, %esp
	movl	36(%esp), %edi
	movl	32(%esp), %esi
	cmpl	%edi, %ebx
	jge	.L608
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L628
.L611:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	incl	%ebx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jge	.L608
	.p2align 4,,15
.L606:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	movl	12(%esi,%ebx,8), %ecx
	movl	8(%esi,%ebx,8), %eax
	movl	$.LC47, 4(%esp)
	movl	%ecx, 12(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	20(%esi,%ebx,8), %ecx
	movl	16(%esi,%ebx,8), %eax
	movl	$.LC47, 4(%esp)
	movl	%ecx, 12(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	28(%esi,%ebx,8), %ecx
	movl	24(%esi,%ebx,8), %eax
	addl	$4, %ebx
	movl	$.LC47, 4(%esp)
	movl	%ecx, 12(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jl	.L606
.L608:
	movl	$10, 32(%esp)
	movl	stderr, %eax
	movl	%eax, 36(%esp)
	addl	$16, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	jmp	fputc
.L628:
	testl	%eax, %eax
	je	.L606
	cmpl	$1, %eax
	jle	.L611
	cmpl	$2, %eax
	jg	.L629
.L612:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	incl	%ebx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L611
.L629:
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	4(%esi), %ecx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L612
.Lfe9:
	.size	dump_vector,.Lfe9-dump_vector
	.section	.rodata.str1.32
	.align 32
.LC48:
	.string	"allocation failure in ivector()"
	.text
	.p2align 4,,15
.globl ivector
	.type	ivector,@function
ivector:
	subl	$12, %esp
	movl	16(%esp), %eax
	movl	%ebx, 8(%esp)
	movl	$4, 4(%esp)
	movl	%eax, (%esp)
	call	calloc
	testl	%eax, %eax
	movl	%eax, %ebx
	je	.L632
.L631:
	movl	%ebx, %eax
	movl	8(%esp), %ebx
	addl	$12, %esp
	ret
.L632:
	movl	$.LC48, (%esp)
	call	fatal
	jmp	.L631
.Lfe10:
	.size	ivector,.Lfe10-ivector
	.p2align 4,,15
.globl free_ivector
	.type	free_ivector,@function
free_ivector:
	jmp	free
.Lfe11:
	.size	free_ivector,.Lfe11-free_ivector
	.section	.rodata.str1.1
.LC49:
	.string	"%d\t"
.LC50:
	.string	"\n\n"
	.text
	.p2align 4,,15
.globl dump_ivector
	.type	dump_ivector,@function
dump_ivector:
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	xorl	%ebx, %ebx
	subl	$16, %esp
	movl	36(%esp), %edi
	movl	32(%esp), %esi
	cmpl	%edi, %ebx
	jge	.L641
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L661
.L644:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jge	.L641
	.p2align 4,,15
.L639:
	movl	(%esi,%ebx,4), %eax
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	4(%esi,%ebx,4), %eax
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	8(%esi,%ebx,4), %eax
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	12(%esi,%ebx,4), %eax
	addl	$4, %ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jl	.L639
.L641:
	movl	$2, 8(%esp)
	movl	stderr, %eax
	movl	$1, 4(%esp)
	movl	$.LC50, (%esp)
	movl	%eax, 12(%esp)
	call	fwrite
	addl	$16, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	ret
.L661:
	testl	%eax, %eax
	je	.L639
	cmpl	$1, %eax
	jle	.L644
	cmpl	$2, %eax
	jg	.L662
.L645:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L644
.L662:
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L645
.Lfe12:
	.size	dump_ivector,.Lfe12-dump_ivector
	.section	.rodata.str1.1
.LC51:
	.string	"matrix"
.LC52:
	.string	"RCutil.c"
.LC54:
	.string	"m[i] != ((void *)0)"
.LC53:
	.string	"m != ((void *)0)"
	.text
	.p2align 4,,15
.globl matrix
	.type	matrix,@function
matrix:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$28, %esp
	movl	48(%esp), %eax
	movl	$4, 4(%esp)
	movl	52(%esp), %ebp
	movl	%eax, (%esp)
	call	calloc
	testl	%eax, %eax
	movl	%eax, %edi
	je	.L703
	xorl	%esi, %esi
	cmpl	48(%esp), %esi
	jge	.L674
	movl	48(%esp), %eax
	andl	$3, %eax
	cmpl	$1, 48(%esp)
	jg	.L704
.L677:
	movl	$8, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%esi,4)
	testl	%eax, %eax
	je	.L701
	incl	%esi
	cmpl	48(%esp), %esi
	jge	.L674
	.p2align 4,,15
.L672:
	movl	$8, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%esi,4)
	testl	%eax, %eax
	je	.L701
	movl	$8, 4(%esp)
	leal	1(%esi), %ebx
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%ebx,4)
	testl	%eax, %eax
	je	.L701
	movl	$8, 4(%esp)
	leal	2(%esi), %ebx
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%ebx,4)
	testl	%eax, %eax
	je	.L701
	movl	$8, 4(%esp)
	leal	3(%esi), %ebx
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%ebx,4)
	testl	%eax, %eax
	je	.L701
	addl	$4, %esi
	cmpl	48(%esp), %esi
	jl	.L672
.L674:
	addl	$28, %esp
	movl	%edi, %eax
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L701:
	movl	$.LC51, 12(%esp)
	movl	$236, 8(%esp)
	movl	$.LC52, 4(%esp)
	movl	$.LC54, (%esp)
.L702:
	call	__assert_fail
.L704:
	testl	%eax, %eax
	je	.L672
	cmpl	$1, %eax
	jle	.L677
	cmpl	$2, %eax
	jg	.L705
.L678:
	movl	$8, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%esi,4)
	testl	%eax, %eax
	je	.L701
	incl	%esi
	jmp	.L677
.L705:
	movl	$8, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi)
	testl	%eax, %eax
	je	.L701
	incl	%esi
	jmp	.L678
.L703:
	movl	$.LC51, 12(%esp)
	movl	$232, 8(%esp)
	movl	$.LC52, 4(%esp)
	movl	$.LC53, (%esp)
	jmp	.L702
.Lfe13:
	.size	matrix,.Lfe13-matrix
	.p2align 4,,15
.globl free_matrix
	.type	free_matrix,@function
free_matrix:
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	xorl	%ebx, %ebx
	subl	$16, %esp
	movl	36(%esp), %edi
	movl	32(%esp), %esi
	cmpl	%edi, %ebx
	jge	.L713
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L733
.L716:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	%eax, (%esp)
	call	free
	cmpl	%edi, %ebx
	jge	.L713
	.p2align 4,,15
.L711:
	movl	(%esi,%ebx,4), %eax
	movl	%eax, (%esp)
	call	free
	movl	4(%esi,%ebx,4), %eax
	movl	%eax, (%esp)
	call	free
	movl	8(%esi,%ebx,4), %eax
	movl	%eax, (%esp)
	call	free
	movl	12(%esi,%ebx,4), %eax
	addl	$4, %ebx
	movl	%eax, (%esp)
	call	free
	cmpl	%edi, %ebx
	jl	.L711
.L713:
	movl	%esi, 32(%esp)
	addl	$16, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	jmp	free
.L733:
	testl	%eax, %eax
	je	.L711
	cmpl	$1, %eax
	jle	.L716
	cmpl	$2, %eax
	jg	.L734
.L717:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	%eax, (%esp)
	call	free
	jmp	.L716
.L734:
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	%eax, (%esp)
	call	free
	jmp	.L717
.Lfe14:
	.size	free_matrix,.Lfe14-free_matrix
	.p2align 4,,15
.globl dump_matrix
	.type	dump_matrix,@function
dump_matrix:
	pushl	%ebp
	xorl	%ebp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$28, %esp
	movl	48(%esp), %eax
	movl	56(%esp), %edi
	movl	%eax, 24(%esp)
	movl	52(%esp), %eax
	cmpl	%eax, %ebp
	movl	%eax, 20(%esp)
	jge	.L748
	.p2align 4,,15
.L746:
	movl	24(%esp), %eax
	xorl	%ebx, %ebx
	cmpl	%edi, %ebx
	movl	(%eax,%ebp,4), %esi
	jge	.L750
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L770
.L753:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	incl	%ebx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jge	.L750
	.p2align 4,,15
.L744:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	movl	12(%esi,%ebx,8), %ecx
	movl	8(%esi,%ebx,8), %eax
	movl	$.LC47, 4(%esp)
	movl	%ecx, 12(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	20(%esi,%ebx,8), %ecx
	movl	16(%esi,%ebx,8), %eax
	movl	$.LC47, 4(%esp)
	movl	%ecx, 12(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	28(%esi,%ebx,8), %ecx
	movl	24(%esi,%ebx,8), %eax
	addl	$4, %ebx
	movl	$.LC47, 4(%esp)
	movl	%ecx, 12(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jl	.L744
.L750:
	movl	$10, (%esp)
	movl	stderr, %eax
	incl	%ebp
	movl	%eax, 4(%esp)
	call	fputc
	cmpl	20(%esp), %ebp
	jl	.L746
.L748:
	movl	$10, 48(%esp)
	movl	stderr, %eax
	movl	%eax, 52(%esp)
	addl	$28, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	jmp	fputc
.L770:
	testl	%eax, %eax
	je	.L744
	cmpl	$1, %eax
	jle	.L753
	cmpl	$2, %eax
	jg	.L771
.L754:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	incl	%ebx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L753
.L771:
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	4(%esi), %ecx
	movl	$.LC47, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%ecx, 12(%esp)
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L754
.Lfe15:
	.size	dump_matrix,.Lfe15-dump_matrix
	.p2align 4,,15
.globl copy_matrix
	.type	copy_matrix,@function
copy_matrix:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$4, %esp
	movl	32(%esp), %eax
	movl	$0, (%esp)
	movl	36(%esp), %ebp
	cmpl	%eax, (%esp)
	jge	.L784
	andl	$1, %eax
	cmpl	$1, 32(%esp)
	jg	.L836
.L808:
	xorl	%ebx, %ebx
	cmpl	%ebp, (%esp)
	jge	.L811
	movl	24(%esp), %eax
	movl	(%eax), %edi
	movl	28(%esp), %eax
	movl	(%eax), %esi
	movl	%ebp, %eax
	andl	$3, %eax
	cmpl	$1, %ebp
	jg	.L837
.L814:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	incl	%ebx
	cmpl	%ebp, %ebx
	jge	.L811
	.p2align 4,,15
.L809:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	leal	1(%ebx), %ecx
	movl	(%esi,%ecx,8), %eax
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	leal	2(%ebx), %ecx
	movl	(%esi,%ecx,8), %eax
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	leal	3(%ebx), %ecx
	addl	$4, %ebx
	movl	(%esi,%ecx,8), %eax
	cmpl	%ebp, %ebx
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	jl	.L809
.L811:
	incl	(%esp)
	movl	32(%esp), %eax
	cmpl	%eax, (%esp)
	jge	.L784
	.p2align 4,,15
.L782:
	xorl	%ebx, %ebx
	cmpl	%ebp, %ebx
	jge	.L820
	movl	(%esp), %eax
	movl	24(%esp), %esi
	movl	28(%esp), %ecx
	movl	(%esi,%eax,4), %edi
	movl	(%ecx,%eax,4), %esi
	movl	%ebp, %eax
	andl	$3, %eax
	cmpl	$1, %ebp
	jg	.L838
.L823:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	incl	%ebx
	cmpl	%ebp, %ebx
	jge	.L820
	.p2align 4,,15
.L818:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	leal	1(%ebx), %ecx
	movl	(%esi,%ecx,8), %eax
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	leal	2(%ebx), %ecx
	movl	(%esi,%ecx,8), %eax
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	leal	3(%ebx), %ecx
	addl	$4, %ebx
	movl	(%esi,%ecx,8), %eax
	cmpl	%ebp, %ebx
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	jl	.L818
.L820:
	movl	(%esp), %eax
	xorl	%ebx, %ebx
	incl	%eax
	cmpl	%ebp, %ebx
	jge	.L829
	movl	24(%esp), %esi
	movl	28(%esp), %ecx
	movl	(%esi,%eax,4), %edi
	movl	(%ecx,%eax,4), %esi
	movl	%ebp, %eax
	andl	$3, %eax
	cmpl	$1, %ebp
	jg	.L839
.L832:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	incl	%ebx
	cmpl	%ebp, %ebx
	jge	.L829
	.p2align 4,,15
.L827:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	leal	1(%ebx), %ecx
	movl	(%esi,%ecx,8), %eax
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	leal	2(%ebx), %ecx
	movl	(%esi,%ecx,8), %eax
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	leal	3(%ebx), %ecx
	addl	$4, %ebx
	movl	(%esi,%ecx,8), %eax
	cmpl	%ebp, %ebx
	movl	4(%esi,%ecx,8), %edx
	movl	%eax, (%edi,%ecx,8)
	movl	%edx, 4(%edi,%ecx,8)
	jl	.L827
.L829:
	addl	$2, (%esp)
	movl	32(%esp), %eax
	cmpl	%eax, (%esp)
	jl	.L782
.L784:
	popl	%eax
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L839:
	testl	%eax, %eax
	je	.L827
	cmpl	$1, %eax
	jle	.L832
	cmpl	$2, %eax
	jle	.L833
	movl	4(%esi), %ebx
	movl	(%esi), %eax
	movl	%ebx, 4(%edi)
	movl	$1, %ebx
	movl	%eax, (%edi)
.L833:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	incl	%ebx
	jmp	.L832
.L838:
	testl	%eax, %eax
	je	.L818
	cmpl	$1, %eax
	jle	.L823
	cmpl	$2, %eax
	jle	.L824
	movl	4(%esi), %ebx
	movl	(%esi), %eax
	movl	%ebx, 4(%edi)
	movl	$1, %ebx
	movl	%eax, (%edi)
.L824:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	incl	%ebx
	jmp	.L823
.L837:
	testl	%eax, %eax
	je	.L809
	cmpl	$1, %eax
	jle	.L814
	cmpl	$2, %eax
	jle	.L815
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	4(%esi), %ecx
	movl	%eax, (%edi)
	movl	%ecx, 4(%edi)
.L815:
	movl	(%esi,%ebx,8), %eax
	movl	4(%esi,%ebx,8), %ecx
	movl	%eax, (%edi,%ebx,8)
	movl	%ecx, 4(%edi,%ebx,8)
	incl	%ebx
	jmp	.L814
.L836:
	testl	%eax, %eax
	je	.L782
	jmp	.L808
.Lfe16:
	.size	copy_matrix,.Lfe16-copy_matrix
	.section	.rodata.str1.1
.LC55:
	.string	"imatrix"
	.text
	.p2align 4,,15
.globl imatrix
	.type	imatrix,@function
imatrix:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$28, %esp
	movl	48(%esp), %eax
	movl	$4, 4(%esp)
	movl	52(%esp), %ebp
	movl	%eax, (%esp)
	call	calloc
	testl	%eax, %eax
	movl	%eax, %edi
	je	.L880
	xorl	%esi, %esi
	cmpl	48(%esp), %esi
	jge	.L851
	movl	48(%esp), %eax
	andl	$3, %eax
	cmpl	$1, 48(%esp)
	jg	.L881
.L854:
	movl	$4, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%esi,4)
	testl	%eax, %eax
	je	.L878
	incl	%esi
	cmpl	48(%esp), %esi
	jge	.L851
	.p2align 4,,15
.L849:
	movl	$4, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%esi,4)
	testl	%eax, %eax
	je	.L878
	movl	$4, 4(%esp)
	leal	1(%esi), %ebx
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%ebx,4)
	testl	%eax, %eax
	je	.L878
	movl	$4, 4(%esp)
	leal	2(%esi), %ebx
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%ebx,4)
	testl	%eax, %eax
	je	.L878
	movl	$4, 4(%esp)
	leal	3(%esi), %ebx
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%ebx,4)
	testl	%eax, %eax
	je	.L878
	addl	$4, %esi
	cmpl	48(%esp), %esi
	jl	.L849
.L851:
	addl	$28, %esp
	movl	%edi, %eax
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L878:
	movl	$.LC55, 12(%esp)
	movl	$276, 8(%esp)
	movl	$.LC52, 4(%esp)
	movl	$.LC54, (%esp)
.L879:
	call	__assert_fail
.L881:
	testl	%eax, %eax
	je	.L849
	cmpl	$1, %eax
	jle	.L854
	cmpl	$2, %eax
	jg	.L882
.L855:
	movl	$4, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi,%esi,4)
	testl	%eax, %eax
	je	.L878
	incl	%esi
	jmp	.L854
.L882:
	movl	$4, 4(%esp)
	movl	%ebp, (%esp)
	call	calloc
	movl	%eax, (%edi)
	testl	%eax, %eax
	je	.L878
	incl	%esi
	jmp	.L855
.L880:
	movl	$.LC55, 12(%esp)
	movl	$272, 8(%esp)
	movl	$.LC52, 4(%esp)
	movl	$.LC53, (%esp)
	jmp	.L879
.Lfe17:
	.size	imatrix,.Lfe17-imatrix
	.p2align 4,,15
.globl free_imatrix
	.type	free_imatrix,@function
free_imatrix:
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	xorl	%ebx, %ebx
	subl	$16, %esp
	movl	36(%esp), %edi
	movl	32(%esp), %esi
	cmpl	%edi, %ebx
	jge	.L890
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L910
.L893:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	%eax, (%esp)
	call	free
	cmpl	%edi, %ebx
	jge	.L890
	.p2align 4,,15
.L888:
	movl	(%esi,%ebx,4), %eax
	movl	%eax, (%esp)
	call	free
	movl	4(%esi,%ebx,4), %eax
	movl	%eax, (%esp)
	call	free
	movl	8(%esi,%ebx,4), %eax
	movl	%eax, (%esp)
	call	free
	movl	12(%esi,%ebx,4), %eax
	addl	$4, %ebx
	movl	%eax, (%esp)
	call	free
	cmpl	%edi, %ebx
	jl	.L888
.L890:
	movl	%esi, 32(%esp)
	addl	$16, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	jmp	free
.L910:
	testl	%eax, %eax
	je	.L888
	cmpl	$1, %eax
	jle	.L893
	cmpl	$2, %eax
	jg	.L911
.L894:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	%eax, (%esp)
	call	free
	jmp	.L893
.L911:
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	%eax, (%esp)
	call	free
	jmp	.L894
.Lfe18:
	.size	free_imatrix,.Lfe18-free_imatrix
	.p2align 4,,15
.globl dump_imatrix
	.type	dump_imatrix,@function
dump_imatrix:
	pushl	%ebp
	xorl	%ebp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$28, %esp
	movl	48(%esp), %eax
	movl	56(%esp), %edi
	movl	%eax, 24(%esp)
	movl	52(%esp), %eax
	cmpl	%eax, %ebp
	movl	%eax, 20(%esp)
	jge	.L925
	.p2align 4,,15
.L923:
	movl	24(%esp), %eax
	xorl	%ebx, %ebx
	cmpl	%edi, %ebx
	movl	(%eax,%ebp,4), %esi
	jge	.L927
	movl	%edi, %eax
	andl	$3, %eax
	cmpl	$1, %edi
	jg	.L947
.L930:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jge	.L927
	.p2align 4,,15
.L921:
	movl	(%esi,%ebx,4), %eax
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	4(%esi,%ebx,4), %eax
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	8(%esi,%ebx,4), %eax
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	movl	12(%esi,%ebx,4), %eax
	addl	$4, %ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	cmpl	%edi, %ebx
	jl	.L921
.L927:
	movl	$2, 8(%esp)
	movl	stderr, %eax
	incl	%ebp
	movl	$1, 4(%esp)
	movl	$.LC50, (%esp)
	movl	%eax, 12(%esp)
	call	fwrite
	cmpl	20(%esp), %ebp
	jl	.L923
.L925:
	movl	$10, 48(%esp)
	movl	stderr, %eax
	movl	%eax, 52(%esp)
	addl	$28, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	jmp	fputc
.L947:
	testl	%eax, %eax
	je	.L921
	cmpl	$1, %eax
	jle	.L930
	cmpl	$2, %eax
	jg	.L948
.L931:
	movl	(%esi,%ebx,4), %eax
	incl	%ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L930
.L948:
	movl	(%esi), %eax
	movl	$1, %ebx
	movl	$.LC49, 4(%esp)
	movl	%eax, 8(%esp)
	movl	stderr, %eax
	movl	%eax, (%esp)
	call	fprintf
	jmp	.L931
.Lfe19:
	.size	dump_imatrix,.Lfe19-dump_imatrix
	.p2align 4,,15
.globl matmult
	.type	matmult,@function
matmult:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$4, %esp
	movl	36(%esp), %eax
	movl	$0, (%esp)
	movl	32(%esp), %ebp
	cmpl	%eax, (%esp)
	jge	.L966
.L964:
	xorl	%edi, %edi
	cmpl	36(%esp), %edi
	jge	.L968
	movl	(%esp), %ecx
	movl	24(%esp), %eax
	movl	(%eax,%ecx,4), %ebx
	.p2align 4,,15
.L963:
	movl	$0, (%ebx)
	xorl	%ecx, %ecx
	cmpl	36(%esp), %ecx
	movl	$0, 4(%ebx)
	jge	.L970
	movl	28(%esp), %eax
	movl	(%esp), %edx
	movl	(%eax,%edx,4), %esi
	movl	36(%esp), %eax
	andl	$3, %eax
	cmpl	$1, 36(%esp)
	jg	.L990
.L973:
	movl	(%ebp,%ecx,4), %eax
	fldl	(%eax,%edi,8)
	fmull	(%esi,%ecx,8)
	incl	%ecx
	cmpl	36(%esp), %ecx
	faddl	(%ebx)
	fstpl	(%ebx)
	jge	.L970
	.p2align 4,,15
.L962:
	movl	(%ebp,%ecx,4), %eax
	leal	1(%ecx), %edx
	fldl	(%eax,%edi,8)
	movl	(%ebp,%edx,4), %eax
	fmull	(%esi,%ecx,8)
	faddl	(%ebx)
	fstl	(%ebx)
	fldl	(%eax,%edi,8)
	fmull	(%esi,%edx,8)
	leal	2(%ecx), %edx
	movl	(%ebp,%edx,4), %eax
	faddp	%st, %st(1)
	fstl	(%ebx)
	fldl	(%eax,%edi,8)
	fmull	(%esi,%edx,8)
	leal	3(%ecx), %edx
	addl	$4, %ecx
	cmpl	36(%esp), %ecx
	movl	(%ebp,%edx,4), %eax
	faddp	%st, %st(1)
	fstl	(%ebx)
	fldl	(%eax,%edi,8)
	fmull	(%esi,%edx,8)
	faddp	%st, %st(1)
	fstpl	(%ebx)
	jl	.L962
.L970:
	incl	%edi
	addl	$8, %ebx
	cmpl	36(%esp), %edi
	jl	.L963
.L968:
	incl	(%esp)
	movl	36(%esp), %eax
	cmpl	%eax, (%esp)
	jl	.L964
.L966:
	popl	%eax
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L990:
	testl	%eax, %eax
	je	.L962
	cmpl	$1, %eax
	jle	.L973
	cmpl	$2, %eax
	jle	.L974
	movl	(%ebp), %eax
	movl	$1, %ecx
	fldl	(%eax,%edi,8)
	fmull	(%esi)
	faddl	(%ebx)
	fstpl	(%ebx)
.L974:
	movl	(%ebp,%ecx,4), %eax
	fldl	(%eax,%edi,8)
	fmull	(%esi,%ecx,8)
	incl	%ecx
	faddl	(%ebx)
	fstpl	(%ebx)
	jmp	.L973
.Lfe20:
	.size	matmult,.Lfe20-matmult
	.p2align 4,,15
.globl matvectmult
	.type	matvectmult,@function
matvectmult:
	pushl	%ebp
	xorl	%ebp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	32(%esp), %edi
	movl	28(%esp), %ebx
	cmpl	%edi, %ebp
	jge	.L1003
	movl	20(%esp), %edx
	.p2align 4,,15
.L1001:
	movl	$0, (%edx)
	xorl	%eax, %eax
	cmpl	%edi, %eax
	movl	$0, 4(%edx)
	jge	.L1005
	movl	24(%esp), %esi
	movl	(%esi,%ebp,4), %ecx
	movl	%edi, %esi
	andl	$3, %esi
	cmpl	$1, %edi
	jg	.L1025
.L1008:
	fldl	(%ebx,%eax,8)
	fmull	(%ecx,%eax,8)
	incl	%eax
	cmpl	%edi, %eax
	faddl	(%edx)
	fstpl	(%edx)
	jge	.L1005
	.p2align 4,,15
.L1000:
	fldl	(%ebx,%eax,8)
	fmull	(%ecx,%eax,8)
	faddl	(%edx)
	fstl	(%edx)
	fldl	8(%ecx,%eax,8)
	fmull	8(%ebx,%eax,8)
	faddp	%st, %st(1)
	fstl	(%edx)
	fldl	16(%ecx,%eax,8)
	fmull	16(%ebx,%eax,8)
	faddp	%st, %st(1)
	fstl	(%edx)
	fldl	24(%ecx,%eax,8)
	fmull	24(%ebx,%eax,8)
	addl	$4, %eax
	cmpl	%edi, %eax
	faddp	%st, %st(1)
	fstpl	(%edx)
	jl	.L1000
.L1005:
	incl	%ebp
	addl	$8, %edx
	cmpl	%edi, %ebp
	jl	.L1001
.L1003:
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L1025:
	testl	%esi, %esi
	je	.L1000
	cmpl	$1, %esi
	jle	.L1008
	cmpl	$2, %esi
	jle	.L1009
	fldl	(%ebx)
	movl	$1, %eax
	fmull	(%ecx)
	faddl	(%edx)
	fstpl	(%edx)
.L1009:
	fldl	(%ebx,%eax,8)
	fmull	(%ecx,%eax,8)
	incl	%eax
	faddl	(%edx)
	fstpl	(%edx)
	jmp	.L1008
.Lfe21:
	.size	matvectmult,.Lfe21-matvectmult
	.p2align 4,,15
.globl matinv
	.type	matinv,@function
matinv:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	movl	64(%esp), %eax
	movl	$4, 4(%esp)
	movl	72(%esp), %esi
	movl	%eax, 40(%esp)
	movl	68(%esp), %eax
	movl	%esi, (%esp)
	movl	%eax, 36(%esp)
	call	calloc
	movl	%eax, 20(%esp)
	testl	%eax, %eax
	je	.L1192
.L1027:
	movl	%esi, (%esp)
	call	vector
	movl	%eax, 28(%esp)
	movl	%esi, (%esp)
	call	vector
	movl	20(%esp), %edx
	movl	%eax, 24(%esp)
	movl	36(%esp), %eax
	movl	%edx, 8(%esp)
	movl	%esi, 4(%esp)
	movl	%eax, (%esp)
	call	lupdcmp
	movl	$0, 32(%esp)
	cmpl	%esi, 32(%esp)
	jge	.L1071
	leal	-1(%esi), %ecx
	movl	%ecx, 16(%esp)
.L1068:
	xorl	%ebx, %ebx
	cmpl	%esi, %ebx
	jge	.L1073
	movl	%esi, %eax
	andl	$3, %eax
	cmpl	$1, %esi
	jg	.L1193
.L1164:
	movl	28(%esp), %ebp
	xorl	%edx, %edx
	movl	$0, (%ebp,%ebx,8)
	movl	%edx, 4(%ebp,%ebx,8)
	incl	%ebx
	cmpl	%esi, %ebx
	jge	.L1073
.L1037:
	movl	28(%esp), %ecx
	xorl	%edx, %edx
	xorl	%ebp, %ebp
	xorl	%edi, %edi
	xorl	%eax, %eax
	movl	%edx, 4(%ecx,%ebx,8)
	xorl	%edx, %edx
	movl	%ebp, 8(%ecx,%ebx,8)
	xorl	%ebp, %ebp
	movl	%edi, 12(%ecx,%ebx,8)
	xorl	%edi, %edi
	movl	$0, (%ecx,%ebx,8)
	movl	%edx, 16(%ecx,%ebx,8)
	movl	%ebp, 20(%ecx,%ebx,8)
	movl	%eax, 24(%ecx,%ebx,8)
	movl	%edi, 28(%ecx,%ebx,8)
	addl	$4, %ebx
	cmpl	%esi, %ebx
	jl	.L1037
.L1073:
	movl	32(%esp), %edi
	movl	$1072693248, %eax
	xorl	%ebx, %ebx
	movl	28(%esp), %ecx
	movl	$0, (%ecx,%edi,8)
	movl	%eax, 4(%ecx,%edi,8)
	movl	%esi, (%esp)
	call	vector
	cmpl	%esi, %ebx
	movl	%eax, %edi
	jge	.L1075
	movl	36(%esp), %ebp
	fldz
	.p2align 4,,15
.L1049:
	xorl	%ecx, %ecx
	cmpl	%ebx, %ecx
	fld	%st(0)
	jge	.L1077
	movl	%ebx, %eax
	andl	$3, %eax
	cmpl	$1, %ebx
	jg	.L1194
.L1131:
	fldl	(%edi,%ecx,8)
	cmpl	%ecx, %ebx
	jle	.L1143
	movl	(%ebp), %eax
	fmull	(%eax,%ecx,8)
.L1183:
	incl	%ecx
	cmpl	%ebx, %ecx
	faddp	%st, %st(1)
	jge	.L1077
	.p2align 4,,15
.L1048:
	fldl	(%edi,%ecx,8)
	cmpl	%ecx, %ebx
	jle	.L1147
	movl	(%ebp), %eax
	fmull	(%eax,%ecx,8)
.L1184:
	leal	1(%ecx), %edx
	cmpl	%edx, %ebx
	faddp	%st, %st(1)
	fldl	(%edi,%edx,8)
	jle	.L1151
	movl	(%ebp), %eax
	fmull	(%eax,%edx,8)
.L1185:
	leal	2(%ecx), %edx
	cmpl	%edx, %ebx
	faddp	%st, %st(1)
	fldl	(%edi,%edx,8)
	jle	.L1155
	movl	(%ebp), %eax
	fmull	(%eax,%edx,8)
.L1186:
	leal	3(%ecx), %edx
	cmpl	%edx, %ebx
	faddp	%st, %st(1)
	fldl	(%edi,%edx,8)
	jle	.L1159
	movl	(%ebp), %eax
	fmull	(%eax,%edx,8)
.L1187:
	addl	$4, %ecx
	cmpl	%ebx, %ecx
	faddp	%st, %st(1)
	jl	.L1048
.L1077:
	movl	20(%esp), %ecx
	addl	$4, %ebp
	movl	28(%esp), %edx
	movl	(%ecx,%ebx,4), %eax
	fldl	(%edx,%eax,8)
	fsubp	%st, %st(1)
	fstpl	(%edi,%ebx,8)
	incl	%ebx
	cmpl	%esi, %ebx
	jl	.L1049
	fstp	%st(0)
.L1075:
	movl	16(%esp), %ebx
	testl	%ebx, %ebx
	js	.L1079
	movl	%ebx, %eax
	movl	%ebx, %edx
	notl	%eax
	andl	$1, %eax
	decl	%edx
	fldz
	jns	.L1195
.L1107:
	leal	1(%ebx), %ecx
	cmpl	%esi, %ecx
	fld	%st(0)
	jge	.L1113
	movl	36(%esp), %eax
	movl	(%eax,%ebx,4), %edx
	.p2align 4,,15
.L1110:
	movl	24(%esp), %eax
	cmpl	%ecx, %ebx
	fldl	(%eax,%ecx,8)
	jg	.L1109
	fmull	(%edx,%ecx,8)
.L1188:
	incl	%ecx
	cmpl	%esi, %ecx
	faddp	%st, %st(1)
	jl	.L1110
.L1112:
	fldl	(%edi,%ebx,8)
	movl	24(%esp), %ebp
	fsubp	%st, %st(1)
	fdivl	(%edx,%ebx,8)
	fstpl	(%ebp,%ebx,8)
	decl	%ebx
	js	.L1191
	.p2align 4,,15
.L1061:
	leal	1(%ebx), %ecx
	cmpl	%esi, %ecx
	fld	%st(0)
	jge	.L1120
	movl	36(%esp), %eax
	movl	(%eax,%ebx,4), %edx
	.p2align 4,,15
.L1117:
	movl	24(%esp), %eax
	cmpl	%ecx, %ebx
	fldl	(%eax,%ecx,8)
	jg	.L1116
	fmull	(%edx,%ecx,8)
.L1189:
	incl	%ecx
	cmpl	%esi, %ecx
	faddp	%st, %st(1)
	jl	.L1117
.L1119:
	fldl	(%edi,%ebx,8)
	cmpl	%esi, %ebx
	movl	24(%esp), %ecx
	leal	-1(%ebx), %eax
	fsubp	%st, %st(1)
	fld	%st(1)
	fxch	%st(1)
	fdivl	(%edx,%ebx,8)
	fstpl	(%ecx,%ebx,8)
	movl	%ebx, %ecx
	jge	.L1127
	movl	36(%esp), %ebp
	movl	(%ebp,%eax,4), %edx
	.p2align 4,,15
.L1124:
	movl	24(%esp), %ebp
	cmpl	%ecx, %eax
	fldl	(%ebp,%ecx,8)
	jg	.L1123
	fmull	(%edx,%ecx,8)
.L1190:
	incl	%ecx
	cmpl	%esi, %ecx
	faddp	%st, %st(1)
	jl	.L1124
.L1126:
	fldl	(%edi,%eax,8)
	subl	$2, %ebx
	fsubp	%st, %st(1)
	fdivl	(%edx,%eax,8)
	movl	24(%esp), %edx
	fstpl	(%edx,%eax,8)
	jns	.L1061
	fstp	%st(0)
.L1079:
	movl	%edi, (%esp)
	xorl	%ebx, %ebx
	call	free_vector
	cmpl	%esi, %ebx
	jge	.L1083
	movl	%esi, %eax
	andl	$3, %eax
	cmpl	$1, %esi
	jg	.L1196
.L1087:
	movl	40(%esp), %eax
	movl	24(%esp), %edx
	movl	32(%esp), %edi
	movl	(%eax,%ebx,4), %ecx
	movl	4(%edx,%ebx,8), %ebp
	movl	(%edx,%ebx,8), %eax
	incl	%ebx
	cmpl	%esi, %ebx
	movl	%ebp, 4(%ecx,%edi,8)
	movl	%eax, (%ecx,%edi,8)
	jge	.L1083
.L1067:
	movl	24(%esp), %edi
	movl	40(%esp), %eax
	movl	32(%esp), %ebp
	movl	4(%edi,%ebx,8), %edx
	movl	(%eax,%ebx,4), %ecx
	movl	(%edi,%ebx,8), %eax
	movl	%edx, 4(%ecx,%ebp,8)
	movl	40(%esp), %edx
	movl	%eax, (%ecx,%ebp,8)
	leal	1(%ebx), %eax
	movl	(%edx,%eax,4), %ecx
	movl	4(%edi,%eax,8), %edx
	movl	(%edi,%eax,8), %eax
	movl	%edx, 4(%ecx,%ebp,8)
	movl	40(%esp), %edx
	movl	%eax, (%ecx,%ebp,8)
	movl	32(%esp), %ebp
	leal	2(%ebx), %eax
	movl	(%edx,%eax,4), %ecx
	movl	4(%edi,%eax,8), %edx
	movl	(%edi,%eax,8), %eax
	movl	%edx, 4(%ecx,%ebp,8)
	movl	40(%esp), %edx
	movl	%eax, (%ecx,%ebp,8)
	leal	3(%ebx), %eax
	addl	$4, %ebx
	cmpl	%esi, %ebx
	movl	(%edx,%eax,4), %ecx
	movl	4(%edi,%eax,8), %edx
	movl	(%edi,%eax,8), %eax
	movl	%edx, 4(%ecx,%ebp,8)
	movl	%eax, (%ecx,%ebp,8)
	jl	.L1067
.L1083:
	incl	32(%esp)
	cmpl	%esi, 32(%esp)
	jl	.L1068
.L1071:
	movl	20(%esp), %ebx
	movl	%ebx, (%esp)
	call	free
	movl	28(%esp), %eax
	movl	%eax, (%esp)
	call	free_vector
	movl	24(%esp), %esi
	movl	%esi, 64(%esp)
	addl	$44, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	jmp	free_vector
.L1196:
	testl	%eax, %eax
	je	.L1067
	cmpl	$1, %eax
	jle	.L1087
	cmpl	$2, %eax
	jle	.L1088
	movl	40(%esp), %eax
	movl	24(%esp), %ecx
	movl	32(%esp), %ebx
	movl	(%eax), %ebp
	movl	4(%ecx), %edi
	movl	(%ecx), %eax
	movl	%edi, 4(%ebp,%ebx,8)
	movl	%eax, (%ebp,%ebx,8)
	movl	$1, %ebx
.L1088:
	movl	40(%esp), %eax
	movl	24(%esp), %edx
	movl	32(%esp), %edi
	movl	(%eax,%ebx,4), %ecx
	movl	4(%edx,%ebx,8), %ebp
	movl	(%edx,%ebx,8), %eax
	incl	%ebx
	movl	%ebp, 4(%ecx,%edi,8)
	movl	%eax, (%ecx,%edi,8)
	jmp	.L1087
	.p2align 4,,7
.L1123:
	fmul	%st(2), %st
	jmp	.L1190
.L1127:
	movl	36(%esp), %ebp
	movl	(%ebp,%eax,4), %edx
	jmp	.L1126
	.p2align 4,,7
.L1116:
	fmul	%st(2), %st
	jmp	.L1189
.L1120:
	movl	36(%esp), %ebp
	movl	(%ebp,%ebx,4), %edx
	jmp	.L1119
.L1191:
	fstp	%st(0)
	jmp	.L1079
.L1109:
	fmul	%st(2), %st
	jmp	.L1188
.L1113:
	movl	36(%esp), %eax
	movl	(%eax,%ebx,4), %edx
	jmp	.L1112
.L1195:
	testl	%eax, %eax
	je	.L1061
	jmp	.L1107
	.p2align 4,,7
.L1159:
	fmul	%st(2), %st
	jmp	.L1187
	.p2align 4,,7
.L1155:
	fmul	%st(2), %st
	jmp	.L1186
	.p2align 4,,7
.L1151:
	fmul	%st(2), %st
	jmp	.L1185
	.p2align 4,,7
.L1147:
	fmul	%st(2), %st
	jmp	.L1184
.L1143:
	fmul	%st(2), %st
	jmp	.L1183
.L1194:
	testl	%eax, %eax
	je	.L1048
	cmpl	$1, %eax
	jle	.L1131
	cmpl	$2, %eax
	jle	.L1132
	fldl	(%edi)
	cmpl	$0, %ebx
	jle	.L1135
	movl	(%ebp), %eax
	fmull	(%eax)
.L1181:
	faddp	%st, %st(1)
	incl	%ecx
.L1132:
	fldl	(%edi,%ecx,8)
	cmpl	%ecx, %ebx
	jle	.L1139
	movl	(%ebp), %eax
	fmull	(%eax,%ecx,8)
.L1182:
	faddp	%st, %st(1)
	incl	%ecx
	jmp	.L1131
.L1139:
	fmul	%st(2), %st
	jmp	.L1182
.L1135:
	fmul	%st(2), %st
	jmp	.L1181
.L1193:
	testl	%eax, %eax
	je	.L1037
	cmpl	$1, %eax
	jle	.L1164
	cmpl	$2, %eax
	jle	.L1165
	movl	28(%esp), %ebx
	movl	$0, (%ebx)
	movl	$0, 4(%ebx)
	movl	$1, %ebx
.L1165:
	movl	28(%esp), %eax
	xorl	%edi, %edi
	movl	$0, (%eax,%ebx,8)
	movl	%edi, 4(%eax,%ebx,8)
	incl	%ebx
	jmp	.L1164
.L1192:
	movl	$.LC48, (%esp)
	call	fatal
	jmp	.L1027
.Lfe22:
	.size	matinv,.Lfe22-matinv
	.ident	"GCC: (GNU) 3.2.2 20030222 (Red Hat Linux 3.2.2-5)"
