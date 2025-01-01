
/root/fibonacci.o:     file format elf32-tradbigmips


Disassembly of section .text:

00000000 <main>:
   0:	27bdffe0 	addiu	sp,sp,-32
   4:	afbf001c 	sw	ra,28(sp)
   8:	afbe0018 	sw	s8,24(sp)
   c:	03a0f025 	move	s8,sp
  10:	2404000f 	li	a0,15
  14:	0c000000 	jal	0 <main>
  18:	00000000 	nop
  1c:	03c0e825 	move	sp,s8
  20:	8fbf001c 	lw	ra,28(sp)
  24:	8fbe0018 	lw	s8,24(sp)
  28:	27bd0020 	addiu	sp,sp,32
  2c:	03e00008 	jr	ra
  30:	00000000 	nop

00000034 <fibonacci>:
  34:	27bdffd8 	addiu	sp,sp,-40
  38:	afbf0024 	sw	ra,36(sp)
  3c:	afbe0020 	sw	s8,32(sp)
  40:	afb0001c 	sw	s0,28(sp)
  44:	03a0f025 	move	s8,sp
  48:	afc40028 	sw	a0,40(s8)
  4c:	8fc20028 	lw	v0,40(s8)
  50:	00000000 	nop
  54:	28420002 	slti	v0,v0,2
  58:	10400004 	beqz	v0,6c <fibonacci+0x38>
  5c:	00000000 	nop
  60:	8fc20028 	lw	v0,40(s8)
  64:	1000000f 	b	a4 <fibonacci+0x70>
  68:	00000000 	nop
  6c:	8fc20028 	lw	v0,40(s8)
  70:	00000000 	nop
  74:	2442ffff 	addiu	v0,v0,-1
  78:	00402025 	move	a0,v0
  7c:	0c000000 	jal	0 <main>
  80:	00000000 	nop
  84:	00408025 	move	s0,v0
  88:	8fc20028 	lw	v0,40(s8)
  8c:	00000000 	nop
  90:	2442fffe 	addiu	v0,v0,-2
  94:	00402025 	move	a0,v0
  98:	0c000000 	jal	0 <main>
  9c:	00000000 	nop
  a0:	02021021 	addu	v0,s0,v0
  a4:	03c0e825 	move	sp,s8
  a8:	8fbf0024 	lw	ra,36(sp)
  ac:	8fbe0020 	lw	s8,32(sp)
  b0:	8fb0001c 	lw	s0,28(sp)
  b4:	27bd0028 	addiu	sp,sp,40
  b8:	03e00008 	jr	ra
  bc:	00000000 	nop
