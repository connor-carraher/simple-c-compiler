lexan:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$lexan.size, %eax
	subq	%rax, %rsp
#IF Start
#Equal Start
	mov	c, %r11d
	cmp	$0, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	je	.L1
#Assignment Start
	movl	$0, %eax
	call	getchar
	movl	%eax, c
#Assignment End
.L1:
#IF End

#While Start
.L3:
#LogicalAnd Start
	movl	c, %edi
	movl	$0, %eax
	call	isspace
	cmp	$0, %eax
	je	.L5
#Not Equal Start
	mov	c, %r11d
	cmp	NL, %r11d
	setne	%r11b
	movzbl	%r11b, %r11d
#Not Equal End
	cmp	$0, %r11d
	je	.L5
	mov	 $0, %r11
	jmp	.L6
.L5:
	mov	 $1, %r11
.L6:
#LogicalAnd End
	cmp	$0, %r11d
	je	.L4
#Assignment Start
	movl	$0, %eax
	call	getchar
	movl	%eax, c
#Assignment End
	jmp	.L3
.L4:
#While End

#IF Start
#Not Start
	movl	c, %edi
	movl	$0, %eax
	call	isdigit
	mov	%eax, %r11d
	cmpl	$0,%r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Not End
	cmp	$0, %r11d
	je	.L7
#Assignment Start
	mov	c, %r11d
	movl	%r11d, -8(%rbp)
#Assignment End

#Assignment Start
	mov	$0, %r11d
	movl	%r11d, c
#Assignment End

	mov	-8(%rbp), %eax
	jmp	.L0

.L7:
#IF End

#Assignment Start
	mov	$0, %r11d
	movl	%r11d, -4(%rbp)
#Assignment End

#While Start
.L9:
	movl	c, %edi
	movl	$0, %eax
	call	isdigit
	cmp	$0, %eax
	je	.L10
#Assignment Start
#Subtract Start
#Add start
#Multiply Start
	mov	-4(%rbp), %r11d
	imul	$10, %r11d
#Multiply End
	add	c, %r11d
#Add end
	sub	$48, %r11d
#Subtract End
	movl	%r11d, -4(%rbp)
#Assignment End

#Assignment Start
	movl	c, %edi
	movl	$0, %eax
	call	getchar
	movl	%eax, c
#Assignment End

	jmp	.L9
.L10:
#While End

#Assignment Start
	mov	-4(%rbp), %r11d
	movl	%r11d, lexval
#Assignment End

	mov	NUM, %eax
	jmp	.L0

.L0:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	lexan.size, 16
	.globl	lexan

match:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$match.size, %eax
	subq	%rax, %rsp
	movl	%edi, -4(%rbp)
#IF Start
#Not Equal Start
	mov	lookahead, %r11d
	cmp	-4(%rbp), %r11d
	setne	%r11b
	movzbl	%r11b, %r11d
#Not Equal End
	cmp	$0, %r11d
	je	.L12
#Address Start
	leaq	.L14,%r11
#Address End
	mov	%r11, -12(%rbp)	# spill
	movl	lookahead, %esi
	movq	-12(%rbp), %rdi
	movl	$0, %eax
	call	printf

	movl	$1, %edi
	movl	$0, %eax
	call	exit

.L12:
#IF End

#Assignment Start
	call	lexan
	movl	%eax, lookahead
#Assignment End

.L11:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	match.size, 16
	.globl	match

factor:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$factor.size, %eax
	subq	%rax, %rsp
#IF Start
#Equal Start
	mov	lookahead, %r11d
	cmp	LPAREN, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	je	.L16
	movl	LPAREN, %edi
	call	match

#Assignment Start
	movl	$0, %eax
	call	expr
	movl	%eax, -4(%rbp)
#Assignment End

	movl	RPAREN, %edi
	call	match

	mov	-4(%rbp), %eax
	jmp	.L15

.L16:
#IF End

#Assignment Start
	mov	lexval, %r11d
	movl	%r11d, -4(%rbp)
#Assignment End

	movl	NUM, %edi
	call	match

	mov	-4(%rbp), %eax
	jmp	.L15

.L15:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	factor.size, 16
	.globl	factor

term:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$term.size, %eax
	subq	%rax, %rsp
#Assignment Start
	call	factor
	movl	%eax, -4(%rbp)
#Assignment End

#While Start
.L19:
#LogicalOr Start
#Equal Start
	mov	lookahead, %r11d
	cmp	STAR, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	jne	.L21
#Equal Start
	mov	lookahead, %r11d
	cmp	SLASH, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	jne	.L21
	mov	 $0, %r11
	jmp	.L22
.L21:
	mov	 $1, %r11
.L22:
#LogicalOr End
	cmp	$0, %r11d
	je	.L20
