/* user_shell.S - "simple" EL0 userspace shell with inline command parsing */
    .section .user, "ax"
    .global  _user_shell_start
    .global  _user_shell_end

_user_shell_start:
    // init buffer pointer and length
    adr     x2, input_buffer
    mov     x3, #0          // buffer length counter

print_prompt:
    // print prompt: $
    mov     x8, #1        // SYS_PUTC
    mov     x0, #'$'      // char print
    svc     #0            // syscall
    
    // print space after prompt
    mov     x8, #1        // SYS_PUTC
    mov     x0, #' '      // space character
    svc     #0            // syscall

input_loop:
    // get character from user
    mov     x8, #2        // SYS_GETC
    svc     #0            // syscall
    mov     x1, x0        // save input character
    
    // check if its Enter/Return
    cmp     x1, #'\r'
    beq     process_command
    cmp     x1, #'\n'
    beq     process_command
    
    // check if its backspace (ASCII 8 or 127)
    cmp     x1, #8
    beq     handle_backspace
    cmp     x1, #127
    beq     handle_backspace
    
    // check if buffer is full (max 63 chars to be safe)
    cmp     x3, #63
    bge     input_loop      // ignore if buffer full
    
    // store character in buffer
    strb    w1, [x2, x3]
    add     x3, x3, #1      // increment buffer length
    
    // echo the character back
    mov     x0, x1        // set output character
    mov     x8, #1        // SYS_PUTC
    svc     #0            // syscall
    
    b       input_loop

handle_backspace:
    // check if buffer is empty
    cmp     x3, #0
    beq     input_loop      // ignore if buffer empty
    
    // remove last character from buffer
    sub     x3, x3, #1
    
    // send backspace sequence: \b \b
    mov     x8, #1        // SYS_PUTC
    mov     x0, #8        // backspace
    svc     #0
    
    mov     x8, #1        // SYS_PUTC
    mov     x0, #' '      // space (to erase character)
    svc     #0
    
    mov     x8, #1        // SYS_PUTC
    mov     x0, #8        // backspace again
    svc     #0
    
    b       input_loop

