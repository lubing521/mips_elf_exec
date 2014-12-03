#include "sym_stub.h"

/* Data */
data_uchar_ptr_t    _ctype                          = (data_uchar_ptr_t)      0x01c37918;
data_void_ptr_t     irq_counts                      = (data_void_ptr_t)       0x02fc6888;
data_void_ptr_t     TCD_Current_Thread              = (data_void_ptr_t)       0x02035ee8;


/* Function */
func_ret_void_t     __touch_exception               = (func_ret_void_t)       0x00bd14d8;
func_ret_int_t      bind                            = (func_ret_int_t)        0x00c38048;
func_ret_int_t      close                           = (func_ret_int_t)        0x00c37898;
func_ret_int_t      connect                         = (func_ret_int_t)        0x00c38258;
func_ret_void_ptr_t create_task                     = (func_ret_void_ptr_t)   0x00bb0fa0;
func_ret_void_t     delete_task                     = (func_ret_void_t)       0x00bb0bf8;
func_ret_int_t      fsync                           = (func_ret_int_t)        0x00bfa1c8;
func_ret_int_t      gettimeofday                    = (func_ret_int_t)        0x00bc4ec0;
func_ret_int_t      inet_pton                       = (func_ret_int_t)        0x007048a8;
func_ret_void_ptr_t kmalloc                         = (func_ret_void_ptr_t)   0x00ba3580;
func_ret_void_t     kfree                           = (func_ret_void_t)       0x00ba1d78;
func_ret_void_ptr_t krealloc                        = (func_ret_void_ptr_t)   0x00ba4d40;
func_ret_void_ptr_t memcpy                          = (func_ret_void_ptr_t)   0x00df1860;
func_ret_void_t     memset                          = (func_ret_void_t)       0x00df1740;
func_ret_int_t      open                            = (func_ret_int_t)        0x00bf86b8;
func_ret_int_t      printk                          = (func_ret_int_t)        0x001bd860;
func_ret_void_t     printk_rt                       = (func_ret_void_t)       0x00c3d6b0;
func_ret_ssize_t    read                            = (func_ret_ssize_t)      0x00c37848;
func_ret_ssize_t    recvfrom                        = (func_ret_ssize_t)      0x00c38ed0;
func_ret_int_t      select                          = (func_ret_int_t)        0x00c378c0;
func_ret_int_t      shutdown                        = (func_ret_int_t)        0x00c39bc8;
func_ret_void_t     sleep                           = (func_ret_void_t)       0x00bb09c0;
func_ret_int_t      socket                          = (func_ret_int_t)        0x00c37a38;
func_ret_int_t      sprintf                         = (func_ret_int_t)        0x00c3d5e8;
func_ret_int_t      stat                            = (func_ret_int_t)        0x00bfb058;
func_ret_long_t     strlen                          = (func_ret_long_t)       0x00c3b430;
func_ret_int_t      strncmp                         = (func_ret_int_t)        0x00c3b370;
func_ret_void_ptr_t strstr                          = (func_ret_void_ptr_t)   0x00c3b790;
func_ret_void_t     trace_currentfunction           = (func_ret_void_t)       0x00df37f8;
func_ret_ssize_t    write                           = (func_ret_ssize_t)      0x00c37820;


