#ifndef SYSCALL_H
#define SYSCALL_H

#ifdef _MSC_VER
#define access		_access
#define chmod		_chmod
#define chdir		_chdir
#define unlink		_unlink
#define open		_open
#define creat		_creat
#define pipe		_pipe
#define dup		_dup
#define dup2		_dup2
#define stat		_stat
#define fstat		_fstat
#define lseek		_lseek
#define read		_read
#define write		_write
#define close		_close
#define getpid		_getpid
#define utime		_utime
#include <sys/utime.h>
#endif /* _MSC_VER */


#define SS_SYS_Linux			4000
#define SS_SYS_syscall			(SS_SYS_Linux +   0)
#define SS_SYS_exit			(SS_SYS_Linux +   1)
#define SS_SYS_fork			(SS_SYS_Linux +   2)
#define SS_SYS_read			(SS_SYS_Linux +   3)
#define SS_SYS_write			(SS_SYS_Linux +   4)
#define SS_SYS_open			(SS_SYS_Linux +   5)
#define SS_SYS_close			(SS_SYS_Linux +   6)
#define SS_SYS_waitpid			(SS_SYS_Linux +   7)
#define SS_SYS_creat			(SS_SYS_Linux +   8)
#define SS_SYS_link			(SS_SYS_Linux +   9)
#define SS_SYS_unlink			(SS_SYS_Linux +  10)
#define SS_SYS_execve			(SS_SYS_Linux +  11)
#define SS_SYS_chdir			(SS_SYS_Linux +  12)
#define SS_SYS_time			(SS_SYS_Linux +  13)
#define SS_SYS_mknod			(SS_SYS_Linux +  14)
#define SS_SYS_chmod			(SS_SYS_Linux +  15)
#define SS_SYS_lchown			(SS_SYS_Linux +  16)
#define SS_SYS_break			(SS_SYS_Linux +  17)
#define SS_SYS_unused18			(SS_SYS_Linux +  18)
#define SS_SYS_lseek			(SS_SYS_Linux +  19)
#define SS_SYS_getpid			(SS_SYS_Linux +  20)
#define SS_SYS_mount			(SS_SYS_Linux +  21)
#define SS_SYS_umount			(SS_SYS_Linux +  22)
#define SS_SYS_setuid			(SS_SYS_Linux +  23)
#define SS_SYS_getuid			(SS_SYS_Linux +  24)
#define SS_SYS_stime			(SS_SYS_Linux +  25)
#define SS_SYS_ptrace			(SS_SYS_Linux +  26)
#define SS_SYS_alarm			(SS_SYS_Linux +  27)
#define SS_SYS_unused28			(SS_SYS_Linux +  28)
#define SS_SYS_pause			(SS_SYS_Linux +  29)
#define SS_SYS_utime			(SS_SYS_Linux +  30)
#define SS_SYS_stty			(SS_SYS_Linux +  31)
#define SS_SYS_gtty			(SS_SYS_Linux +  32)
#define SS_SYS_access			(SS_SYS_Linux +  33)
#define SS_SYS_nice			(SS_SYS_Linux +  34)
#define SS_SYS_ftime			(SS_SYS_Linux +  35)
#define SS_SYS_sync			(SS_SYS_Linux +  36)
#define SS_SYS_kill			(SS_SYS_Linux +  37)
#define SS_SYS_rename			(SS_SYS_Linux +  38)
#define SS_SYS_mkdir			(SS_SYS_Linux +  39)
#define SS_SYS_rmdir			(SS_SYS_Linux +  40)
#define SS_SYS_dup			(SS_SYS_Linux +  41)
#define SS_SYS_pipe			(SS_SYS_Linux +  42)
#define SS_SYS_times			(SS_SYS_Linux +  43)
#define SS_SYS_prof			(SS_SYS_Linux +  44)
#define SS_SYS_brk			(SS_SYS_Linux +  45)
#define SS_SYS_setgid			(SS_SYS_Linux +  46)
#define SS_SYS_getgid			(SS_SYS_Linux +  47)
#define SS_SYS_signal			(SS_SYS_Linux +  48)
#define SS_SYS_geteuid			(SS_SYS_Linux +  49)
#define SS_SYS_getegid			(SS_SYS_Linux +  50)
#define SS_SYS_acct			(SS_SYS_Linux +  51)
#define SS_SYS_umount2			(SS_SYS_Linux +  52)
#define SS_SYS_lock			(SS_SYS_Linux +  53)
#define SS_SYS_ioctl			(SS_SYS_Linux +  54)
#define SS_SYS_fcntl			(SS_SYS_Linux +  55)
#define SS_SYS_mpx			(SS_SYS_Linux +  56)
#define SS_SYS_setpgid			(SS_SYS_Linux +  57)
#define SS_SYS_ulimit			(SS_SYS_Linux +  58)
#define SS_SYS_unused59			(SS_SYS_Linux +  59)
#define SS_SYS_umask			(SS_SYS_Linux +  60)
#define SS_SYS_chroot			(SS_SYS_Linux +  61)
#define SS_SYS_ustat			(SS_SYS_Linux +  62)
#define SS_SYS_dup2			(SS_SYS_Linux +  63)
#define SS_SYS_getppid			(SS_SYS_Linux +  64)
#define SS_SYS_getpgrp			(SS_SYS_Linux +  65)
#define SS_SYS_setsid			(SS_SYS_Linux +  66)
#define SS_SYS_sigaction			(SS_SYS_Linux +  67)
#define SS_SYS_sgetmask			(SS_SYS_Linux +  68)
#define SS_SYS_ssetmask			(SS_SYS_Linux +  69)
#define SS_SYS_setreuid			(SS_SYS_Linux +  70)
#define SS_SYS_setregid			(SS_SYS_Linux +  71)
#define SS_SYS_sigsuspend			(SS_SYS_Linux +  72)
#define SS_SYS_sigpending			(SS_SYS_Linux +  73)
#define SS_SYS_sethostname		(SS_SYS_Linux +  74)
#define SS_SYS_setrlimit			(SS_SYS_Linux +  75)
#define SS_SYS_getrlimit			(SS_SYS_Linux +  76)
#define SS_SYS_getrusage			(SS_SYS_Linux +  77)
#define SS_SYS_gettimeofday		(SS_SYS_Linux +  78)
#define SS_SYS_settimeofday		(SS_SYS_Linux +  79)
#define SS_SYS_getgroups			(SS_SYS_Linux +  80)
#define SS_SYS_setgroups			(SS_SYS_Linux +  81)
#define SS_SYS_reserved82			(SS_SYS_Linux +  82)
#define SS_SYS_symlink			(SS_SYS_Linux +  83)
#define SS_SYS_unused84			(SS_SYS_Linux +  84)
#define SS_SYS_readlink			(SS_SYS_Linux +  85)
#define SS_SYS_uselib			(SS_SYS_Linux +  86)
#define SS_SYS_swapon			(SS_SYS_Linux +  87)
#define SS_SYS_reboot			(SS_SYS_Linux +  88)
#define SS_SYS_readdir			(SS_SYS_Linux +  89)
#define SS_SYS_mmap			(SS_SYS_Linux +  90)
#define SS_SYS_munmap			(SS_SYS_Linux +  91)
#define SS_SYS_truncate			(SS_SYS_Linux +  92)
#define SS_SYS_ftruncate			(SS_SYS_Linux +  93)
#define SS_SYS_fchmod			(SS_SYS_Linux +  94)
#define SS_SYS_fchown			(SS_SYS_Linux +  95)
#define SS_SYS_getpriority		(SS_SYS_Linux +  96)
#define SS_SYS_setpriority		(SS_SYS_Linux +  97)
#define SS_SYS_profil			(SS_SYS_Linux +  98)
#define SS_SYS_statfs			(SS_SYS_Linux +  99)
#define SS_SYS_fstatfs			(SS_SYS_Linux + 100)
#define SS_SYS_ioperm			(SS_SYS_Linux + 101)
#define SS_SYS_socketcall			(SS_SYS_Linux + 102)
#define SS_SYS_syslog			(SS_SYS_Linux + 103)
#define SS_SYS_setitimer			(SS_SYS_Linux + 104)
#define SS_SYS_getitimer			(SS_SYS_Linux + 105)
#define SS_SYS_stat			(SS_SYS_Linux + 106)
#define SS_SYS_lstat			(SS_SYS_Linux + 107)
#define SS_SYS_fstat			(SS_SYS_Linux + 108)
#define SS_SYS_unused109			(SS_SYS_Linux + 109)
#define SS_SYS_iopl			(SS_SYS_Linux + 110)
#define SS_SYS_vhangup			(SS_SYS_Linux + 111)
#define SS_SYS_idle			(SS_SYS_Linux + 112)
#define SS_SYS_vm86			(SS_SYS_Linux + 113)
#define SS_SYS_wait4			(SS_SYS_Linux + 114)
#define SS_SYS_swapoff			(SS_SYS_Linux + 115)
#define SS_SYS_sysinfo			(SS_SYS_Linux + 116)
#define SS_SYS_ipc			(SS_SYS_Linux + 117)
#define SS_SYS_fsync			(SS_SYS_Linux + 118)
#define SS_SYS_sigreturn			(SS_SYS_Linux + 119)
#define SS_SYS_clone			(SS_SYS_Linux + 120)
#define SS_SYS_setdomainname		(SS_SYS_Linux + 121)
#define SS_SYS_uname			(SS_SYS_Linux + 122)
#define SS_SYS_modify_ldt			(SS_SYS_Linux + 123)
#define SS_SYS_adjtimex			(SS_SYS_Linux + 124)
#define SS_SYS_mprotect			(SS_SYS_Linux + 125)
#define SS_SYS_sigprocmask		(SS_SYS_Linux + 126)
#define SS_SYS_create_module		(SS_SYS_Linux + 127)
#define SS_SYS_init_module		(SS_SYS_Linux + 128)
#define SS_SYS_delete_module		(SS_SYS_Linux + 129)
#define SS_SYS_get_kernel_syms		(SS_SYS_Linux + 130)
#define SS_SYS_quotactl			(SS_SYS_Linux + 131)
#define SS_SYS_getpgid			(SS_SYS_Linux + 132)
#define SS_SYS_fchdir			(SS_SYS_Linux + 133)
#define SS_SYS_bdflush			(SS_SYS_Linux + 134)
#define SS_SYS_sysfs			(SS_SYS_Linux + 135)
#define SS_SYS_personality		(SS_SYS_Linux + 136)
#define SS_SYS_afs_syscall		(SS_SYS_Linux + 137) /* Syscall for Andrew File System */
#define SS_SYS_setfsuid			(SS_SYS_Linux + 138)
#define SS_SYS_setfsgid			(SS_SYS_Linux + 139)
#define SS_SYS_llseek			(SS_SYS_Linux + 140)
#define SS_SYS_getdents			(SS_SYS_Linux + 141)
#define SS_SYS_newselect			(SS_SYS_Linux + 142)
#define SS_SYS_flock			(SS_SYS_Linux + 143)
#define SS_SYS_msync			(SS_SYS_Linux + 144)
#define SS_SYS_readv			(SS_SYS_Linux + 145)
#define SS_SYS_writev			(SS_SYS_Linux + 146)
#define SS_SYS_cacheflush			(SS_SYS_Linux + 147)
#define SS_SYS_cachectl			(SS_SYS_Linux + 148)
#define SS_SYS_sysmips			(SS_SYS_Linux + 149)
#define SS_SYS_unused150			(SS_SYS_Linux + 150)
#define SS_SYS_getsid			(SS_SYS_Linux + 151)
#define SS_SYS_fdatasync			(SS_SYS_Linux + 152)
#define SS_SYS_sysctl			(SS_SYS_Linux + 153)
#define SS_SYS_mlock			(SS_SYS_Linux + 154)
#define SS_SYS_munlock			(SS_SYS_Linux + 155)
#define SS_SYS_mlockall			(SS_SYS_Linux + 156)
#define SS_SYS_munlockall			(SS_SYS_Linux + 157)
#define SS_SYS_sched_setparam		(SS_SYS_Linux + 158)
#define SS_SYS_sched_getparam		(SS_SYS_Linux + 159)
#define SS_SYS_sched_setscheduler		(SS_SYS_Linux + 160)
#define SS_SYS_sched_getscheduler		(SS_SYS_Linux + 161)
#define SS_SYS_sched_yield		(SS_SYS_Linux + 162)
#define SS_SYS_sched_get_priority_max	(SS_SYS_Linux + 163)
#define SS_SYS_sched_get_priority_min	(SS_SYS_Linux + 164)
#define SS_SYS_sched_rr_get_interval	(SS_SYS_Linux + 165)
#define SS_SYS_nanosleep			(SS_SYS_Linux + 166)
#define SS_SYS_mremap			(SS_SYS_Linux + 167)
#define SS_SYS_accept			(SS_SYS_Linux + 168)
#define SS_SYS_bind			(SS_SYS_Linux + 169)
#define SS_SYS_connect			(SS_SYS_Linux + 170)
#define SS_SYS_getpeername		(SS_SYS_Linux + 171)
#define SS_SYS_getsockname		(SS_SYS_Linux + 172)
#define SS_SYS_getsockopt			(SS_SYS_Linux + 173)
#define SS_SYS_listen			(SS_SYS_Linux + 174)
#define SS_SYS_recv			(SS_SYS_Linux + 175)
#define SS_SYS_recvfrom			(SS_SYS_Linux + 176)
#define SS_SYS_recvmsg			(SS_SYS_Linux + 177)
#define SS_SYS_send			(SS_SYS_Linux + 178)
#define SS_SYS_sendmsg			(SS_SYS_Linux + 179)
#define SS_SYS_sendto			(SS_SYS_Linux + 180)
#define SS_SYS_setsockopt			(SS_SYS_Linux + 181)
#define SS_SYS_shutdown			(SS_SYS_Linux + 182)
#define SS_SYS_socket			(SS_SYS_Linux + 183)
#define SS_SYS_socketpair			(SS_SYS_Linux + 184)
#define SS_SYS_setresuid			(SS_SYS_Linux + 185)
#define SS_SYS_getresuid			(SS_SYS_Linux + 186)
#define SS_SYS_query_module		(SS_SYS_Linux + 187)
#define SS_SYS_poll			(SS_SYS_Linux + 188)
#define SS_SYS_nfsservctl			(SS_SYS_Linux + 189)
#define SS_SYS_setresgid			(SS_SYS_Linux + 190)
#define SS_SYS_getresgid			(SS_SYS_Linux + 191)
#define SS_SYS_prctl			(SS_SYS_Linux + 192)
#define SS_SYS_rt_sigreturn		(SS_SYS_Linux + 193)
#define SS_SYS_rt_sigaction		(SS_SYS_Linux + 194)
#define SS_SYS_rt_sigprocmask		(SS_SYS_Linux + 195)
#define SS_SYS_rt_sigpending		(SS_SYS_Linux + 196)
#define SS_SYS_rt_sigtimedwait		(SS_SYS_Linux + 197)
#define SS_SYS_rt_sigqueueinfo		(SS_SYS_Linux + 198)
#define SS_SYS_rt_sigsuspend		(SS_SYS_Linux + 199)
#define SS_SYS_pread64			(SS_SYS_Linux + 200)
#define SS_SYS_pwrite64			(SS_SYS_Linux + 201)
#define SS_SYS_chown			(SS_SYS_Linux + 202)
#define SS_SYS_getcwd			(SS_SYS_Linux + 203)
#define SS_SYS_capget			(SS_SYS_Linux + 204)
#define SS_SYS_capset			(SS_SYS_Linux + 205)
#define SS_SYS_sigaltstack		(SS_SYS_Linux + 206)
#define SS_SYS_sendfile			(SS_SYS_Linux + 207)
#define SS_SYS_getpmsg			(SS_SYS_Linux + 208)
#define SS_SYS_putpmsg			(SS_SYS_Linux + 209)
#define SS_SYS_mmap2			(SS_SYS_Linux + 210)
#define SS_SYS_truncate64			(SS_SYS_Linux + 211)
#define SS_SYS_ftruncate64		(SS_SYS_Linux + 212)
#define SS_SYS_stat64			(SS_SYS_Linux + 213)
#define SS_SYS_lstat64			(SS_SYS_Linux + 214)
#define SS_SYS_fstat64			(SS_SYS_Linux + 215)
#define SS_SYS_pivot_root			(SS_SYS_Linux + 216)
#define SS_SYS_mincore			(SS_SYS_Linux + 217)
#define SS_SYS_madvise			(SS_SYS_Linux + 218)
#define SS_SYS_getdents64			(SS_SYS_Linux + 219)
#define SS_SYS_fcntl64			(SS_SYS_Linux + 220)
#define SS_SYS_reserved221		(SS_SYS_Linux + 221)
#define SS_SYS_gettid			(SS_SYS_Linux + 222)
#define SS_SYS_readahead			(SS_SYS_Linux + 223)
#define SS_SYS_setxattr			(SS_SYS_Linux + 224)
#define SS_SYS_lsetxattr			(SS_SYS_Linux + 225)
#define SS_SYS_fsetxattr			(SS_SYS_Linux + 226)
#define SS_SYS_getxattr			(SS_SYS_Linux + 227)
#define SS_SYS_lgetxattr			(SS_SYS_Linux + 228)
#define SS_SYS_fgetxattr			(SS_SYS_Linux + 229)
#define SS_SYS_listxattr			(SS_SYS_Linux + 230)
#define SS_SYS_llistxattr			(SS_SYS_Linux + 231)
#define SS_SYS_flistxattr			(SS_SYS_Linux + 232)
#define SS_SYS_removexattr		(SS_SYS_Linux + 233)
#define SS_SYS_lremovexattr		(SS_SYS_Linux + 234)
#define SS_SYS_fremovexattr		(SS_SYS_Linux + 235)
#define SS_SYS_tkill			(SS_SYS_Linux + 236)
#define SS_SYS_sendfile64			(SS_SYS_Linux + 237)
#define SS_SYS_futex			(SS_SYS_Linux + 238)
#define SS_SYS_sched_setaffinity		(SS_SYS_Linux + 239)
#define SS_SYS_sched_getaffinity		(SS_SYS_Linux + 240)
#define SS_SYS_io_setup			(SS_SYS_Linux + 241)
#define SS_SYS_io_destroy			(SS_SYS_Linux + 242)
#define SS_SYS_io_getevents		(SS_SYS_Linux + 243)
#define SS_SYS_io_submit			(SS_SYS_Linux + 244)
#define SS_SYS_io_cancel			(SS_SYS_Linux + 245)
#define SS_SYS_exit_group			(SS_SYS_Linux + 246)
#define SS_SYS_lookup_dcookie		(SS_SYS_Linux + 247)
#define SS_SYS_epoll_create		(SS_SYS_Linux + 248)
#define SS_SYS_epoll_ctl			(SS_SYS_Linux + 249)
#define SS_SYS_epoll_wait			(SS_SYS_Linux + 250)
#define SS_SYS_remap_file_pages		(SS_SYS_Linux + 251)
#define SS_SYS_set_tid_address		(SS_SYS_Linux + 252)
#define SS_SYS_restart_syscall		(SS_SYS_Linux + 253)
#define SS_SYS_fadvise64			(SS_SYS_Linux + 254)
#define SS_SYS_statfs64			(SS_SYS_Linux + 255)
#define SS_SYS_fstatfs64			(SS_SYS_Linux + 256)
#define SS_SYS_timer_create		(SS_SYS_Linux + 257)
#define SS_SYS_timer_settime		(SS_SYS_Linux + 258)
#define SS_SYS_timer_gettime		(SS_SYS_Linux + 259)
#define SS_SYS_timer_getoverrun		(SS_SYS_Linux + 260)
#define SS_SYS_timer_delete		(SS_SYS_Linux + 261)
#define SS_SYS_clock_settime		(SS_SYS_Linux + 262)
#define SS_SYS_clock_gettime		(SS_SYS_Linux + 263)
#define SS_SYS_clock_getres		(SS_SYS_Linux + 264)
#define SS_SYS_clock_nanosleep		(SS_SYS_Linux + 265)
#define SS_SYS_tgkill			(SS_SYS_Linux + 266)
#define SS_SYS_utimes			(SS_SYS_Linux + 267)
#define SS_SYS_mbind			(SS_SYS_Linux + 268)
#define SS_SYS_get_mempolicy		(SS_SYS_Linux + 269)
#define SS_SYS_set_mempolicy		(SS_SYS_Linux + 270)
#define SS_SYS_mq_open			(SS_SYS_Linux + 271)
#define SS_SYS_mq_unlink			(SS_SYS_Linux + 272)
#define SS_SYS_mq_timedsend		(SS_SYS_Linux + 273)
#define SS_SYS_mq_timedreceive		(SS_SYS_Linux + 274)
#define SS_SYS_mq_notify			(SS_SYS_Linux + 275)
#define SS_SYS_mq_getsetattr		(SS_SYS_Linux + 276)
#define SS_SYS_vserver			(SS_SYS_Linux + 277)
#define SS_SYS_waitid			(SS_SYS_Linux + 278)
/* #define SS_SYS_sys_setaltroot		(SS_SYS_Linux + 279) */
#define SS_SYS_add_key			(SS_SYS_Linux + 280)
#define SS_SYS_request_key		(SS_SYS_Linux + 281)
#define SS_SYS_keyctl			(SS_SYS_Linux + 282)
#define SS_SYS_set_thread_area		(SS_SYS_Linux + 283)
#define SS_SYS_runrisc2		(SS_SYS_Linux + 284)//    yaoyp_double_risc
#define SS_SYS_cpyrisc2       (SS_SYS_Linux + 285)
#define SS_SYS_isreadyrisc2   (SS_SYS_Linux + 286)
#define SS_SYS_inotify_init		(SS_SYS_Linux + 287)//
#define SS_SYS_inotify_add_watch		(SS_SYS_Linux + 288)//
#define SS_SYS_inotify_rm_watch		(SS_SYS_Linux + 289)//

