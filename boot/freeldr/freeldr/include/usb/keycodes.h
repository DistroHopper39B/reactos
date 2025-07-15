/* Public Domain Curses */

/* $Id: curses.h,v 1.295 2008/07/15 17:13:25 wmcbrine Exp $ */

#ifndef _KEYCODES_H
#define _KEYCODES_H

/*----------------------------------------------------------------------
 *
 *  Function and Keypad Key Definitions.
 *  Many are just for compatibility.
 *
 */

#define USB_KEY_CODE_YES  0x100  /* If get_wch() gives a key code */

#define USB_KEY_BREAK     0x101  /* Not on PC KBD */
#define USB_KEY_DOWN      0x102  /* Down arrow key */
#define USB_KEY_UP        0x103  /* Up arrow key */
#define USB_KEY_LEFT      0x104  /* Left arrow key */
#define USB_KEY_RIGHT     0x105  /* Right arrow key */
#define USB_KEY_HOME      0x106  /* home key */
#define USB_KEY_BACKSPACE 0x107  /* not on pc */
#define USB_KEY_F0        0x108  /* function keys; 64 reserved */

#define USB_KEY_DL        0x148  /* delete line */
#define USB_KEY_IL        0x149  /* insert line */
#define USB_KEY_DC        0x14a  /* delete character */
#define USB_KEY_IC        0x14b  /* insert char or enter ins mode */
#define USB_KEY_EIC       0x14c  /* exit insert char mode */
#define USB_KEY_CLEAR     0x14d  /* clear screen */
#define USB_KEY_EOS       0x14e  /* clear to end of screen */
#define USB_KEY_EOL       0x14f  /* clear to end of line */
#define USB_KEY_SF        0x150  /* scroll 1 line forward */
#define USB_KEY_SR        0x151  /* scroll 1 line back (reverse) */
#define USB_KEY_NPAGE     0x152  /* next page */
#define USB_KEY_PPAGE     0x153  /* previous page */
#define USB_KEY_STAB      0x154  /* set tab */
#define USB_KEY_CTAB      0x155  /* clear tab */
#define USB_KEY_CATAB     0x156  /* clear all tabs */
#define USB_KEY_ENTER     0x157  /* enter or send (unreliable) */
#define USB_KEY_SRESET    0x158  /* soft/reset (partial/unreliable) */
#define USB_KEY_RESET     0x159  /* reset/hard reset (unreliable) */
#define USB_KEY_PRINT     0x15a  /* print/copy */
#define USB_KEY_LL        0x15b  /* home down/bottom (lower left) */
#define USB_KEY_ABORT     0x15c  /* abort/terminate key (any) */
#define USB_KEY_SHELP     0x15d  /* short help */
#define USB_KEY_LHELP     0x15e  /* long help */
#define USB_KEY_BTAB      0x15f  /* Back tab key */
#define USB_KEY_BEG       0x160  /* beg(inning) key */
#define USB_KEY_CANCEL    0x161  /* cancel key */
#define USB_KEY_CLOSE     0x162  /* close key */
#define USB_KEY_COMMAND   0x163  /* cmd (command) key */
#define USB_KEY_COPY      0x164  /* copy key */
#define USB_KEY_CREATE    0x165  /* create key */
#define USB_KEY_END       0x166  /* end key */
#define USB_KEY_EXIT      0x167  /* exit key */
#define USB_KEY_FIND      0x168  /* find key */
#define USB_KEY_HELP      0x169  /* help key */
#define USB_KEY_MARK      0x16a  /* mark key */
#define USB_KEY_MESSAGE   0x16b  /* message key */
#define USB_KEY_MOVE      0x16c  /* move key */
#define USB_KEY_NEXT      0x16d  /* next object key */
#define USB_KEY_OPEN      0x16e  /* open key */
#define USB_KEY_OPTIONS   0x16f  /* options key */
#define USB_KEY_PREVIOUS  0x170  /* previous object key */
#define USB_KEY_REDO      0x171  /* redo key */
#define USB_KEY_REFERENCE 0x172  /* ref(erence) key */
#define USB_KEY_REFRESH   0x173  /* refresh key */
#define USB_KEY_REPLACE   0x174  /* replace key */
#define USB_KEY_RESTART   0x175  /* restart key */
#define USB_KEY_RESUME    0x176  /* resume key */
#define USB_KEY_SAVE      0x177  /* save key */
#define USB_KEY_SBEG      0x178  /* shifted beginning key */
#define USB_KEY_SCANCEL   0x179  /* shifted cancel key */
#define USB_KEY_SCOMMAND  0x17a  /* shifted command key */
#define USB_KEY_SCOPY     0x17b  /* shifted copy key */
#define USB_KEY_SCREATE   0x17c  /* shifted create key */
#define USB_KEY_SDC       0x17d  /* shifted delete char key */
#define USB_KEY_SDL       0x17e  /* shifted delete line key */
#define USB_KEY_SELECT    0x17f  /* select key */
#define USB_KEY_SEND      0x180  /* shifted end key */
#define USB_KEY_SEOL      0x181  /* shifted clear line key */
#define USB_KEY_SEXIT     0x182  /* shifted exit key */
#define USB_KEY_SFIND     0x183  /* shifted find key */
#define USB_KEY_SHOME     0x184  /* shifted home key */
#define USB_KEY_SIC       0x185  /* shifted input key */

