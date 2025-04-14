/*
트랩(Trap)**과 **인터럽트(Interrupt)**를 처리하는 핵심적인 코드가 포함 
이 파일은 CPU에서 발생하는 다양한 이벤트(시스템 호출, 하드웨어 인터럽트, 예외 등)를 처리하는 역할
*/
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "i8254.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256]; // 256개의 인터럽트 게이트를 저장하는 배열
// vecotrs.S에 정의된 256개의 인터럽트 핸들러의 주소를 저장하는 배열
// 각 핸들러는 특정 인터럽트 번호에 대응
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock; // ticks 변수에 대한 동시 접근을 제어하기 위한 spinlock
uint ticks; // 시스템 타이머 틱(tick) 카운터.

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
// 트랩(Trap) 및 인터럽트를 처리하는 메인 핸들러
// 트랩 프레임(struct trapframe)은 CPU 상태를 저장하고 복원
void
trap(struct trapframe *tf) ////////////////////////////////////////////////
// 사용자 프로세스에 여러 쓰레드가 있나?
// 타이머 인터럽트가 kernel 모드에서 발동을 했나?
{
  if(tf->trapno == T_SYSCALL){ // 시스템 호출을 처리하기 위해 syscall() 호출
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){  // struct trapframe의 멤버 변수 trapno
  case T_IRQ0 + IRQ_TIMER: // 요기서 타임 인터럽트가 발생
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks); // ticks를 기다리는 프로세스를 깨움
      release(&tickslock); // 동시접근 제어
    }
    lapiceoi();
    if (myproc() && myproc()->scheduler != 0 && ticks%10 == 0){ //
      // cprintf("\nchange!! invoke user start addr of scheduler function");
      myproc()->tf->eip = myproc()->scheduler; // 스케줄러 주소를 넣어줌
    }
    
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 0xB:
    i8254_intr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break; // 요 다음에 몰 넣어줘야함.
    // 스케듈러 주소 현재 프로세서에 얻어 올 수 있음. 부르면 죽음
    // 함수를 호출하려면 스택을 잘 쓰면 됨. 스택 주소를 잘 집어넣어라?
    // 쓰레드 creat 에 스택이 어떻게 나오는지 보면 됨.
    // current thread 라는 걸 쓰면 됨?

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING && // 타이머 인터렉트가 발생할 때 마다 스케듈러 동작 -> 요때 스케듈이 되도록 추가
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield(); // 컨텍스트 스위칭

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
