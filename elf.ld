/*
 * Copyright (C) 2006 ruijie network inc.
 * All rights reserved.
 *
 * /scripts/mips/mips-trad-eb32.ld
 * Original Author: ChenPP(chenpp@star-net.cn), 2006-6-21
 *
 * Description:
 *
 * Link script for MIPS BigEndian.
 *
 * History:
 *
 * Chenkb(chenkb@star-net.cn), 2006-7-7
 * Add 'pci_fixup' section.
 *
 * ChenPP(chenpp@star-net.cn), 2006-6-21
 * inital version.
 */
OUTPUT_FORMAT("elf32-tradbigmips", "elf32-tradbigmips", "elf32-tradlittlemips")
OUTPUT_ARCH(mips)

/*
 * 强制要求所有的MIPS平台都使用这个入口函数。
 */
ENTRY(dynload_elf)

SECTIONS
{
    . = 0x10000000;
    .text : {
        _ftext = . ;
        KEEP(*(.vectors .vectors.*))
        KEEP(*(.init .init.*))
        KEEP(*(.rom_string .rom_string.*))
        *(.text .stub .text.* .gnu.linkonce.t.*)
        *(.gnu.warning)
        *(.mips16.fn.*) *(.mips16.call.*)
        _etext = .;
    } =0

    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
    .rodata1 : { *(.rodata1) }

    . = ALIGN(32);
    .data : {
        _fdata = . ;
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
    }

    /*
     * NGSA系统自定义的段.
     * 都把它们放到一起，并且保证它们符合最大对齐要求.
     */
    . = ALIGN(32);
    .ngsa_sects : {
        _fmodule_list = .;
        KEEP(*(module_list))
        _emodule_list = .;

        . = ALIGN(32);
        _fusrdef_sect = .;
        KEEP(*(usrdef_sect))
        _eusrdef_sect = .;

        . = ALIGN(32);
        _fsubsys_sect = .;
        KEEP(*(subsys_sect))
        _esubsys_sect = .;
    }

    . = ALIGN(32);
    .gcov : {
        __CTOR_LIST__ = .;
        LONG((__CTOR_END__ - __CTOR_LIST__) /
            (__CTOR_LIST2__ - __CTOR_LIST__) - 2)
        __CTOR_LIST2__ = .;
        KEEP(*(.ctors))
        LONG(0)
        __CTOR_END__ = .;
        __DTOR_LIST__ = .;
        LONG((__DTOR_END__ - __DTOR_LIST__) /
            (__CTOR_LIST2__ - __CTOR_LIST__) - 2)
        KEEP(*(.dtors))
        LONG(0)
        __DTOR_END__ = .;
    }

    . = ALIGN(32);
    .pci_fixup : {
        __start_pci_fixups_early = .;
        KEEP(*(.pci_fixup_early))
        __end_pci_fixups_early = .;

        __start_pci_fixups_header = .;
        KEEP(*(.pci_fixup_header))
        __end_pci_fixups_header = .;   

        __start_pci_fixups_final = .;
        KEEP(*(.pci_fixup_final))
        __end_pci_fixups_final = .; 

        __start_pci_fixups_enable = .;
        KEEP(*(.pci_fixup_enable))
        __end_pci_fixups_enable = .;

        __start_pci_fixups_busnr = .;
        KEEP(*(.pci_fixup_busnr))
        __end_pci_fixups_busnr = .;

        __start_pci_fixups_resource = .;
        KEEP(*(.pci_fixup_resource))
        __end_pci_fixups_resource = .;
    }

    . = ALIGN(32);
    .targets : {
        __start_real_target = .;
        KEEP(*(.real_target))
        __end_real_target = .;
    }
    
    . = ALIGN(32);
    .checkfunction :  {
        __start_check_function = .;
        KEEP(*(.check_function))
        __end_check_funtion = .;
    }

    . = ALIGN(32);
    __start___param = .;
    .param : { KEEP(*(__param)) }
    __stop___param = .;

    . = ALIGN(32);
    __start__magic_table = .;
    .magic_table : { KEEP(*(.magic_table)) }
    __end__magic_table = .;

    . = ALIGN(32);
    .symbol : {
        _fsymbol = .;
        _f_sym_table = .;
        KEEP(*(.symbol.table))
        _e_sym_table = .;        
        _f_sym_str = .;
        KEEP(*(.symbol.str))
        _e_sym_str = .;
        _esymbol = .;
    }

    . = ALIGN(32);
    _gp = ALIGN(16) + 0x7ff0;
    .sdata : { *(.sdata .sdata.* .gnu.linkonce.s.*) }
    .lit8 : { *(.lit8) }
    .lit4 : { *(.lit4) }
    _edata = . ;

    . = ALIGN(32);
    PROVIDE(__bss_start = .);
    _fbss = .;
    .sbss : {
        PROVIDE(__sbss_start = .);
        *(.dynsbss)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
        PROVIDE(__sbss_end = .);
    }
    .bss : {
       *(.dynbss)
       *(.bss .bss.* .gnu.linkonce.b.*)
       *(COMMON)
       /* Align here to ensure that the .bss section occupies space up to
          _end.  Align after .bss to ensure correct alignment even if the
          .bss section disappears because there are no input sections.  */
       . = ALIGN(32);
    }
    . = ALIGN(32);
    _end = .;
    PROVIDE(__bss_end = .);

    /* Stabs debugging sections.  */
    .stab           0 : { *(.stab) }
    .stabstr        0 : { *(.stabstr) }
    .stab.excl      0 : { *(.stab.excl) }
    .stab.exclstr   0 : { *(.stab.exclstr) }
    .stab.index     0 : { *(.stab.index) }
    .stab.indexstr  0 : { *(.stab.indexstr) }
    .comment        0 : { *(.comment) }

    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */

    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }

    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }

    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }

    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }

    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
    .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
    .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
    /DISCARD/ : { *(.note.GNU-stack) }
}