#define USB_KEY_SLEFT     0x187  /* shifted left arrow key */
#define USB_KEY_SMESSAGE  0x188  /* shifted message key */
#define USB_KEY_SMOVE     0x189  /* shifted move key */
#define USB_KEY_SNEXT     0x18a  /* shifted next key */
#define USB_KEY_SOPTIONS  0x18b  /* shifted options key */
#define USB_KEY_SPREVIOUS 0x18c  /* shifted prev key */
#define USB_KEY_SPRINT    0x18d  /* shifted print key */
#define USB_KEY_SREDO     0x18e  /* shifted redo key */
#define USB_KEY_SREPLACE  0x18f  /* shifted replace key */
#define USB_KEY_SRIGHT    0x190  /* shifted right arrow */
#define USB_KEY_SRSUME    0x191  /* shifted resume key */
#define USB_KEY_SSAVE     0x192  /* shifted save key */
#define USB_KEY_SSUSPEND  0x193  /* shifted suspend key */
#define USB_KEY_SUNDO     0x194  /* shifted undo key */
#define USB_KEY_SUSPEND   0x195  /* suspend key */
#define USB_KEY_UNDO      0x196  /* undo key */

/* PDCurses-specific key definitions -- PC only */

#define USB_ALT_0         0x197
#define USB_ALT_1         0x198
#define USB_ALT_2         0x199
#define USB_ALT_3         0x19a
#define USB_ALT_4         0x19b
#define USB_ALT_5         0x19c
#define USB_ALT_6         0x19d
#define USB_ALT_7         0x19e
#define USB_ALT_8         0x19f
#define USB_ALT_9         0x1a0
#define USB_ALT_A         0x1a1
#define USB_ALT_B         0x1a2
#define USB_ALT_C         0x1a3
#define USB_ALT_D         0x1a4
#define USB_ALT_E         0x1a5
#define USB_ALT_F         0x1a6
#define USB_ALT_G         0x1a7
#define USB_ALT_H         0x1a8
#define USB_ALT_I         0x1a9
#define USB_ALT_J         0x1aa
#define USB_ALT_K         0x1ab
#define USB_ALT_L         0x1ac
#define USB_ALT_M         0x1ad
#define USB_ALT_N         0x1ae
#define USB_ALT_O         0x1af
#define USB_ALT_P         0x1b0
#define USB_ALT_Q         0x1b1
#define USB_ALT_R         0x1b2
#define USB_ALT_S         0x1b3
#define USB_ALT_T         0x1b4
#define USB_ALT_U         0x1b5
#define USB_ALT_V         0x1b6
#define USB_ALT_W         0x1b7
#define USB_ALT_X         0x1b8
#define USB_ALT_Y         0x1b9
#define USB_ALT_Z         0x1ba

