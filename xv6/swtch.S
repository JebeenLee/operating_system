# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating 현재 레지스터를 스택에 저장하여
# a struct context, and save its address in *old. 구조체 컨텍스트를 생성하고, 해당 주소를 *old에 저장
# Switch stacks to new and pop previously-saved registers. 스택을 new로 전환하고 이전에 저장된 레지스터를 팝합니다.

.globl swtch
swtch:
  movl 4(%esp), %eax
  movl 8(%esp), %edx

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)
  movl %edx, %esp

  # Load new callee-save registers a c d 는 여기서 저장을 안함, 저장을 할 필요가 없다. 함수를 부르는 애가 저장을 한다. -> proc.h에 나와있음
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret
