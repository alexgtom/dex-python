/*
 *-----------------------------------------------------------------------*
 * NOTICE:  This code was developed by the US Government.  The original
 * versions, REX 1.0-3.12, were developed for the pdp11 architecture and
 * distributed without restrictions.  This version, REX 4.0, is a port of
 * the original version to the Intel 80x86 architecture.  This version is
 * distributed only under license agreement from the National Institutes 
 * of Health, Laboratory of Sensorimotor Research, Bldg 10 Rm 10C101, 
 * 9000 Rockville Pike, Bethesda, MD, 20892, (301) 496-9375.
 *-----------------------------------------------------------------------*
 */
#ifndef __PROC_H__
#define __PROC_H__

/*
 *	Process management and interprocess communication.  If anything
 * is changed here, the entire REX system needs to be recompiled.
 */

#define P_NPROC		15	/* max number of processes */
#define P_NMENU		150	/* max number of menus, entire REX system */
#define P_NNOUN		150	/* max number of nouns, entire REX system */
#define P_LNAME		20	/* max length of a noun, menu name; also
				   used as max length of a string variable
				   in menu system */
#define P_LFNAME	14	/* max length of A, E file names */
#define P_LVERSION	20	/* max length of version number string */
#define P_NARG		20	/* max num of args in command line */
#define P_LPROCNAME	14	/* length of ascii name of process */
#define P_ISLEN		100	/* interprocess string length */
#define P_MSDEL		500	/* delay (in msec) to spin waiting for a
				   semaphore to free */
#define P_MSGMAX	15	/* max num of messages (bits in an int) */
#define P_EXWAIT	5	/* number of seconds to wait for process to
				   terminate after sending kill message */

/*
 *	Signals.  Unix signals are not adequate for REX interprocess
 * communication.  Shared memory and a semaphore mechanism is used
 * instead.  Signal 16 interrupts processes to processes messages.
 * It has been changed in the kernel to not require resetting to
 * avoid problem of receiving another signal before previous one has
 * been reset.  With REX versions 2.2 and higher, however, signal
 * sending is interlocked in such a way that this is no longer necessary.
 */
#define S_CNTRLC	SIGINT	/* ignored by all processes */
#define S_CNTRLB	SIGQUIT	/* causes all process to reset inter-
				   process communication and return to
				   quiescent state */
#define S_ALERT		SIGUSR1	/* interprocess communication signal */
#define S_DEBUG_PRINT	SIGUSR2	/* to print debugging info */
#define S_STATESIG	SIGEMT	/* to trigger state processor every
				   msec;  SIGEMT is not used by QNX */

typedef struct nametbl NAME;

/*
 *	Process structure.  One instance of this struct is maintained
 * by comm in the INT_BLOCK for each REX process.
 */
typedef struct {
	int	p_id;		/* process id;  when 0 struct is free */
	int	p_state;	/* state of process (running, stopped, etc.) */
	u_int	p_sem;		/* semaphore used by a tst/set operation to
				   synchronize interprocess signalling */
	u_int	p_msg;		/* messages */
	u_int	p_rmask;	/* re-interrupt mask for messages */
	NAME	*p_nounp;	/* pointer to noun list for this process
				   in comm's address space */
	NAME	*p_menup;	/* pointer to menu list */
	int	p_vindex;	/* holds a verb, noun pair:  high order byte
				   is first char of verb, low order byte is
				   index to noun table.  Verb, noun pair is
				   being sent if high order byte is nonzero */
	int	p_pipe[2];	/* pipe file descriptors */
	char	p_name[P_LPROCNAME];	/* ascii name */
	char	p_version[P_LVERSION];	/* ascii version specification */
} PROCTBL;
#ifdef NEED_FAR
typedef PROCTBL far * PROCTBL_P;
#else
typedef PROCTBL * PROCTBL_P;
#endif
extern PROCTBL_P myptp;		/* externs used by all REX processes */
extern int myptx;		/* my ptbl index */

/*
 * Struct to hold names of processes' nouns and menus.  This table is in
 * comm's address space only.
 * This is typedef'ed to NAME in statement earlier.
 */
struct nametbl  /* typedef: NAME */  {
	PROCTBL_P na_pp;	/* proctbl pointer for this name's process;
				   when na_pp == NULLP struct is free */
	NAME	*na_nextn;	/* pointer to next name for this process */
	int	na_tblx;	/* index of name in owner process' table */
	char	na_name[P_LNAME];
};

/*
 * Noun table of each process.
 */
typedef struct {
	char	n_name[P_LNAME];	/* name of noun */
	int (*n_ptr)();			/* address of processing routine for
					   this noun */
}NOUN;
extern NOUN nouns[];

/*
 *	p_state defines.
 */
#define P_RUN_ST	01	/* process is running */
#define P_NOSIG_ST	02	/* interlocks signal sending;  when set S_ALERT
				   has been sent to process, further S_ALERTs
				   are not needed */
#define P_ALRTBSY_ST	04	/* when set alert routine protected against
				   being re-entered */
#define P_EARLYWAKE_ST	010	/* set when processing a noun, verb if comm
				   is awakened before processing is completed;
				   might be done by noun, verbs that require
				   no tty interaction */
#define P_EXIT_ST	020	/* set by process when exiting after receiving
				   kill message;  signifies that process has
				   terminated */
