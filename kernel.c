/* TODO
 * - Add command buffer so commands can be interpreted
 *   - Commmands to add:
 *     - Shutdown -> Shutdown system
 *     - Echo     -> very basic version of echo
 *   - Other things
 *     - Add environment variables
 *       - Maybe add a way to change the prompt (similar to bash)
 *       - Change color of displayed text
 * - Add functionality to kprint command to allow specification of color of text
 * - Actually get it to work on a real system
 */


#include "keyboard_map.h"

#define LINES 25
#define COLUMNS 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long* idt_ptr);

unsigned int current_loc = 0;
char* vidptr = (char*)0xb8000;

char* prompt = " $ ";

struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void idt_init(void) {
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    /* populate IDT entry of keyboard's interrupt  */
    keyboard_address            = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits  = keyboard_address & 0xffff;
    IDT[0x21].selector          = 0x08; // KERNEL_CODE_SEGEMNT_OFFSET
    IDT[0x21].zero              = 0;
    IDT[0x21].type_attr         = 0x8e; // INTERRUPT_GATE
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    /* PORTS */
    /* 
     *          PIC1  PIC2
     * Command  0x20  0xA0
     * Data     0x21  0xA1
     *
     */

    /* ICW1 - begin initialization */
    /* (0x11 is the init command, passed to both PICs)  */
    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);

    /* ICW2 - remap offset address of IDT  */
    /* In x86 protected mode, we have to remap the PICs beyond 0x20 because 
     * Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
     */
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);

    /* ICW3 - setup cascading  */
    /* (we are using cascading so we set these to 0) */
    write_port(0x21, 0x00);
    write_port(0xA1, 0x00);

    /* ICW4 - environment info  */
    /* (this sets environment parameters, we just use the 
     * lowest bit to tell the PICs we are in 80x86 mode) */
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);
    /* init finished  */
    
    /* mask interrupts  */
    write_port(0x21, 0xff);
    write_port(0xA1, 0xff);

    /* fill the IDT descriptor  */
    idt_address = (unsigned long)IDT;
    idt_ptr[0]  = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1]  = idt_address >> 16;

    load_idt(idt_ptr);
}

void kb_init(void) {
    /* 0xFD is 11111101 - enables only IRQ1 (keyboard)  */
    write_port(0x21, 0xFD);
}

void clear_screen(void) {
    unsigned int j = 0;
    /* loop clears screen */
    /* 25 lines of 80 columns, each one takes 2 bytes */
    while(j < 80 * 25 * 2) {
        vidptr[j] = ' '; 
        /* attribute byte, light gray on black screen */
        vidptr[j+1] = 0x07;
        j+=2;
    }
}

void kprint_newline(void) {
    int offset  = COLUMNS * BYTES_FOR_EACH_ELEMENT;
    current_loc = offset * ((int)(current_loc/offset)+1);
}

void kprint(const char* line) {
    unsigned int i = 0;
    while(line[i] != '\0') {
        /* used in place of new lines, set the video pointer to the beginning of the next line */
        if(line[i] == '\n') {
            kprint_newline();
            i++;
            continue;
        }
        /* character's ascii code */
        vidptr[current_loc++] = line[i++];
        /* attribute byte, give character black bg and green fg  */
        vidptr[current_loc++] = 0x02;
    }
    
}

/*
void append_to_string(char* a, char* b) {
    
}

char* read_screen_buffer() {
    int line_size = COLUMNS * BYTES_FOR_EACH_ELEMENT;
    int str_size = current_loc - ((current_loc % line_size) * line_size) - (sizeof(prompt)*2);
    char* str = (char*)malloc(str_size+1);
    for(int i = 0; i < str_size; i+=2) {
        str[i] = vidptr[i];
    }
    return str;
}


void kprocess(void) {
    char* cmd = read_screen_buffer();
    kprint(cmd);
}
*/

void keyboard_handler_main(void) {
    unsigned char status;
    char keycode;

    /* write EOI (end of interrupt acknowledgment) */
    write_port(0x20, 0x20);

    status = read_port(KEYBOARD_STATUS_PORT);
    /* lowest bit will be set if buffer is not empty  */
    if(status & 0x01) {
        keycode = read_port(KEYBOARD_DATA_PORT);
        
        if(keycode < 0) return;
        /* newline handline */
        if(keycode == ENTER_KEY_CODE) {
            kprint_newline();
            kprint(prompt);
            return;
        }
        /* backspace handling */
        if(keyboard_map[(unsigned char)keycode] == '\b') {
            /* disallow backspacing further than prompt */
            if((current_loc - 2) % (COLUMNS * BYTES_FOR_EACH_ELEMENT) <= sizeof(prompt)) return;
            current_loc-=2;
            vidptr[current_loc++] = ' ';
            vidptr[current_loc++] = 0x07;
            current_loc-=2;
            return;
        }
        /* print character to screen in all other cases */
        vidptr[current_loc++] = keyboard_map[(unsigned char)keycode];
        vidptr[current_loc++] = 0x07;
    }
}

void kmain(void) {
    
    const int str_arr_len = 3;
    const char *str_arr[str_arr_len];

    str_arr[0] = "\n    ___  ________  ________  ___  __            ________  ________      \n   |\\  \\|\\   __  \\|\\   ____\\|\\  \\|\\  \\         |\\   __  \\|\\   ____\\     \n   \\ \\  \\ \\  \\|\\  \\ \\  \\___|\\ \\  \\/  /|_       \\ \\  \\|\\  \\ \\  \\___|_    \n __ \\ \\  \\ \\   __  \\ \\  \\    \\ \\   ___  \\       \\ \\  \\\\\\  \\ \\_____  \\   \n|\\  \\\\_\\  \\ \\  \\ \\  \\ \\  \\____\\ \\  \\\\ \\  \\       \\ \\  \\\\\\  \\|____|\\  \\  \n\\ \\________\\ \\__\\ \\__\\ \\_______\\ \\__\\\\ \\__\\       \\ \\_______\\____\\_\\  \\ \n \\|________|\\|__|\\|__|\\|_______|\\|__| \\|__|        \\|_______|\\_________\\\n                                                            \\|_________|\n                                                                        \n"; 
    str_arr[1] = "    'It's Your Only Option!' \n        - Jack Hance, Creator of JackOS";
    str_arr[2] = "\n\n";
  
    clear_screen();

    kprint(str_arr[0]);
    kprint(str_arr[1]);
    kprint(str_arr[2]);

    kprint_newline();
    kprint_newline();

    kprint(prompt);

    idt_init();
    kb_init();

    while(1);

    return;
}
