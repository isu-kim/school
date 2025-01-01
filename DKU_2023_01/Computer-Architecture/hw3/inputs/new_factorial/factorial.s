
/root/factorial.o:     file format elf32-tradbigmips


Disassembly of section .text:

00000000 <main>:
   0:	27bdffe0 	addiu	sp,sp,-32
   4:	afbf001c 	sw	ra,28(sp)
   8:	afbe0018 	sw	s8,24(sp)
   c:	03a0f025 	move	s8,sp
  10:	24040005 	li	a0,5
  14:	0c000000 	jal	0 <main>
  18:	00000000 	nop
  1c:	03c0e825 	move	sp,s8
  20:	8fbf001c 	lw	ra,28(sp)
  24:	8fbe0018 	lw	s8,24(sp)
  28:	27bd0020 	addiu	sp,sp,32
  2c:	03e00008 	jr	ra
  30:	00000000 	nop

00000034 <factorial>:
  34:	27bdffe0 	addiu	sp,sp,-32
  38:	afbf001c 	sw	ra,28(sp)
  3c:	afbe0018 	sw	s8,24(sp)
  40:	03a0f025 	move	s8,sp
  44:	afc40020 	sw	a0,32(s8)
  48:	8fc20020 	lw	v0,32(s8)
  4c:	00000000 	nop
  50:	10400005 	beqz	v0,68 <factorial+0x34>
  54:	00000000 	nop
  58:	8fc30020 	lw	v1,32(s8)
  5c:	24020001 	li	v0,1
  60:	14620004 	bne	v1,v0,74 <factorial+0x40>
  64:	00000000 	nop
  68:	24020001 	li	v0,1
  6c:	1000000c 	b	a0 <factorial+0x6c>
  70:	00000000 	nop
  74:	8fc20020 	lw	v0,32(s8)
  78:	00000000 	nop
  7c:	2442ffff 	addiu	v0,v0,-1
  80:	00402025 	move	a0,v0
  84:	0c000000 	jal	0 <main>
  88:	00000000 	nop
  8c:	00401825 	move	v1,v0
  90:	8fc20020 	lw	v0,32(s8)
  94:	00000000 	nop
  98:	00620018 	mult	v1,v0
  9c:	00001012 	mflo	v0
  a0:	03c0e825 	move	sp,s8
  a4:	8fbf001c 	lw	ra,28(sp)
  a8:	8fbe0018 	lw	s8,24(sp)
  ac:	27bd0020 	addiu	sp,sp,32
  b0:	03e00008 	jr	ra
  b4:	00000000 	nop
	...