#define P_INTPROC_ST	040	/* process is an interrupt process */

/*
 *	Comm system wide flags kept in i_b->c_flags.
 */
#define C_ASLEEP	01	/* comm is asleep */

/*
 *	Int system wide flags kept in i_b->i_flags.
 */
#define I_GO		01	/* clock is running */
#define I_FILEOPEN	02	/* data file is open */
#define I_NEWSAMPHDR	04	/* request to put new samp hdr in data file */
#define I_EOUT		010	/* efile data saving enabled */
#define I_AOUT		020	/* afile data saving enabled */
#define I_WINDOPEN	040	/* analog data window currently open */
#define I_REVACTIVE	0100	/* remote event waiting or in process of
				   being loaded */
#define I_FORCECLOSE	0200	/* set by scribe on error to force closure
				   of an analog data window */

/*
 *	Display system wide flags kept in i_b->d_flags.
 */
#define D_RLINE		01	/* current display is running line
				   (generated by int process) */
#define D_WIND		02	/* current display is eye position
				   window (generated by int and display */
#define D_WMOV		04	/* window display is moving instead of
				   stationary */
#define D_REWIND	010	/* set when window display started (or
				   restarted) to stop invalid old cursors
				   from being erased */
#define D_RASTH		020	/* current display is raster, histogram */
#define D_HISTON	040	/* histograms enabled */
#define D_HSIG		0100	/* signalling from int for histogram updating
				   enabled */
#define D_RHDRAW	0200	/* when set causes rast, hist display to be
				   completely drawn, assumes screen is blank */
#define D_SCACT		(D_RLINE|D_WIND|D_RASTH)	/* screen is active */
#define D_RHACT		(D_RASTH|D_HSIG)		/* rast, hist is act */
#define D_ALLACT	(D_SCACT|D_RHACT|D_REWIND|D_RHDRAW)

/*
 * Root control flags; kept in i_b->i_rtflag.
 */
#define RT_CLOSE	01	/* terminate current root read */
#define RT_RNEST	02	/* set with RT_CLOSE when close preceeds a
				   nested read */
#define RT_READ		04	/* read root file */
#define RT_ECHO		010	/* set with RT_READ when root file is to 
				   be echoed on tty */
#define RT_WSTATE	020	/* write state info into root */
#define RT_WMENU	040	/* write menus into root file */

/*
 * This struct saves the state of a previous root read when nested reads
 * occur in a root file.
 */
typedef struct {
	int	rs_rtflag;	/* i_rtflag */
	int	rs_rtcx;	/* i_rtcx */
	int	rs_rtcln;	/* i_rtcln */
	int32_t	rs_rtseekp;	/* i_rtseekp */
	char rs_rtname[P_ISLEN];	/* i_rtname */
} RTSAV;

#define P_RTLMAX	5	/* max num of nested root reads */

/*
 * Root support variables local to each process.
 */
extern int infd;		/* root file descriptor */
extern int32_t lastrseek;		/* last root file seek pointer */
extern int echo;		/* echo flag */

/*
 *	Messages.  Allocated as bits in an int;  each process can receive
 * up to 16 messages.  Global messages defined below are sent by comm and
 * received by all other REX processes.  They are allocated from bit 15
 * down.  Other messages recived by a process cannot overlap these global
 * messages.
 */

/*
 *	Global messages sent by comm only to all other REX processes.
 * Comm does not receive these messages.
 */
#define G_KILL		15	/* kill process */
#define G_STOP		14	/* process is set to stop state */
#define G_RUN		13	/* process is set to run state */
#define G_NMEXEC	12	/* process is sent a noun, verb or menu
				   access command */
#define G_RTEXEC	11	/* process is sent a root command */

/*
 *	Messages received by comm.  Note:  comm doesnt receive the global
 * messages (it sends them) and therefore its message numbers can overlap
 * global message numbers.
 */
#define CM_RXERR	0	/* error report */
#define CM_BADCHILD	1	/* comm-started child process bombs */
#define CM_SLEEP	2	/* process wishes comm to sleep (to free the
				   keyboard) */
#define CM_WAKEUP	3	/* process lets comm wakeup */
#define CM_STATUS	4	/* print new status line */
#define CM_SENDNAME	5	/* process is ready to send its nouns, menus */
#define CM_AFFIRM	6	/* successful response to comm msg */
#define CM_NEG		7	/* unsuccessful response to comm msg */

/*
 *	Messages received by scribe.  These cannot overlap global messages
 * above.
 */
#define SC_ANERR	0	/* analog file error;  error type stored in
				   r_b->r_anerr */
#define SC_EVERR	1	/* event error;  buffer overflow */
#define SC_EDUMP	2	/* write E buffer to disk */
#define SC_ADUMP	3	/* write A buffer to disk */
#define SC_ADERR	4	/* a/d device error */

/*
 *	Messages received by the int process.  Cannot overlap global messages.
 *	(Currently int processes need to receive only global messages).
 */

/*
 *	Messages received by display.  Cannot overlap global messages.
 */
#define DS_DRAS		0	/* build a new raster on specified ecode */
#define DS_WDNEW	1	/* rebuild window display */

/*
 * Function prototypes.
 */
int sendmsg(PROCTBL_P p, u_int msg);

#endif /*  __PROC_H__ */
