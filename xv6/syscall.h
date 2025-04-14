// System call numbers 이름에 다 rule이 있음, 시스템 호출을 추가할 거임
// 시스템 호출 번호 앞에 놓으면 다 망함.
// _ 이 약속임. 안 넣으면 안돌아감.
// 여기다가 시스템 호출 추가. 함수 이름은 user.h와 defs.h에 정의해야함?
// 왜 2군데냐?
// kernel과 유저는 address 주소가 다름
// ex 유저가 wait를 부르면 wait는 커널에 있음. 실제로 userlevel에는 wait가 없다.-> proc.c
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13 
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
#define SYS_uthread_init 22 // 