process_command:
    // null terminate the command
    strb    wzr, [x2, x3]
    
    // print newline
    mov     x8, #1        // SYS_PUTC
    mov     x0, #'\n'     // newline
    svc     #0            // syscall
    
    // check if command is empty
    cmp     x3, #0
    beq     reset_and_prompt
    
    // check for "echo" command (inline)
    cmp     x3, #4
    blt     check_other_commands
    ldrb    w4, [x2]
    cmp     w4, #'e'
    bne     check_other_commands
    ldrb    w4, [x2, #1]
    cmp     w4, #'c'
    bne     check_other_commands
    ldrb    w4, [x2, #2]
    cmp     w4, #'h'
    bne     check_other_commands
    ldrb    w4, [x2, #3]
    cmp     w4, #'o'
    bne     check_other_commands
    
    // check if its exactly "echo" or "echo "
    cmp     x3, #4
    beq     echo_empty
    ldrb    w4, [x2, #4]
    cmp     w4, #' '
    bne     check_other_commands
    
    // handle echo command - print everything after "echo "
    mov     x4, #5          // start after "echo "
    b       echo_loop

check_other_commands:
    // check for "sysname" command (inline)
    cmp     x3, #7
    bne     check_help
    ldrb    w4, [x2]
    cmp     w4, #'s'
    bne     check_help
    ldrb    w4, [x2, #1]
    cmp     w4, #'y'
    bne     check_help
    ldrb    w4, [x2, #2]
    cmp     w4, #'s'
    bne     check_help
    ldrb    w4, [x2, #3]
    cmp     w4, #'n'
    bne     check_help
    ldrb    w4, [x2, #4]
    cmp     w4, #'a'
    bne     check_help
    ldrb    w4, [x2, #5]
    cmp     w4, #'m'
    bne     check_help
    ldrb    w4, [x2, #6]
    cmp     w4, #'e'
    bne     check_help
    
    // print sysname message (inline)
    adr     x4, sysname_msg
    b       print_message

check_help:
    // check for "help" command (inline)
    cmp     x3, #4
    bne     check_clear
    ldrb    w4, [x2]
    cmp     w4, #'h'
    bne     check_clear
    ldrb    w4, [x2, #1]
    cmp     w4, #'e'
    bne     check_clear
    ldrb    w4, [x2, #2]
    cmp     w4, #'l'
    bne     check_clear
    ldrb    w4, [x2, #3]
    cmp     w4, #'p'
    bne     check_clear
    
    // print help message (inline)
    adr     x4, help_msg
    b       print_message

check_clear:
    // check for "clear" command (inline)
    cmp     x3, #5
    bne     check_ls
    ldrb    w4, [x2]
    cmp     w4, #'c'
    bne     check_ls
    ldrb    w4, [x2, #1]
    cmp     w4, #'l'
    bne     check_ls
    ldrb    w4, [x2, #2]
    cmp     w4, #'e'
    bne     check_ls
    ldrb    w4, [x2, #3]
    cmp     w4, #'a'
    bne     check_ls
    ldrb    w4, [x2, #4]
    cmp     w4, #'r'
    bne     check_ls
    
    // send clear screen sequence (ANSI escape code)
    mov     x8, #1        // SYS_PUTC
    mov     x0, #27       // ESC character
    svc     #0
    mov     x0, #'['
    svc     #0
    mov     x0, #'2'
    svc     #0
    mov     x0, #'J'
    svc     #0
    mov     x0, #27       // ESC character
    svc     #0
    mov     x0, #'['
    svc     #0
    mov     x0, #'H'
    svc     #0
    
    b       reset_and_prompt

check_ls:
    // check for "ls" command (inline)
    cmp     x3, #2
    blt     check_touch
    ldrb    w4, [x2]
    cmp     w4, #'l'
    bne     check_touch
    ldrb    w4, [x2, #1]
    cmp     w4, #'s'
    bne     check_touch
    
    // check if its exactly "ls" or "ls " (with argument)
    cmp     x3, #2
    beq     ls_no_args      // exactly "ls" with no arguments
    ldrb    w4, [x2, #2]
    cmp     w4, #' '
    bne     check_touch     // must have space after "ls" or be exactly "ls"
    
    // find path start (skip spaces after "ls")
    mov     x4, #3          // start after "ls "
find_ls_path:
    cmp     x4, x3
    bge     ls_no_args      // no path found, use default
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_ls_path
    add     x4, x4, #1
    b       find_ls_path
    
found_ls_path:
    // x4 now points to path start
    // Calculate path length
    mov     x5, x4          // path start
    mov     x6, x4          // end pointer
find_ls_path_end:
    cmp     x6, x3
    bge     ls_path_found
    ldrb    w7, [x2, x6]
    cmp     w7, #' '
    beq     ls_path_found
    add     x6, x6, #1
    b       find_ls_path_end
    
ls_path_found:
    // x5 = start, x6 = end, length = x6 - x5
    sub     x7, x6, x5      // path length
    cmp     x7, #0
    beq     ls_no_args
    
    // null terminate the path in place
    mov     w8, #0
    strb    w8, [x2, x6]
    
    // Add path to input buffer (at x5)
    add     x9, x2, x5      // point to path in buffer
    b       ls_with_path
    
ls_no_args:
    // for ls with no arguments, use current working directory
    // will allocate extra space for the path and use MSG_GETCWD
    
ls_with_path:
    // ls command - list directory contents
    // allocate space for message struct + directory entries + path buffer
    // message struct: 96 bytes
    // directory entries: 4KB for dirents 
    // path buffer: 512 bytes for current directory path
    sub     sp, sp, #512     // path buffer (512 bytes)
    sub     sp, sp, #4096    // allocate 4KB for dirents
    sub     sp, sp, #96      // allocate space for message struct
    
    // check if we need to get current directory (ls with no args)
    cmp     x3, #2           // check if command was exactly "ls"
    beq     ls_get_cwd
    
    // For ls with arguments, x9 already points to the path
    b       ls_do_readdir
    
ls_get_cwd:
    // get current working directory first
    // init message structure for MSG_GETCWD
    mov     x4, sp           // point to message struct
    
    // clear message structure
    mov     x5, #0
    mov     x6, #8           // 8 x 8-byte words to clear
clear_ls_cwd_msg:
    str     x5, [x4, x6, lsl #3]
    subs    x6, x6, #1
    bge     clear_ls_cwd_msg
    
    // set up message fields for MSG_GETCWD
    mov     x5, #15          // MSG_GETCWD
    str     x5, [x4]         // msg.type
    
    add     x5, sp, #96      // point to path buffer (after message struct)
    add     x5, x5, #4096    // point to path buffer (after dirents)
    str     x5, [x4, #24]    // msg.data
    
    mov     x5, #512         // Buffer size
    str     x5, [x4, #32]    // msg.size
    
    // send message
    mov     x0, x4
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check result
    cmp     x0, #0
    blt     ls_cwd_error
    
    // point to the cwd path buffer for the read_dir call
    add     x9, sp, #96      // point to after message struct
    add     x9, x9, #4096    // point to path buffer
    b       ls_do_readdir
    
ls_cwd_error:
    // Fall back to root path on error
    adr     x9, root_path    // point to "/"
    
ls_do_readdir:
    // init message structure for MSG_READ_DIR
    mov     x4, #12          // MSG_READ_DIR
    str     x4, [sp]         // msg->type
    str     x9, [sp, #8]     // msg->path (either from args or cwd)
    
    // clear other fields
    mov     x4, #0
    str     x4, [sp, #16]    // msg->argv
    
    // set data pointer to space after message (at offset 96)
    add     x4, sp, #96
    str     x4, [sp, #24]    // msg->data (space for dirents)
    
    // set size to available space for dirents (4KB)
    mov     x4, #4096        // 4KB for dirents
    str     x4, [sp, #32]    // msg->size
    
    // clear remaining fields
    mov     x4, #0
    str     x4, [sp, #40]    // msg->flags
    str     x4, [sp, #48]    // msg->fd
    str     x4, [sp, #56]    // msg->pid
    str     x4, [sp, #64]    // msg->status
    str     x4, [sp, #72]    // msg->entry
    str     x4, [sp, #80]    // msg->dirents
    str     x4, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     ls_error
    
    // Get result from message
    ldr     x4, [sp, #72]    // msg->dirent_count (corrected offset)
    cmp     x4, #0
    beq     ls_empty
    
    // Directory entries are in msg->data (at sp+96)
    add     x5, sp, #96      // point to directory entries
    mov     x6, #0           // Counter
    
ls_loop:
    cmp     x6, x4           // Compare counter with count
    bge     ls_done
    
    // Calculate dirent offset (each dirent is 8 + 256 = 264 bytes)
    mov     x7, #264
    mul     x7, x6, x7
    add     x7, x5, x7       // x7 = dirents[i]
    
    // Skip inode (8 bytes), print name at offset 8
    add     x7, x7, #8
    mov     x9, x7           // point to name
    
    // print filename
ls_name_loop:
    ldrb    w10, [x9]
    cmp     w10, #0
    beq     ls_name_done
    
    mov     x0, x10
    mov     x8, #1           // SYS_PUTC
    svc     #0
    
    add     x9, x9, #1
    b       ls_name_loop
    
ls_name_done:
    // print newline
    mov     x0, #'\n'
    mov     x8, #1           // SYS_PUTC
    svc     #0
    
    add     x6, x6, #1       // Increment counter
    b       ls_loop
    
ls_empty:
    // print empty directory message
    adr     x4, empty_msg
    b       print_message_and_restore_ls

ls_error:
    // print error message
    adr     x4, ls_error_msg
    b       print_message_and_restore_ls
    
ls_done:
    add     sp, sp, #96      // restore message struct space
    add     sp, sp, #4096    // restore dirents space
    add     sp, sp, #512     // restore path buffer space
    b       reset_and_prompt

ls_done_with_pwd_buffer:
    add     sp, sp, #96      // restore message struct space
    add     sp, sp, #4096    // restore dirents space
    add     sp, sp, #512     // restore path buffer space (for ls with no args)
    b       reset_and_prompt

check_touch:
    // check for "touch" command (inline)
    cmp     x3, #5
    blt     check_mkdir      // Try mkdir if not touch
    ldrb    w4, [x2]
    cmp     w4, #'t'
    bne     check_mkdir
    ldrb    w4, [x2, #1]
    cmp     w4, #'o'
    bne     check_mkdir
    ldrb    w4, [x2, #2]
    cmp     w4, #'u'
    bne     check_mkdir
    ldrb    w4, [x2, #3]
    cmp     w4, #'c'
    bne     check_mkdir
    ldrb    w4, [x2, #4]
    cmp     w4, #'h'
    bne     check_mkdir
    
    // check if its exactly "touch" or "touch " (with argument)
    cmp     x3, #5
    beq     touch_no_args    // exactly "touch" with no space
    ldrb    w4, [x2, #5]
    cmp     w4, #' '
    bne     check_mkdir      // must have space after "touch"
    
    // find start of filename (skip spaces after "touch")
    mov     x4, #6          // start after "touch "
find_filename:
    cmp     x4, x3
    bge     touch_no_args
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_filename
    add     x4, x4, #1
    b       find_filename
    
found_filename:
    // x4 now points to start of filename
    // Calculate filename length
    mov     x5, x4          // start of filename
    mov     x6, x4          // end pointer
find_filename_end:
    cmp     x6, x3
    bge     filename_found
    ldrb    w7, [x2, x6]
    cmp     w7, #' '
    beq     filename_found
    add     x6, x6, #1
    b       find_filename_end
    
filename_found:
    // x5 = start, x6 = end, length = x6 - x5
    sub     x7, x6, x5      // Filename length
    cmp     x7, #0
    beq     touch_no_args
    
    // null terminate the filename in place
    strb    wzr, [x2, x6]
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_CREATE
    mov     x8, #13         // MSG_CREATE
    str     x8, [sp]        // msg->type
    
    // set path pointer to filename in buffer
    add     x8, x2, x5      // point to filename
    str     x8, [sp, #8]    // msg->path
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #16]   // msg->argv
    str     x8, [sp, #24]   // msg->data
    str     x8, [sp, #32]   // msg->size
    str     x8, [sp, #40]   // msg->flags
    str     x8, [sp, #48]   // msg->fd
    str     x8, [sp, #56]   // msg->pid
    str     x8, [sp, #64]   // msg->status
    str     x8, [sp, #72]   // msg->entry
    str     x8, [sp, #80]   // msg->dirents
    str     x8, [sp, #88]   // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp          // Message pointer
    mov     x8, #4          // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     touch_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
touch_error:
    // print error message
    adr     x4, touch_error_msg
    b       print_message_simple
    
touch_no_args:
    // print usage message
    adr     x4, touch_usage_msg
    b       print_message_simple

// Simple message print function (no stack operations)
print_message_simple:
    ldrb    w5, [x4]
    cmp     w5, #0
    beq     print_msg_simple_done
    
    mov     x0, x5
    mov     x8, #1          // SYS_PUTC
    svc     #0
    
    add     x4, x4, #1
    b       print_message_simple
    
print_msg_simple_done:
    b       reset_and_prompt

check_mkdir:
    // check for "mkdir" command (inline)
    cmp     x3, #5
    blt     check_pwd        // Try pwd if not mkdir
    ldrb    w4, [x2]
    cmp     w4, #'m'
    bne     check_pwd
    ldrb    w4, [x2, #1]
    cmp     w4, #'k'
    bne     check_pwd
    ldrb    w4, [x2, #2]
    cmp     w4, #'d'
    bne     check_pwd
    ldrb    w4, [x2, #3]
    cmp     w4, #'i'
    bne     check_pwd
    ldrb    w4, [x2, #4]
    cmp     w4, #'r'
    bne     check_pwd
    
    // check if its exactly "mkdir" or "mkdir " (with argument)
    cmp     x3, #5
    beq     mkdir_no_args    // exactly "mkdir" with no space
    ldrb    w4, [x2, #5]
    cmp     w4, #' '
    bne     check_pwd        // must have space after "mkdir"
    
    // find start of dirname (skip spaces after "mkdir")
    mov     x4, #6          // start after "mkdir "
find_dirname:
    cmp     x4, x3
    bge     mkdir_no_args
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_dirname
    add     x4, x4, #1
    b       find_dirname
    
found_dirname:
    // x4 now points to start of dirname
    // Calculate dirname length
    mov     x5, x4          // start of dirname
    mov     x6, x4          // end pointer
find_dirname_end:
    cmp     x6, x3
    bge     dirname_found
    ldrb    w7, [x2, x6]
    cmp     w7, #' '
    beq     dirname_found
    add     x6, x6, #1
    b       find_dirname_end
    
dirname_found:
    // x5 = start, x6 = end, length = x6 - x5
    sub     x7, x6, x5      // Dirname length
    cmp     x7, #0
    beq     mkdir_no_args
    
    // null terminate the dirname in place
    strb    wzr, [x2, x6]
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_MKDIR
    mov     x8, #14         // MSG_MKDIR (value 14: MSG_CREATE=13, MSG_MKDIR=14)
    str     x8, [sp]        // msg->type
    
    // set path pointer to dirname in buffer
    add     x8, x2, x5      // point to dirname
    str     x8, [sp, #8]    // msg->path
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #16]   // msg->argv
    str     x8, [sp, #24]   // msg->data
    str     x8, [sp, #32]   // msg->size
    str     x8, [sp, #40]   // msg->flags
    str     x8, [sp, #48]   // msg->fd
    str     x8, [sp, #56]   // msg->pid
    str     x8, [sp, #64]   // msg->status
    str     x8, [sp, #72]   // msg->entry
    str     x8, [sp, #80]   // msg->dirents
    str     x8, [sp, #88]   // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp          // Message pointer
    mov     x8, #4          // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     mkdir_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
mkdir_error:
    // print error message
    adr     x4, mkdir_error_msg
    b       print_message_simple
    
mkdir_no_args:
    // print usage message
    adr     x4, mkdir_usage_msg
    b       print_message_simple

check_pwd:
    // check for "pwd" command (inline)
    cmp     x3, #3
    blt     check_cd         // cd if not pwd
    ldrb    w4, [x2]
    cmp     w4, #'p'
    bne     check_cd
    ldrb    w4, [x2, #1]
    cmp     w4, #'w'
    bne     check_cd
    ldrb    w4, [x2, #2]
    cmp     w4, #'d'
    bne     check_cd
    
    // check if its exactly "pwd" with no extra characters
    cmp     x3, #3
    beq     pwd_command      // exactly "pwd"
    
    // allow "pwd " with potential arguments (ignore them)
    ldrb    w4, [x2, #3]
    cmp     w4, #' '
    beq     pwd_command      // "pwd " is OK
    b       unknown_command   // "pwd" with something else is not OK

pwd_command:
    // allocate space for working directory path
    sub     sp, sp, #512     // 512 bytes for path buffer
    
    // init message structure for MSG_GETCWD
    sub     sp, sp, #64      // Message structure space
    mov     x4, sp
    
    // clear message structure
    mov     x5, #0
    mov     x6, #8           // 8 x 8-byte words to clear
clear_pwd_msg:
    str     x5, [x4, x6, lsl #3]
    subs    x6, x6, #1
    bge     clear_pwd_msg
    
    // set up message fields
    mov     x5, #15          // MSG_GETCWD (MSG_CREATE=13, MSG_MKDIR=14, MSG_GETCWD=15)
    str     x5, [x4]         // msg.type
    
    add     x5, sp, #64      // point to path buffer
    str     x5, [x4, #24]    // msg.data
    
    mov     x5, #512         // Buffer size
    str     x5, [x4, #32]    // msg.size
    
    // send message
    mov     x0, x4
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check result
    cmp     x0, #0
    blt     pwd_error
    
    // print the current working directory
    add     x4, sp, #64      // point to path buffer
    b       print_message_and_restore_pwd

pwd_error:
    adr     x4, pwd_error_msg
    b       print_message_and_restore_pwd

check_cd:
    // check for "cd" command (inline)
    cmp     x3, #2
    blt     check_cp         // Try cp if not cd
    ldrb    w4, [x2]
    cmp     w4, #'c'
    bne     check_cp
    ldrb    w4, [x2, #1]
    cmp     w4, #'d'
    bne     check_cp
    
    // check if its exactly "cd" or "cd " (with argument)
    cmp     x3, #2
    beq     cd_no_args       // exactly "cd" with no space (go to home/root)
    ldrb    w4, [x2, #2]
    cmp     w4, #' '
    bne     check_cp         // must have space after "cd"
    
    // find start of dirname (skip spaces after "cd")
    mov     x4, #3           // start after "cd "
find_cd_path:
    cmp     x4, x3
    bge     cd_no_args       // no path found, use default
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_cd_path
    add     x4, x4, #1
    b       find_cd_path
    
found_cd_path:
    // x4 now points to path start
    // Calculate path length
    mov     x5, x4           // path start
    mov     x6, x4           // end pointer
find_cd_path_end:
    cmp     x6, x3
    bge     cd_path_found
    ldrb    w7, [x2, x6]
    cmp     w7, #' '
    beq     cd_path_found
    add     x6, x6, #1
    b       find_cd_path_end
    
cd_path_found:
    // x5 = start, x6 = end, length = x6 - x5
    sub     x7, x6, x5       // path length
    cmp     x7, #0
    beq     cd_no_args
    
    // null terminate the path in place
    strb    wzr, [x2, x6]
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_CHDIR
    mov     x8, #16          // MSG_CHDIR (MSG_GETCWD=15, MSG_CHDIR=16)
    str     x8, [sp]         // msg->type
    
    // set path pointer to dirname in buffer
    add     x8, x2, x5       // point to path
    str     x8, [sp, #8]     // msg->path
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    str     x8, [sp, #24]    // msg->data
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     cd_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
cd_error:
    // print error message
    adr     x4, cd_error_msg
    b       print_message_simple
    
cd_no_args:
    // Change to root directory (default behavior)
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_CHDIR to root
    mov     x8, #16          // MSG_CHDIR
    str     x8, [sp]         // msg->type
    
    // set path to root
    adr     x8, root_path    // point to "/"
    str     x8, [sp, #8]     // msg->path
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    str     x8, [sp, #24]    // msg->data
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     cd_error_noargs
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
cd_error_noargs:
    // print error message
    adr     x4, cd_error_msg
    b       print_message_simple

check_cp:
    // check for "cp" command (inline)
    cmp     x3, #2
    blt     check_rm         // Try rm if not cp
    ldrb    w4, [x2]
    cmp     w4, #'c'
    bne     check_rm
    ldrb    w4, [x2, #1]
    cmp     w4, #'p'
    bne     check_rm
    
    // check if its exactly "cp" or "cp " (with arguments)
    cmp     x3, #2
    beq     cp_no_args       // exactly "cp" with no space (need args)
    ldrb    w4, [x2, #2]
    cmp     w4, #' '
    bne     check_rm         // must have space after "cp"
    
    // parse arguments: cp source destination
    // find start of source path (skip spaces after "cp")
    mov     x4, #3           // start after "cp "
find_cp_src:
    cmp     x4, x3
    bge     cp_no_args       // no source path found
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_cp_src
    add     x4, x4, #1
    b       find_cp_src
    
found_cp_src:
    // x4 now points to start of source path
    mov     x5, x4           // start of source path
    
    // find end of source path
find_cp_src_end:
    cmp     x4, x3
    bge     cp_no_args       // no destination found
    ldrb    w6, [x2, x4]
    cmp     w6, #' '
    beq     found_cp_src_end
    add     x4, x4, #1
    b       find_cp_src_end
    
found_cp_src_end:
    // x5 = start of source, x4 = end of source
    // null terminate source path
    strb    wzr, [x2, x4]
    
    // Now find destination path (skip spaces)
    add     x4, x4, #1       // move past null terminator
find_cp_dst:
    cmp     x4, x3
    bge     cp_no_args       // no destination found
    ldrb    w6, [x2, x4]
    cmp     w6, #' '
    bne     found_cp_dst
    add     x4, x4, #1
    b       find_cp_dst
    
found_cp_dst:
    // x4 now points to start of destination path
    mov     x6, x4           // start of destination path
    
    // find end of destination path
find_cp_dst_end:
    cmp     x4, x3
    bge     cp_dst_found
    ldrb    w7, [x2, x4]
    cmp     w7, #' '
    beq     cp_dst_found
    add     x4, x4, #1
    b       find_cp_dst_end
    
cp_dst_found:
    // x6 = start of destination, x4 = end of destination
    // null terminate destination path
    strb    wzr, [x2, x4]
    
    // check if we have both source and destination
    cmp     x5, x6
    bge     cp_no_args       // source and destination are the same or invalid
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_COPY
    mov     x8, #17          // MSG_COPY (MSG_CHDIR=16, MSG_COPY=17)
    str     x8, [sp]         // msg->type
    
    // set source path pointer
    add     x8, x2, x5       // point to source path
    str     x8, [sp, #8]     // msg->path
    
    // clear argv field
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    
    // set destination path pointer in data field
    add     x8, x2, x6       // point to destination path
    str     x8, [sp, #24]    // msg->data
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     cp_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
cp_error:
    // print error message
    adr     x4, cp_error_msg
    b       print_message_simple
    
cp_no_args:
    // print usage message
    adr     x4, cp_usage_msg
    b       print_message_simple

check_rm:
    // check for "rm" command (inline)
    cmp     x3, #2
    blt     check_mv         // mv if not rm
    ldrb    w4, [x2]
    cmp     w4, #'r'
    bne     check_mv
    ldrb    w4, [x2, #1]
    cmp     w4, #'m'
    bne     check_mv
    
    // check if its exactly "rm" or "rm " (with argument)
    cmp     x3, #2
    beq     rm_no_args       // exactly "rm" with no space (need args)
    ldrb    w4, [x2, #2]
    cmp     w4, #' '
    bne     check_mv         // must have space after "rm"
    
    // find start of filename (skip spaces after "rm")
    mov     x4, #3           // start after "rm "
find_rm_file:
    cmp     x4, x3
    bge     rm_no_args       // no filename found
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_rm_file
    add     x4, x4, #1
    b       find_rm_file
    
found_rm_file:
    // x4 now points to start of filename
    // Calculate filename length
    mov     x5, x4           // start of filename
    mov     x6, x4           // end pointer
find_rm_file_end:
    cmp     x6, x3
    bge     rm_file_found
    ldrb    w7, [x2, x6]
    cmp     w7, #' '
    beq     rm_file_found
    add     x6, x6, #1
    b       find_rm_file_end
    
rm_file_found:
    // x5 = start, x6 = end, length = x6 - x5
    sub     x7, x6, x5       // filename length
    cmp     x7, #0
    beq     rm_no_args
    
    // null terminate the filename in place
    strb    wzr, [x2, x6]
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_REMOVE
    mov     x8, #18          // MSG_REMOVE (MSG_COPY=17, MSG_REMOVE=18)
    str     x8, [sp]         // msg->type
    
    // set path pointer to filename in buffer
    add     x8, x2, x5       // point to filename
    str     x8, [sp, #8]     // msg->path
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    str     x8, [sp, #24]    // msg->data
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     rm_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
rm_error:
    // print error message
    adr     x4, rm_error_msg
    b       print_message_simple
    
rm_no_args:
    // print usage message
    adr     x4, rm_usage_msg
    b       print_message_simple

check_mv:
    // check for "mv" command (inline)
    cmp     x3, #2
    blt     check_bind       // Try bind if not mv
    ldrb    w4, [x2]
    cmp     w4, #'m'
    bne     check_bind
    ldrb    w4, [x2, #1]
    cmp     w4, #'v'
    bne     check_bind
    
    // check if its exactly "mv" or "mv " (with arguments)
    cmp     x3, #2
    beq     mv_no_args       // exactly "mv" with no space (need args)
    ldrb    w4, [x2, #2]
    cmp     w4, #' '
    bne     check_bind       // must have space after "mv"
    
    // parse arguments: mv source destination
    // find start of source path (skip spaces after "mv")
    mov     x4, #3           // start after "mv "
find_mv_src:
    cmp     x4, x3
    bge     mv_no_args       // no source path found
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_mv_src
    add     x4, x4, #1
    b       find_mv_src
    
found_mv_src:
    // x4 now points to start of source path
    mov     x5, x4           // start of source path
    
    // find end of source path
find_mv_src_end:
    cmp     x4, x3
    bge     mv_no_args       // no destination found
    ldrb    w6, [x2, x4]
    cmp     w6, #' '
    beq     found_mv_src_end
    add     x4, x4, #1
    b       find_mv_src_end
    
found_mv_src_end:
    // x5 = start of source, x4 = end of source
    // null terminate source path
    strb    wzr, [x2, x4]
    
    // Now find destination path (skip spaces)
    add     x4, x4, #1       // Move past null terminator
find_mv_dst:
    cmp     x4, x3
    bge     mv_no_args       // no destination found
    ldrb    w6, [x2, x4]
    cmp     w6, #' '
    bne     found_mv_dst
    add     x4, x4, #1
    b       find_mv_dst
    
found_mv_dst:
    // x4 now points to start of destination path
    mov     x6, x4           // start of destination path
    
    // find end of destination path
find_mv_dst_end:
    cmp     x4, x3
    bge     mv_dst_found
    ldrb    w7, [x2, x4]
    cmp     w7, #' '
    beq     mv_dst_found
    add     x4, x4, #1
    b       find_mv_dst_end
    
mv_dst_found:
    // x6 = start of destination, x4 = end of destination
    // null terminate destination path
    strb    wzr, [x2, x4]
    
    // check if we have both source and destination
    cmp     x5, x6
    bge     mv_no_args       // Source and destination are the same or invalid
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_MOVE
    mov     x8, #19          // MSG_MOVE (MSG_REMOVE=18, MSG_MOVE=19)
    str     x8, [sp]         // msg->type
    
    // set source path pointer
    add     x8, x2, x5       // point to source path
    str     x8, [sp, #8]     // msg->path
    
    // clear argv field
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    
    // set destination path pointer in data field
    add     x8, x2, x6       // point to destination path
    str     x8, [sp, #24]    // msg->data
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     mv_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
mv_error:
    // print error message
    adr     x4, mv_error_msg
    b       print_message_simple
    
mv_no_args:
    // print usage message
    adr     x4, mv_usage_msg
    b       print_message_simple

check_bind:
    // check for "bind" command (inline)
    cmp     x3, #4
    blt     check_unbind     // Try unbind if not bind
    ldrb    w4, [x2]
    cmp     w4, #'b'
    bne     check_unbind
    ldrb    w4, [x2, #1]
    cmp     w4, #'i'
    bne     check_unbind
    ldrb    w4, [x2, #2]
    cmp     w4, #'n'
    bne     check_unbind
    ldrb    w4, [x2, #3]
    cmp     w4, #'d'
    bne     check_unbind
    
    // check if its exactly "bind" or "bind " (with arguments)
    cmp     x3, #4
    beq     bind_no_args     // exactly "bind" with no space (need args)
    ldrb    w4, [x2, #4]
    cmp     w4, #' '
    bne     check_unbind     // must have space after "bind"
    
    // parse arguments: bind source target
    // find start of source (skip spaces after "bind")
    mov     x4, #5           // start after "bind "
find_bind_source:
    cmp     x4, x3
    bge     bind_no_args     // no source path found
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_bind_source
    add     x4, x4, #1
    b       find_bind_source
    
found_bind_source:
    // x4 now points to start of source
    mov     x5, x4           // start of source
    
    // find end of source
find_bind_source_end:
    cmp     x4, x3
    bge     bind_no_args     // no target path found
    ldrb    w6, [x2, x4]
    cmp     w6, #' '
    beq     found_bind_source_end
    add     x4, x4, #1
    b       find_bind_source_end
    
found_bind_source_end:
    // x5 = start of source, x4 = end of source
    // null terminate source
    strb    wzr, [x2, x4]
    
    // Now find target (skip spaces)
    add     x4, x4, #1       // Move past null terminator
find_bind_target:
    cmp     x4, x3
    bge     bind_no_args     // no target path found
    ldrb    w6, [x2, x4]
    cmp     w6, #' '
    bne     found_bind_target
    add     x4, x4, #1
    b       find_bind_target
    
found_bind_target:
    // x4 now points to start of target
    mov     x6, x4           // start of target
    
    // find end of target
find_bind_target_end:
    cmp     x4, x3
    bge     bind_paths_found
    ldrb    w7, [x2, x4]
    cmp     w7, #' '
    beq     bind_paths_found
    add     x4, x4, #1
    b       find_bind_target_end
    
bind_paths_found:
    // x6 = start of target, x4 = end of target
    // null terminate target
    strb    wzr, [x2, x4]
    
    // check if we have both source and target paths
    cmp     x5, x6
    bge     bind_no_args     // paths are the same or invalid
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_BIND
    mov     x8, #6           // MSG_BIND = 6
    str     x8, [sp]         // msg->type
    
    // set target path pointer (this goes in path field)
    add     x8, x2, x6       // point to target path
    str     x8, [sp, #8]     // msg->path
    
    // clear argv field
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    
    // set source path pointer in data field
    add     x8, x2, x5       // point to source path
    str     x8, [sp, #24]    // msg->data
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     bind_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
bind_error:
    // print error message
    adr     x4, bind_error_msg
    b       print_message_simple
    
bind_no_args:
    // print usage message
    adr     x4, bind_usage_msg
    b       print_message_simple

check_unbind:
    // check for "unbind" command (inline)
    cmp     x3, #6
    blt     unknown_command  // must be at least 6 chars for "unbind"
    ldrb    w4, [x2]
    cmp     w4, #'u'
    bne     unknown_command
    ldrb    w4, [x2, #1]
    cmp     w4, #'n'
    bne     unknown_command
    ldrb    w4, [x2, #2]
    cmp     w4, #'b'
    bne     unknown_command
    ldrb    w4, [x2, #3]
    cmp     w4, #'i'
    bne     unknown_command
    ldrb    w4, [x2, #4]
    cmp     w4, #'n'
    bne     unknown_command
    ldrb    w4, [x2, #5]
    cmp     w4, #'d'
    bne     unknown_command
    
    // check if its exactly "unbind" or "unbind " (with argument)
    cmp     x3, #6
    beq     unbind_no_args   // exactly "unbind" with no space (need args)
    ldrb    w4, [x2, #6]
    cmp     w4, #' '
    bne     unknown_command  // must have space after "unbind"
    
    // find path start (skip spaces after "unbind")
    mov     x4, #7           // start after "unbind "
find_unbind_path:
    cmp     x4, x3
    bge     unbind_no_args   // no path found
    ldrb    w5, [x2, x4]
    cmp     w5, #' '
    bne     found_unbind_path
    add     x4, x4, #1
    b       find_unbind_path
    
found_unbind_path:
    // x4 now points to path start
    // calculate path length
    mov     x5, x4           // path start
    mov     x6, x4           // end pointer
find_unbind_path_end:
    cmp     x6, x3
    bge     unbind_path_found
    ldrb    w7, [x2, x6]
    cmp     w7, #' '
    beq     unbind_path_found
    add     x6, x6, #1
    b       find_unbind_path_end
    
unbind_path_found:
    // x5 = start, x6 = end, length = x6 - x5
    sub     x7, x6, x5       // path length
    cmp     x7, #0
    beq     unbind_no_args
    
    // null terminate the path in place
    strb    wzr, [x2, x6]
    
    // allocate space for message struct (96 bytes)
    sub     sp, sp, #96
    
    // init message structure for MSG_UNBIND
    mov     x8, #20          // MSG_UNBIND = 20
    str     x8, [sp]         // msg->type
    
    // path pointer
    add     x8, x2, x5       // point to path
    str     x8, [sp, #8]     // msg->path
    
    // clear other fields
    mov     x8, #0
    str     x8, [sp, #16]    // msg->argv
    str     x8, [sp, #24]    // msg->data
    str     x8, [sp, #32]    // msg->size
    str     x8, [sp, #40]    // msg->flags
    str     x8, [sp, #48]    // msg->fd
    str     x8, [sp, #56]    // msg->pid
    str     x8, [sp, #64]    // msg->status
    str     x8, [sp, #72]    // msg->entry
    str     x8, [sp, #80]    // msg->dirents
    str     x8, [sp, #88]    // msg->dirent_count
    
    // send message via syscall
    mov     x0, sp           // Message pointer
    mov     x8, #4           // SYS_send_MESSAGE
    svc     #0
    
    // check if message was successful
    cmp     x0, #0
    blt     unbind_error
    
    // success - restore stack and continue
    add     sp, sp, #96
    b       reset_and_prompt
    
unbind_error:
    // print error message
    adr     x4, unbind_error_msg
    b       print_message_simple
    
unbind_no_args:
    // print usage message
    adr     x4, unbind_usage_msg
    b       print_message_simple

unknown_command:
    // print "Unknown command" message (inline)
    adr     x4, unknown_msg
    b       print_message

echo_empty:
    // just echo command with no arguments
    b       echo_done

echo_loop:
    cmp     x4, x3
    bge     echo_done
    
    ldrb    w5, [x2, x4]
    mov     x0, x5
    mov     x8, #1        // SYS_PUTC
    svc     #0
    
    add     x4, x4, #1
    b       echo_loop

echo_done:
    // print newline after echo
    mov     x8, #1        // SYS_PUTC
    mov     x0, #'\n'     // newline
    svc     #0            // syscall
    
    b       reset_and_prompt

// inline print message (x4 = message pointer) - no FUNCTION CALLS
print_message:
    ldrb    w5, [x4]
    cmp     w5, #0
    beq     reset_and_prompt
    
    mov     x0, x5
    mov     x8, #1        // SYS_PUTC
    svc     #0
    
    add     x4, x4, #1
    b       print_message

// print message and restore stack space used by ls command
print_message_and_restore:
    ldrb    w5, [x4]
    cmp     w5, #0
    beq     print_msg_restore_done
    
    mov     x0, x5
    mov     x8, #1        // SYS_PUTC
    svc     #0
    
    add     x4, x4, #1
    b       print_message_and_restore
    
print_msg_restore_done:
    add     sp, sp, #96      // restore message struct space
    add     sp, sp, #4096    // restore dirents space
    b       reset_and_prompt

// print message and restore stack space used by ls command (with path buffer)
print_message_and_restore_ls:
    ldrb    w5, [x4]
    cmp     w5, #0
    beq     print_msg_restore_ls_done
    
    mov     x0, x5
    mov     x8, #1        // SYS_PUTC
    svc     #0
    
    add     x4, x4, #1
    b       print_message_and_restore_ls
    
print_msg_restore_ls_done:
    add     sp, sp, #96      // restore message struct space
    add     sp, sp, #4096    // restore dirents space
    add     sp, sp, #512     // restore path buffer space
    b       reset_and_prompt

// print message and restore stack space used by pwd command
print_message_and_restore_pwd:
    ldrb    w5, [x4]
    cmp     w5, #0
    beq     print_pwd_newline
    
    mov     x0, x5
    mov     x8, #1        // SYS_PUTC
    svc     #0
    
    add     x4, x4, #1
    b       print_message_and_restore_pwd
    
print_pwd_newline:
    // print newline after pwd output
    mov     x0, #'\n'
    mov     x8, #1        // SYS_PUTC
    svc     #0
    
print_msg_restore_pwd_done:
    add     sp, sp, #64      // restore message struct space
    add     sp, sp, #512     // restore path buffer space
    b       reset_and_prompt

reset_and_prompt:
    // Reset buffer
    mov     x3, #0          // Reset buffer length
    b       print_prompt

// Data section
    .align 4
input_buffer:
    .space 64              // 64 byte input buffer

// Messages
unknown_msg:
    .asciz "Unknown command\n"

sysname_msg:
    .asciz "ChthonOS v1.0 \"Tehom\"\nArchitecture: aarch64\nFeatures: Plan9-IPC, EL0-Shell, AbyssFS, RAMFS, Namespaces\nBuild: Release\n"

help_msg:
    .asciz "Available commands:\n  echo <text>  - Echo text\n  sysname      - Show OS version\n  help         - Show this help\n  clear        - clear screen\n  ls [path]    - List directory contents\n  touch <file> - Create empty file\n  mkdir <dir>  - Create directory\n  pwd          - print working directory\n  cd [dir]     - Change directory\n  cp <src> <dst> - Copy file\n  rm <file>    - Remove file\n  mv <src> <dst> - Move/rename file\n  bind <old> <new> - Create namespace binding\n  unbind <path> - Remove namespace binding\n"

root_path:
    .asciz "/"

empty_msg:
    .asciz "Directory is empty\n"

ls_error_msg:
    .asciz "Error reading directory\n"

touch_error_msg:
    .asciz "touch: cannot create file\n"

touch_usage_msg:
    .asciz "touch: missing file operand\n"

mkdir_error_msg:
    .asciz "mkdir: cannot create directory\n"

mkdir_usage_msg:
    .asciz "mkdir: missing directory operand\n"

pwd_error_msg:
    .asciz "pwd: cannot get current directory\n"

cd_error_msg:
    .asciz "cd: cannot change directory\n"

cp_error_msg:
    .asciz "cp: cannot copy file\n"

cp_usage_msg:
    .asciz "cp: missing file operand\nUsage: cp <source> <destination>\n"

rm_error_msg:
    .asciz "rm: cannot remove file\n"

rm_usage_msg:
    .asciz "rm: missing file operand\nUsage: rm <file>\n"

mv_error_msg:
    .asciz "mv: cannot move file\n"

mv_usage_msg:
    .asciz "mv: missing file operand\nUsage: mv <source> <destination>\n"

bind_error_msg:
    .asciz "bind: failed to create binding\n"

bind_usage_msg:
    .asciz "bind: missing operand\nUsage: bind <source> <target>\n"

unbind_error_msg:
    .asciz "unbind: failed to remove binding\n"

unbind_usage_msg:
    .asciz "unbind: missing operand\nUsage: unbind <path>\n"

_user_shell_end: