#include "types.h"
#include "stat.h"
#include "user.h"
// 현재 수행하는 위치는 왜 저장을 안하나?
// 스위칭 하면 위치를 바꿔줘야 하는데?
// 스택에 return 값이 있음. 스택이 저장을 함. 따로 저장 안해도 됨
// 
/* Possible states of a thread; */ // 쓰레드의 가능한 상태 정의
#define FREE        0x0 // wait가 없다.
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

typedef struct thread thread_t, *thread_p;
typedef struct mutex mutex_t, *mutex_p;

// 
struct thread { // 쓰레드를 구조체로 정의 
  int        sp;                /* 저장된 스택 포인터 */ // 요 포인터로 처음에 가면 모가 있단다. 얘가 sp -> trap,c 에서 하는건가?
  char stack[STACK_SIZE];       /* 쓰레드 스택 */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};

static thread_t all_thread[MAX_THREAD]; // 모든 Thread
thread_p  current_thread; // 현재 실행중인 쓰레드                          넥스트 쓰레드를 스택 포인터에 로드
thread_p  next_thread; // 다음에 실행할 쓰레드
extern void thread_switch(void); // 다음에 실행 가능한 쓰레드를 찾아서 필요에 따라 전환
                                 // 실행 가능한 쓰레드가 없으면 오류 출력

static void  // 얘는 좀 바꿔도 됨. 다른 건 안됨
thread_schedule(void) // 실행 가능한 스레드를 찾아 필요에 따라 전환
                      // 실행 가능한 쓰레드가 없으면 오류 출력
{
  thread_p t;

  /* Find another runnable thread. */
  next_thread = 0; // 쭉 뒤져서 현재 쓰레드 다음거 max_thread -> 스택 스위칭 하면 끝!
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }

  if (t >= all_thread + MAX_THREAD && current_thread->state == RUNNABLE) {
    /* The current thread is the only runnable thread; run it. */
    next_thread = current_thread;
  }

  if (next_thread == 0) {
    printf(2, "thread_schedule: no runnable threads\n");
    exit();
  }

  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    // 현재 쓰레드가 메인 쓰레드가 아니고 RUNNING 상태라면 RUNNABLE로 변경하여 스케듈러가 나중에 실행 할 수 있도록 준비
    if (current_thread != &all_thread[0] && current_thread->state == RUNNING) 
      current_thread->state = RUNNABLE;
    thread_switch();
  } else
    next_thread = 0;
}

void 
thread_init(void) // 쓰레딩 시스템 초기화, 메인 쓰레드를 현재 쓰레드로 설정하고 RUNNING 상태로 표시
{
  uthread_init((int)thread_schedule); // system call

  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
  uthread_init((int)thread_schedule); // system call
}

// 요놈은 분석을 좀 해야함
void 
thread_create(void (*func)()) // 새 스레드를 생성. 빈 스레드 슬롯을 찾아 스택을 설정하고 RUNNABLE 상태로 표시
{
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break; // 스택을 초기화?
  }
  t->sp = (int) (t->stack + STACK_SIZE);   // set sp to the top of the stack
  t->sp -= 4;                              // space for return address
  * (int *) (t->sp) = (int)func;           // push return address on stack
  t->sp -= 32;                             // space for registers that thread_switch expects
  t->state = RUNNABLE;
}

static void 
mythread(void) // 메세지를 출력하고 자신을 Free 상태로 표시하는 예제 쓰레드 함수
{
  int i;
  printf(1, "my thread running\n");
  for (i = 0; i < 100; i++) { // 100 번 돌면서 출력
    printf(1, "my thread 0x%x\n", (int) current_thread);
  }
  printf(1, "my thread: exit\n");
  current_thread->state = FREE;
  thread_schedule(); // 스케줄러를 호출하여 다음 실행할 쓰레드를 찾음
} // mythread를 실행하다가 스케줄러가 호출되면 스위칭 -> 스택만 왔다갔다. ->> 요거 재미 없음. 바꿈.
// 커널에서 schedule을 부르도록 합시다.


int // main 함수로 쓰레딩 시스템을초기화 하고 mythread를 실행하는 2개의 쓰레드를 생성하며 scheduler를 시작
main(int argc, char *argv[]) 
{
  thread_init();
  thread_create(mythread);
  thread_create(mythread);
  thread_schedule();
  return 0;
}

/*
실행 흐름
1. 초기화:
  thread_init()이 호출되어 스레딩 시스템을 설정.
  메인 쓰레드는 현재 스레드로 설정되고 RUNNING 상태로 표시 됨.

2. 쓰레드 생성
  thread_create(mythread)이 두 번 호출되어 mythread를 실행하는 두 개의 스레드를 생성

3. 스케줄링:
  thread_schedule()이 호출되어 쓰레드 스케줄러를 시작. 이는 생성된 쓰레드 간 전환을 수행

스레드 전환
  실제 쓰레드 전환은 이 코드에서 정의되지 않은 thread_switch() 함수에 의해 처리되며,
  이는 현재 쓰레드에서 다음 쓰레드로 CPU 컨텍스트를 전환
*/
