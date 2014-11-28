#include "sym_stub.h"

/* Data */
data_uchar_ptr_t    _ctype                          = (data_uchar_ptr_t)      0x01c3c398;
data_void_ptr_t     irq_counts                      = (data_void_ptr_t)       0x02fcb888;
data_void_ptr_t     TCD_Current_Thread              = (data_void_ptr_t)       0x0203ac50;


/* Function */
func_ret_void_t     __touch_exception               = (func_ret_void_t)       0x00bd4648;
func_ret_int_t      bind                            = (func_ret_int_t)        0x00c3b1b8;
func_ret_int_t      close                           = (func_ret_int_t)        0x00c3aa08;
func_ret_int_t      connect                         = (func_ret_int_t)        0x00c3b3c8;
func_ret_void_ptr_t create_task                     = (func_ret_void_ptr_t)   0x00bb4110;
func_ret_void_t     delete_task                     = (func_ret_void_t)       0x00bb3d68;
func_ret_int_t      fsync                           = (func_ret_int_t)        0x00bfd338;
func_ret_int_t      gettimeofday                    = (func_ret_int_t)        0x00bc8030;
func_ret_int_t      inet_pton                       = (func_ret_int_t)        0x00707a18;
func_ret_void_ptr_t kmalloc                         = (func_ret_void_ptr_t)   0x00ba66f0;
func_ret_void_t     kfree                           = (func_ret_void_t)       0x00ba4ee8;
func_ret_void_ptr_t krealloc                        = (func_ret_void_ptr_t)   0x00ba7eb0;
func_ret_void_ptr_t memcpy                          = (func_ret_void_ptr_t)   0x00df49c0;
func_ret_void_t     memset                          = (func_ret_void_t)       0x00df48a0;
func_ret_int_t      open                            = (func_ret_int_t)        0x00bfb828;
func_ret_int_t      printk                          = (func_ret_int_t)        0x001bd860;
func_ret_void_t     printk_rt                       = (func_ret_void_t)       0x00c40820;
func_ret_ssize_t    read                            = (func_ret_ssize_t)      0x00c3a9b8;
func_ret_ssize_t    recvfrom                        = (func_ret_ssize_t)      0x00c3c040;
func_ret_int_t      select                          = (func_ret_int_t)        0x00c3aa30;
func_ret_int_t      shutdown                        = (func_ret_int_t)        0x00c3cd38;
func_ret_void_t     sleep                           = (func_ret_void_t)       0x00bb3b30;
func_ret_int_t      socket                          = (func_ret_int_t)        0x00c3aba8;
func_ret_int_t      sprintf                         = (func_ret_int_t)        0x00c40758;
func_ret_int_t      stat                            = (func_ret_int_t)        0x00bfe1c8;
func_ret_long_t     strlen                          = (func_ret_long_t)       0x00c3e5a0;
func_ret_int_t      strncmp                         = (func_ret_int_t)        0x00c3e4e0;
func_ret_void_ptr_t strstr                          = (func_ret_void_ptr_t)   0x00c3e900;
func_ret_void_t     trace_currentfunction           = (func_ret_void_t)       0x00df6958;
func_ret_ssize_t    write                           = (func_ret_ssize_t)      0x00c3a990;