#IF Start
#Equal Start
	mov	lookahead, %r11d
	cmp	STAR, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	je	.L23
	movl	STAR, %edi
	call	match

#Assignment Start
#Multiply Start
	call	factor
	mov	-4(%rbp), %r11d
	imul	%eax, %r11d
#Multiply End
	movl	%r11d, -4(%rbp)
#Assignment End

	jmp	.L24
.L23:
	movl	SLASH, %edi
	call	match

#Assignment Start
#Divide Start
	call	factor
	mov	%eax, -8(%rbp)	# spill
	mov	-4(%rbp), %eax
	movl	$0, %edx
	cltd
	idivl	-8(%rbp)
#Divide End
	movl	%eax, -4(%rbp)
#Assignment End

.L24:
#IF End

	jmp	.L19
.L20:
#While End

	mov	-4(%rbp), %eax
	jmp	.L18

.L18:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	term.size, 16
	.globl	term

expr:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$expr.size, %eax
	subq	%rax, %rsp
#Assignment Start
	call	term
	movl	%eax, -4(%rbp)
#Assignment End

#While Start
.L26:
#LogicalOr Start
#Equal Start
	mov	lookahead, %r11d
	cmp	PLUS, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	jne	.L28
#Equal Start
	mov	lookahead, %r11d
	cmp	MINUS, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	jne	.L28
	mov	 $0, %r11
	jmp	.L29
.L28:
	mov	 $1, %r11
.L29:
#LogicalOr End
	cmp	$0, %r11d
	je	.L27
#IF Start
#Equal Start
	mov	lookahead, %r11d
	cmp	PLUS, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	je	.L30
	movl	PLUS, %edi
	call	match

#Assignment Start
#Add start
	call	term
	mov	-4(%rbp), %r11d
	add	%eax, %r11d
#Add end
	movl	%r11d, -4(%rbp)
#Assignment End

	jmp	.L31
.L30:
	movl	MINUS, %edi
	call	match

#Assignment Start
#Subtract Start
	call	term
	mov	-4(%rbp), %r11d
	sub	%eax, %r11d
#Subtract End
	movl	%r11d, -4(%rbp)
#Assignment End

.L31:
#IF End

	jmp	.L26
.L27:
#While End

	mov	-4(%rbp), %eax
	jmp	.L25

.L25:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	expr.size, 16
	.globl	expr

main:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$main.size, %eax
	subq	%rax, %rsp
#Assignment Start
	mov	$256, %r11d
	movl	%r11d, NUM
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L33,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, NL
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L34,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, LPAREN
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L35,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, RPAREN
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L36,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, PLUS
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L37,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, MINUS
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L38,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, STAR
#Assignment End

#Assignment Start
#Cast Start
#Dereference Start
#Address Start
	leaq	.L39,%r11
#Address End
	movb	(%r11), %r11b
#Dereference End
	movsbl	%r11b, %r10d
#Cast End
	movl	%r10d, SLASH
#Assignment End

#Assignment Start
	call	lexan
	movl	%eax, lookahead
#Assignment End

#While Start
.L40:
#Not Equal Start
#Negate Start
	mov	$1, %r11d
	negl	%r11d
#Negate End
	mov	lookahead, %r10d
	cmp	%r11d, %r10d
	setne	%r10b
	movzbl	%r10b, %r10d
#Not Equal End
	cmp	$0, %r10d
	je	.L41
#Assignment Start
	call	expr
	movl	%eax, -4(%rbp)
#Assignment End

#Address Start
	leaq	.L42,%r11
#Address End
	mov	%r11, -12(%rbp)	# spill
	movl	-4(%rbp), %esi
	movq	-12(%rbp), %rdi
	movl	$0, %eax
	call	printf

#While Start
.L43:
#Equal Start
	mov	lookahead, %r11d
	cmp	NL, %r11d
	sete	%r11b
	movzbl	%r11b, %r11d
#Equal End
	cmp	$0, %r11d
	je	.L44
	movl	NL, %edi
	call	match
	jmp	.L43
.L44:
#While End

	jmp	.L40
.L41:
#While End

.L32:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	main.size, 16
	.globl	main

.L14:	.asciz	"syntax error at %d\n"
.L33:	.asciz	"\n"
.L34:	.asciz	"("
.L35:	.asciz	")"
.L36:	.asciz	"+"
.L37:	.asciz	"-"
.L38:	.asciz	"*"
.L39:	.asciz	"/"
.L42:	.asciz	"%d\n"
	.comm	NUM, 4
	.comm	NL, 4
	.comm	LPAREN, 4
	.comm	RPAREN, 4
	.comm	PLUS, 4
	.comm	MINUS, 4
	.comm	STAR, 4
	.comm	SLASH, 4
	.comm	lookahead, 4
	.comm	c, 4
	.comm	lexval, 4