#define USB_CTL_LEFT      0x1bb  /* Control-Left-Arrow */
#define USB_CTL_RIGHT     0x1bc
#define USB_CTL_PGUP      0x1bd
#define USB_CTL_PGDN      0x1be
#define USB_CTL_HOME      0x1bf
#define USB_CTL_END       0x1c0

#define USB_KEY_A1        0x1c1  /* upper left on Virtual keypad */
#define USB_KEY_A2        0x1c2  /* upper middle on Virt. keypad */
#define USB_KEY_A3        0x1c3  /* upper right on Vir. keypad */
#define USB_KEY_B1        0x1c4  /* middle left on Virt. keypad */
#define USB_KEY_B2        0x1c5  /* center on Virt. keypad */
#define USB_KEY_B3        0x1c6  /* middle right on Vir. keypad */
#define USB_KEY_C1        0x1c7  /* lower left on Virt. keypad */
#define USB_KEY_C2        0x1c8  /* lower middle on Virt. keypad */
#define USB_KEY_C3        0x1c9  /* lower right on Vir. keypad */

#define USB_PADSLASH      0x1ca  /* slash on keypad */
#define USB_PADENTER      0x1cb  /* enter on keypad */
#define USB_CTL_PADENTER  0x1cc  /* ctl-enter on keypad */
#define USB_ALT_PADENTER  0x1cd  /* alt-enter on keypad */
#define USB_PADSTOP       0x1ce  /* stop on keypad */
#define USB_PADSTAR       0x1cf  /* star on keypad */
#define USB_PADMINUS      0x1d0  /* minus on keypad */
#define USB_PADPLUS       0x1d1  /* plus on keypad */
#define USB_CTL_PADSTOP   0x1d2  /* ctl-stop on keypad */
#define USB_CTL_PADCENTER 0x1d3  /* ctl-enter on keypad */
#define USB_CTL_PADPLUS   0x1d4  /* ctl-plus on keypad */
#define USB_CTL_PADMINUS  0x1d5  /* ctl-minus on keypad */
#define USB_CTL_PADSLASH  0x1d6  /* ctl-slash on keypad */
#define USB_CTL_PADSTAR   0x1d7  /* ctl-star on keypad */
#define USB_ALT_PADPLUS   0x1d8  /* alt-plus on keypad */
#define USB_ALT_PADMINUS  0x1d9  /* alt-minus on keypad */
#define USB_ALT_PADSLASH  0x1da  /* alt-slash on keypad */
#define USB_ALT_PADSTAR   0x1db  /* alt-star on keypad */
#define USB_ALT_PADSTOP   0x1dc  /* alt-stop on keypad */
#define USB_CTL_INS       0x1dd  /* ctl-insert */
#define USB_ALT_DEL       0x1de  /* alt-delete */
#define USB_ALT_INS       0x1df  /* alt-insert */
#define USB_CTL_UP        0x1e0  /* ctl-up arrow */
#define USB_CTL_DOWN      0x1e1  /* ctl-down arrow */
#define USB_CTL_TAB       0x1e2  /* ctl-tab */
#define USB_ALT_TAB       0x1e3
#define USB_ALT_MINUS     0x1e4
#define USB_ALT_EQUAL     0x1e5
#define USB_ALT_HOME      0x1e6
#define USB_ALT_PGUP      0x1e7
#define USB_ALT_PGDN      0x1e8
#define USB_ALT_END       0x1e9
#define USB_ALT_UP        0x1ea  /* alt-up arrow */
#define USB_ALT_DOWN      0x1eb  /* alt-down arrow */
#define USB_ALT_RIGHT     0x1ec  /* alt-right arrow */
#define USB_ALT_LEFT      0x1ed  /* alt-left arrow */
#define USB_ALT_ENTER     0x1ee  /* alt-enter */
#define USB_ALT_ESC       0x1ef  /* alt-escape */
#define USB_ALT_BQUOTE    0x1f0  /* alt-back quote */
#define USB_ALT_LBRACKET  0x1f1  /* alt-left bracket */
#define USB_ALT_RBRACKET  0x1f2  /* alt-right bracket */
#define USB_ALT_SEMICOLON 0x1f3  /* alt-semi-colon */
#define USB_ALT_FQUOTE    0x1f4  /* alt-forward quote */
#define USB_ALT_COMMA     0x1f5  /* alt-comma */
#define USB_ALT_STOP      0x1f6  /* alt-stop */
#define USB_ALT_FSLASH    0x1f7  /* alt-forward slash */
#define USB_ALT_BKSP      0x1f8  /* alt-backspace */
#define USB_CTL_BKSP      0x1f9  /* ctl-backspace */
#define USB_PAD0          0x1fa  /* keypad 0 */