/* Used as sync among threads*/
/* by wxh*/
#define OSF_SYS_fork        2

#define OSF_SYS_getthreadid 500
#define OSF_SYS_printf 516

#define OSF_SYS_barrier 518

#define OSF_SYS_WAIT 521

#define OSF_SYS_STATS 530
/* Tells the simulator when the program enters and exits the synchronization mode */
#define OSF_SYS_BARRIER_STATS 531
#define OSF_SYS_LOCK_STATS 532		

#define OSF_SYS_BARRIER_INSTR 533
#define OSF_SYS_QUEISCE 534

#define OSF_SYS_SHDADDR 554


/* SStrix ioctl values */
#define SS_IOCTL_TIOCGETP	1074164744
#define SS_IOCTL_TIOCSETP	-2147060727
#define SS_IOCTL_TCGETP		1076130901
#define SS_IOCTL_TCGETA		1075082331
#define SS_IOCTL_TIOCGLTC	1074164852
#define SS_IOCTL_TIOCSLTC	-2147060619
#define SS_IOCTL_TIOCGWINSZ	1074295912
#define SS_IOCTL_TCSETAW	-2146143143
#define SS_IOCTL_TIOCGETC	1074164754
#define SS_IOCTL_TIOCSETC	-2147060719
#define SS_IOCTL_TIOCLBIC	-2147191682
#define SS_IOCTL_TIOCLBIS	-2147191681
#define SS_IOCTL_TIOCLGET	0x4004747c
#define SS_IOCTL_TIOCLSET	-2147191683

/* internal system call buffer size, used primarily for file name arguments,
   argument larger than this will be truncated */
#define MAXBUFSIZE 		1024

/* total bytes to copy from a valid pointer argument for ioctl() calls,
   syscall.c does not decode ioctl() calls to determine the size of the
   arguments that reside in memory, instead, the ioctl() proxy simply copies
   NUM_IOCTL_BYTES bytes from the pointer argument to host memory */
#define NUM_IOCTL_BYTES		128

extern int barrier_waiting[MAXTHREADS];
#endif