	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */

	pushal  /* return to ra */

	movl current_thread, %eax
	movl %esp, (%eax)
	movl next_thread, %eax
	movl (%eax), %esp

	movl %eax, current_thread
	popal

	movl $0, next_thread

	ret 
/*
스택 스위칭? -> thread_switch 

 current thread next thread을 변경. 
 
 어떤 레지스터를 넣을 지만 정하면 됨.

 커널에서 스케듈러를 동작시킬 껀 아님.

 스케듈러를 호출하도록 만들기만.
 */