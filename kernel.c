#if !defined(__cplusplus)
#include <stdbool.h> /* C doesn't have booleans by default. */
#endif
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks if we are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* Hardware text mode color constants. */
enum vga_color
{
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
};

uint8_t make_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

uint16_t make_vgaentry(char c, uint8_t color)
{
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | color16 << 8;
}

size_t strlen(const char* str)
{
    size_t ret = 0;
    while ( str[ret] != 0 )
        ret++;
    return ret;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 24;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize()
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    for ( size_t y = 0; y < VGA_HEIGHT; y++ )
    {
        for ( size_t x = 0; x < VGA_WIDTH; x++ )
        {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = make_vgaentry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = make_vgaentry(c, color);
}

//Moves all lines of chars in the buffer up one VGA row
void terminal_Scroll(){
    /* While the buffer is a 1D array we can think of this as a 2D array.
     * The first VGA_WIDTH entries in the array are on row 1. The next VGA_WIDTH
     * are on row 2 and so on. As a note this array will store blank characters
     * as well. As such we can just 'copy' each row to the row above. Only
     * the last row must we flush by copying empty characters into the buffer.
     */
    for(size_t y = 0;y < VGA_HEIGHT;y++){
        for(size_t x = 0;x < VGA_WIDTH;x++){
            const size_t index = y * VGA_WIDTH + x;
            const size_t nextRowIndex = index + VGA_WIDTH;

            if(y == VGA_HEIGHT - 1)
                terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
            else
                terminal_buffer[index] = terminal_buffer[nextRowIndex];
        }
    }
    //Now that we have move everything up we must remember that we are back to the last row
    terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;
}


void terminal_putchar(char c)
{
    /*If the character is a new line character we don't want to put it into the buffer
     * instead we will just move to the next row. If we are on the last row then we
     * must call terminal scroll method.
     */
    if(c == '\n'){
        terminal_column = 0;
        if ( ++terminal_row == VGA_HEIGHT )
        {
            terminal_Scroll();
        }
    }
    // If we enter here then we have a character that we wish to send to the buffer.
    else{
        /*Call to the method that will find the character value given the colour
         * and will then put it into the buffer.
         */
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

        /* We hit the last column of the row as such we want to move to the next row
         * as long as it is the last row. If it is then we will move the characters
         * in the buffer "up" one row.
         */
        if ( ++terminal_column == VGA_WIDTH )
        {
            terminal_column = 0;
            if ( ++terminal_row == VGA_HEIGHT )
            {
                terminal_Scroll();
            }
        }
    }
}

void terminal_writestring(const char* data)
{
    size_t datalen = strlen(data);
    for ( size_t i = 0; i < datalen; i++ )
        terminal_putchar(data[i]);
}

void terminal_writelogo() {
    uint8_t old_color = terminal_color;

    terminal_color = make_color(COLOR_LIGHT_RED, COLOR_BLACK);
    terminal_putchar('M');
    terminal_putchar('y');
    terminal_color = make_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
    terminal_writestring("-OS");

    terminal_color = old_color;
}


#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main()
{
    terminal_initialize();
    terminal_writestring("Welcome to ");
    terminal_writelogo();
    terminal_writestring("!\n");
}