#define USB_CTL_PAD0      0x1fb  /* ctl-keypad 0 */
#define USB_CTL_PAD1      0x1fc
#define USB_CTL_PAD2      0x1fd
#define USB_CTL_PAD3      0x1fe
#define USB_CTL_PAD4      0x1ff
#define USB_CTL_PAD5      0x200
#define USB_CTL_PAD6      0x201
#define USB_CTL_PAD7      0x202
#define USB_CTL_PAD8      0x203
#define USB_CTL_PAD9      0x204

#define USB_ALT_PAD0      0x205  /* alt-keypad 0 */
#define USB_ALT_PAD1      0x206
#define USB_ALT_PAD2      0x207
#define USB_ALT_PAD3      0x208
#define USB_ALT_PAD4      0x209
#define USB_ALT_PAD5      0x20a
#define USB_ALT_PAD6      0x20b
#define USB_ALT_PAD7      0x20c
#define USB_ALT_PAD8      0x20d
#define USB_ALT_PAD9      0x20e

#define USB_CTL_DEL       0x20f  /* clt-delete */
#define USB_ALT_BSLASH    0x210  /* alt-back slash */
#define USB_CTL_ENTER     0x211  /* ctl-enter */

#define USB_SHF_PADENTER  0x212  /* shift-enter on keypad */
#define USB_SHF_PADSLASH  0x213  /* shift-slash on keypad */
#define USB_SHF_PADSTAR   0x214  /* shift-star  on keypad */
#define USB_SHF_PADPLUS   0x215  /* shift-plus  on keypad */
#define USB_SHF_PADMINUS  0x216  /* shift-minus on keypad */
#define USB_SHF_UP        0x217  /* shift-up on keypad */
#define USB_SHF_DOWN      0x218  /* shift-down on keypad */
#define USB_SHF_IC        0x219  /* shift-insert on keypad */
#define USB_SHF_DC        0x21a  /* shift-delete on keypad */

#define USB_KEY_MOUSE     0x21b  /* "mouse" key */
#define USB_KEY_SHIFT_L   0x21c  /* Left-shift */
#define USB_KEY_SHIFT_R   0x21d  /* Right-shift */
#define USB_KEY_CONTROL_L 0x21e  /* Left-control */
#define USB_KEY_CONTROL_R 0x21f  /* Right-control */
#define USB_KEY_ALT_L     0x220  /* Left-alt */
#define USB_KEY_ALT_R     0x221  /* Right-alt */
#define USB_KEY_RESIZE    0x222  /* Window resize */
#define USB_KEY_SUP       0x223  /* Shifted up arrow */
#define USB_KEY_SDOWN     0x224  /* Shifted down arrow */

#define USB_KEY_MIN       KEY_BREAK      /* Minimum curses key value */
#define USB_KEY_MAX       KEY_SDOWN      /* Maximum curses key */

#define USB_KEY_F(n)      (USB_KEY_F0 + (n))

#endif /* _KEYCODES_H */
