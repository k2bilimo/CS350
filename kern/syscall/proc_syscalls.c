#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <machine/trapframe.h>
#include <synch.h>
#include<wchan.h>
  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */

  bool nullTest = p->parent == NULL;
  
  if(!nullTest){ // change ourselves to terminated:
  for(int i = array_num(p->parent->childProcs) -1; i >=0; i--){
	struct skeleton *arrProc = (struct skeleton*) array_get(p->parent->childProcs, i);
	if(arrProc->pid == p->pid){ // same PID, same process, change to terminated, edit vals
	arrProc->terminated = true;
	arrProc->waitPidResult.exitStatus = __WEXITED;
	arrProc->waitPidResult.result = exitcode;
	wchan_wakeone(arrProc->procWchan);
	wchan_destroy(arrProc->procWchan);
	break;
  }
  }
  }
  else{
	// if we don't have a parent anymore, then we delete our PID.
	// We also check if other children have been terminated so we can free their PID.
	// point parent to NULL
	for(int i = array_num(p->childProcs) -1; i>=0; i--){
	struct skeleton *arrProc = (struct skeleton*) array_get(p->childProcs, i);
	if(arrProc->terminated){
		deletePid(arrProc->pid);
		arrProc->parent = NULL;
		kfree(arrProc);
		array_remove(p->childProcs,i);
	}
	}
	deletePid(p->pid);
  }
  
  	proc_destroy(p);
  // Wake up anyone on our CV.
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = curproc->pid;
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;
  bool pidFound = false;
  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */
// Cycle through all your process' children. See if the PID exists. If it doesn't then throw ESRCH
for(int i = array_num(curproc->childProcs)-1; i >= 0; i--){
	struct skeleton *arrProc = (struct skeleton *) array_get(curproc->childProcs, i);
	if(arrProc->pid == pid){
	pidFound = true;
	// found PID check if terminated or not. 
	if(arrProc->terminated){
	//if terminated receive the status codes. in arrProc->waitPidResult
	exitstatus = arrProc->waitPidResult.exitStatus;
	result = arrProc->waitPidResult.result;
	}
	else{
	//the process hasn't terminated, sleep on the child's CV/Lock
	wchan_lock(arrProc->procWchan);
	wchan_sleep(arrProc->procWchan);
	exitstatus = arrProc->waitPidResult.exitStatus;
	result = arrProc->waitPidResult.result;
	}
	}
}
  if(!pidFound){
	return ESRCH;
  }
  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

int sys_fork(pid_t *retval, struct trapframe *tf){
// need to pass in trapframe from parent process to child.
/* things to do for fork
1. Create process structure for the child process using proc_create_runprogram()
2. Create and copy the address space using as_copy()
3. Assign PID to child process, and create child process relationship. 
4. Create thread for the child process. 
4a. Copy a copy of the trapframe of the current process to the child process. 
4b. Use thread_fork to create the new thread. 
*/
	
	struct proc *parent = curproc;
	if(parent->pid == -1){
	createProcessPid(&(parent->pid));
	addProc(parent);
	}
// 1. Create the process structure for the child process using proc_create_runprogram()
	struct proc *child = proc_create_runprogram(curproc->p_name);
	if(child == NULL){
	// if we couldn't return a child process, there was not enough memory to declare one.
	return ENOMEM;
	}
//2. Create and copy the address space using as_copy()
	//as_copy takes two parameters : addrspace *old, and addrspace **ret
	//**ret is the return address space. copies *old into **ret.
	child->p_addrspace = as_create();
	if(child->p_addrspace == NULL){
	proc_destroy(child);
	return ENOMEM;
	}
	int as_copy_status = as_copy(curproc_getas(), &(child->p_addrspace));
	//  as_copy returns 0 if successful, otherwise it fails. returns ENOMEM
	if(as_copy_status == ENOMEM){
	proc_destroy(child);
	return ENOMEM;
	}

//3. Create the parent-child relationship, and also assign a PID to the Child.
	//First we add a PID to the child. 
	int val =  createProcessPid(&(child->pid));
	if(val == ENOMEM){
	proc_destroy(child);
	// we have too many processes running.
	return EMPROC;
	}
	// child's parent is current process. This is a pointer
	// whenever child exits, we have a backpointer to our parent to tell it we've terminated.
	child->parent = curproc;
	//Next we add the child onto the current process' array of children:
	//The parent receives a copy, not a reference to the child.
	//This way when the parent dies, the child remains intact. 
	val = addChild(curproc, child);
	// Now we copy the parent trapframe into a heap variable, pass it into the stack value in our proc structure.
	child->tf = kmalloc(sizeof(struct trapframe));
	if(child->tf == NULL){
	proc_destroy(child);
	return ENOMEM;
	}
	memcpy(child->tf, tf, sizeof(struct trapframe));
	// Call fork and run enter_forked_process function.
	addProc(child);
	val = thread_fork("Child Thread" ,child, &enter_forked_process, child->tf, 0);
	if(val){
	// non-zero value. return whatever error code it encounters
	proc_destroy(child);
	return val;
	}
	*retval = child->pid;
	return 0;
}
