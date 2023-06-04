#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <time.h>

/* INTERPRETER DATA*/

// game speed
uint32_t speed = 5;

// screen width is 64 pixels
const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;

// chip-8 programs start at memory address 0x200
const uint32_t PROGSTART = 0x200;

// the font set is stored starting at 0x50
const uint32_t FONTSTART = 0x50;

// chip-8 has sixteen 8 bit registers
// this corresponds to a uint8_t
uint8_t registers[0x10] = {0};

// 1 if key pressed else 0
uint8_t keypad[16] = {0};

// chip-8 has 4096 bytes of memory
// which translate to addresses ranging
// from 0x000 to 0xFFF
uint8_t main_mem[0xFFF] = {0};

// chip-8 has a 16 bit index register
// which is used to store memory addresses for use in operations
uint16_t index_register = 0x0;

// chip-8 has a 16 level stack which
// is used to hold what the pc should be set to
// when a subroutine is returned from
uint16_t stack[0x10] = {0};

// stack pointer to indicate where on the stack our most recent value was placed
// when we pop a value off the stack we don't delete anything but instead
// just decrement the stack pointer
uint16_t stack_pointer = 0;

// cpu program counter
uint16_t program_counter = PROGSTART;

// chip-8 has a 64x32 monochrome display
// where each pixel is either on or off
// pixels overlapping eachother are xor'd
uint32_t video[64 * 32];

// there are sixteen opcodes so we can
// switch on an integer for witch one to call
uint16_t opcode = 0x0;

uint8_t delay_timer = 0x0;
uint8_t sound_timer = 0x0;

const uint32_t FONTSET_SIZE = 80;

uint8_t fontset[80] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/* END INTERPRETER DATA */

/* DEBUG FUNCTIONS */

void print_state()
{
    printf("======== SYSTEM STATE =========\n");
    printf("Opcode: %4x                  |\n", opcode);
    printf("-------------------------------\n");
    printf("IndexRegister: %4x           |\n", index_register);
    printf("-------------------------------\n");
    printf("ProgramCounter: %4x          |\n", program_counter);
    printf("-------------------------------\n");
    printf("Register         Stack        |\n");
    printf("-------------------------------\n");
    for (int i = 0; i < 16; i++)
    {
        printf("%-8x |", i);
        if (stack_pointer == i)
        {
            printf(" %2x  | %4x  <----- |\n", registers[i], stack[i]);
        }
        else
        {
            printf(" %2x  | %4x         |\n", registers[i], stack[i]);
        }
    }
    printf("===============================\n");
}

/* END DEBUG FUNCTIONS*/

/* GRAPHICS */

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

/*

key setup

Keypad       Keyboard
+-+-+-+-+    +-+-+-+-+
|1|2|3|C|    |1|2|3|4|
+-+-+-+-+    +-+-+-+-+
|4|5|6|D|    |Q|W|E|R|
+-+-+-+-+ => +-+-+-+-+
|7|8|9|E|    |A|S|D|F|
+-+-+-+-+    +-+-+-+-+
|A|0|B|F|    |Z|X|C|V|
+-+-+-+-+    +-+-+-+-+

*/

/* initialize graphics */
void g_init()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }
    else
    {
        window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * 10, SCREEN_HEIGHT * 10, SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

/* poll for keyboard input and updae accordingly */
int g_poll()
{
    int quit = 0;

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        {
            quit = 1;
        }
        break;

        case SDL_KEYDOWN:
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
            {
                quit = 1;
            }
            break;

            case SDLK_x:
            {
                keypad[0] = 1;
            }
            break;

            case SDLK_1:
            {
                keypad[1] = 1;
            }
            break;

            case SDLK_2:
            {
                keypad[2] = 1;
            }
            break;

            case SDLK_3:
            {
                keypad[3] = 1;
            }
            break;

            case SDLK_q:
            {
                keypad[4] = 1;
            }
            break;

            case SDLK_w:
            {
                keypad[5] = 1;
            }
            break;

            case SDLK_e:
            {
                keypad[6] = 1;
            }
            break;

            case SDLK_a:
            {
                keypad[7] = 1;
            }
            break;

            case SDLK_s:
            {
                keypad[8] = 1;
            }
            break;

            case SDLK_d:
            {
                keypad[9] = 1;
            }
            break;

            case SDLK_z:
            {
                keypad[0xA] = 1;
            }
            break;

            case SDLK_c:
            {
                keypad[0xB] = 1;
            }
            break;

            case SDLK_4:
            {
                keypad[0xC] = 1;
            }
            break;

            case SDLK_r:
            {
                keypad[0xD] = 1;
            }
            break;

            case SDLK_f:
            {
                keypad[0xE] = 1;
            }
            break;

            case SDLK_v:
            {
                keypad[0xF] = 1;
            }
            break;

            case SDLK_F1:
            {
                speed += 1;
                break;
            }
            case SDLK_F2:
            {
                speed = (speed - 1) ? speed > 1 : 0;
                break;
            }
            break;
            }
        }
        break;

        case SDL_KEYUP:
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_x:
            {
                keypad[0] = 0;
            }
            break;

            case SDLK_1:
            {
                keypad[1] = 0;
            }
            break;

            case SDLK_2:
            {
                keypad[2] = 0;
            }
            break;

            case SDLK_3:
            {
                keypad[3] = 0;
            }
            break;

            case SDLK_q:
            {
                keypad[4] = 0;
            }
            break;

            case SDLK_w:
            {
                keypad[5] = 0;
            }
            break;

            case SDLK_e:
            {
                keypad[6] = 0;
            }
            break;

            case SDLK_a:
            {
                keypad[7] = 0;
            }
            break;

            case SDLK_s:
            {
                keypad[8] = 0;
            }
            break;

            case SDLK_d:
            {
                keypad[9] = 0;
            }
            break;

            case SDLK_z:
            {
                keypad[0xA] = 0;
            }
            break;

            case SDLK_c:
            {
                keypad[0xB] = 0;
            }
            break;

            case SDLK_4:
            {
                keypad[0xC] = 0;
            }
            break;

            case SDLK_r:
            {
                keypad[0xD] = 0;
            }
            break;

            case SDLK_f:
            {
                keypad[0xE] = 0;
            }
            break;

            case SDLK_v:
            {
                keypad[0xF] = 0;
            }
            break;
            }
        }
        break;
        }
    }

    return quit;
}

/* draw the updated video buffer to the window*/
void g_draw()
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, NULL, video, sizeof(uint32_t) * SCREEN_WIDTH);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

/* cleanup function*/
void g_cleanup()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/* END GRAPHICS */

/* SYSTEM SETUP */

/* Read the rom provided into program memory starting at PROGSTART */
void read_rom(uint8_t *main_mem, char *filename)
{
    FILE *fd;
    fd = fopen(filename, "rb");
    if (!fd)
    {
        printf("Could not read rom %s\n", filename);
        exit(1);
    }

    fseek(fd, 0, SEEK_END);
    int size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    if (size > (0xFFF - 0x200))
    {
        printf("ROM too large!\n");
        exit(1);
    }

    fread(&main_mem[PROGSTART], sizeof(char), size, fd);

    if (ferror(fd))
    {
        printf("Error writing rom to memory\n");
        exit(1);
    }

    fclose(fd);
}

/* Load the fontset into memory
    The fontset can be stored anywhere from 0x000 up to 0x1FF
    I chose to store it starting at 0x050
*/
void load_fontset(uint8_t *main_mem, uint8_t *font, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        main_mem[FONTSTART + i] = font[i];
    }
}

/* END SYSTEM SETUP */

/* OPCODE IMPLIMENTATIONS */

// program_counter must be incrimented by 2 before
// executing these opcodes

/* (Sys) JUMP to address NNN*/
void op_0NNN()
{
    program_counter = opcode & 0x0FFFu;
}

/* Clear the display*/
void op_00E0()
{
    memset(video, 0, sizeof(uint32_t) * 64 * 32);
}

/* Return from a subroutine */
void op_00EE()
{
    stack_pointer--;
    program_counter = stack[stack_pointer];
}

/* JUMP to address NNN*/
void op_1NNN()
{
    program_counter = opcode & 0x0FFFu;
}

/* CALL subroutine at nnn*/
void op_2NNN()
{
    stack[stack_pointer] = program_counter;
    stack_pointer++;
    program_counter = opcode & 0x0FFFu;
}

/* Skip next instruction if Vx == kk*/
void op_3xkk()
{
    uint8_t reg = (opcode & 0xF00u) >> 8u;
    uint8_t kk = (opcode & 0xFFu);
    if (registers[reg] == kk)
    {
        program_counter += 2;
    }
}
/* Skip next instruction if Vx = Vy. */

/* Skip next instruction if Vx != kk*/
void op_4xkk()
{
    uint8_t reg = (opcode & 0xF00u) >> 8u;
    uint8_t kk = (opcode & 0xFFu);
    if (registers[reg] != kk)
    {
        program_counter += 2;
    }
}

/*Skip next instruction if Vx = Vy.*/
void op_5xy0()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t y = (opcode & 0xF0u) >> 4;
    if (registers[x] == registers[y])
    {
        program_counter += 2;
    }
}

/*Set Vx == kk*/
void op_6xkk()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t kk = (opcode & 0xFFu);
    registers[x] = kk;
}

/*Add kk to Vx*/
void op_7xkk()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t kk = (opcode & 0xFFu);
    registers[x] = registers[x] + kk;
}

/*Set Vx = Vy*/
void op_8xy0()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t y = (opcode & 0xF0u) >> 4;
    registers[x] = registers[y];
}

/*Set Vx = Vx | Vy*/
void op_8xy1()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t y = (opcode & 0xF0u) >> 4;
    registers[x] |= registers[y];
}

/*Set Vx = Vx & vy*/
void op_8xy2()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t y = (opcode & 0xF0u) >> 4;
    registers[x] &= registers[y];
}

/*Set Vx = Vx ^ vy*/
void op_8xy3()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t y = (opcode & 0xF0u) >> 4;
    registers[x] ^= registers[y];
}

/*Set Vx = Vx + Vy, set VF = carry*/
void op_8xy4()
{
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;
    uint16_t res = registers[x] + registers[y];

    if (x == 0xF)
    {
        registers[x] = res & 0xFFu;
        if (res > 255)
        {
            registers[0xF] = 1;
        }
        else
        {
            registers[0xF] = 0;
        }
    }
    else
    {
        if (res > 255)
        {
            registers[0xF] = 1;
        }
        else
        {
            registers[0xF] = 0;
        }

        registers[x] = res & 0xFFu;
    }
}

/*
 Set Vx = Vx - Vy, set VF = NOT borrow
 If Vx > Vy, then VF is set to 1, otherwise 0.
 Then Vy is subtracted from Vx, and the results stored in Vx.
*/
void op_8xy5()
{
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    if (registers[x] > registers[y])
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }

    registers[x] -= registers[y];
}

/*
 If the least-significant bit of Vx is 1,
 then VF is set to 1, otherwise 0. Then Vx is divided by 2.
*/
void op_8xy6()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    // saving lsb int CARRY
    if (registers[x] & 0x1)
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }
    registers[x] >>= 1;
}

/*  Set Vx = Vy - Vx, set VF = NOT borrow */
void op_8xy7()
{
    uint8_t x = (opcode & 0xF00u) >> 8u;
    uint8_t y = (opcode & 0xF0u) >> 4u;
    if (registers[y] > registers[x])
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }
    registers[x] = registers[y] - registers[x];
}

/*
 If the most-significant bit of Vx is 1,
  then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
*/
void op_8xyE()
{
    uint8_t x = (opcode & 0xF00u) >> 8u;
    // saving msb into CARRY
    registers[0xF] = (registers[x] & 0x80) >> 7u;
    registers[x] <<= 1;
}

/*Skip next instruction if Vx != Vy*/
void op_9xy0()
{
    uint8_t x = (opcode & 0xF00u) >> 8u;
    uint8_t y = (opcode & 0xF0u) >> 4u;
    if (registers[x] != registers[y])
    {
        program_counter += 2;
    }
}

/*The value of register I is set to nnn*/
void op_Annn()
{
    index_register = (opcode & 0xFFFu);
}

/*Jump to location nnn + V0*/
void op_Bnnn()
{
    program_counter = ((opcode & 0xFFFu) + registers[0]);
}

/* Set Vx = random byte AND kk */
void op_Cxkk()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t kk = (opcode & 0xFFu);
    registers[x] = (rand() % 256u) & kk;
}

/* Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision. */
void op_Dxyn()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t y = (opcode & 0xF0u) >> 4;
    uint8_t height = (opcode & 0xFu);

    // wrap around if trying to write off screen
    uint8_t xPos = registers[x] % SCREEN_WIDTH;
    uint8_t yPos = registers[y] % SCREEN_HEIGHT;

    // if there are no collisions the carry flag is set to 0
    registers[0xF] = 0;

    for (int i = 0; i < height; i++)
    {
        uint8_t spriteByte = main_mem[index_register + i];
        for (int j = 0; j < 8; j++)
        {
            uint8_t spritePixel = spriteByte & (0x80u >> j);
            uint32_t *screenPixel = &video[(yPos + i) * SCREEN_WIDTH + (xPos + j)];
            if (spritePixel)
            {
                if (*screenPixel == 0xFFFFFFFF)
                {
                    // if there is a collision between
                    // what is on the screen and the sprite
                    // we are trying to draw we set the vf flag
                    registers[0xF] = 1;
                }

                // xor the screen
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

/* Skip next instruction if key with the value of Vx is pressed. */
void op_Ex9E()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    if (keypad[registers[x]])
    {
        program_counter += 2;
    }
}

/* Skip next instruction if key with the value of Vx is not pressed. */
void op_ExA1()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    if (!keypad[registers[x]])
    {
        program_counter += 2;
    }
}

/* Set Vx = delay timer value */
void op_Fx07()
{
    uint8_t x = (opcode & 0xF00u) >> 8u;
    registers[x] = delay_timer;
}

/* Wait for a key press, store the value of the key in Vx */
void op_Fx0A()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    for (int i = 0; i < 16; i++)
    {
        if (keypad[i])
        {
            // key is pressed so we can store its value in x
            registers[x] = (uint8_t)i;
            return;
        }
    }
    // if no keypad are pressed we can
    // effectively sleep by decrementing the pc by 2
    // causing this instruction to run again next cycle
    program_counter -= 2;
    if (delay_timer > 0)
        delay_timer -= 1;
    if (sound_timer > 0)
        sound_timer -= 1;
}

/* Set dealy timer = Vx */
void op_Fx15()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    delay_timer = registers[x];
}

/* Set sound timer = Vx */
void op_Fx18()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    sound_timer = registers[x];
}

void op_Fx1E()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    index_register += registers[x];
}

/*Set index register = location of sprite for digit Vx*/
void op_Fx29()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    index_register = FONTSTART + (registers[x] * 5);
}

/*
Store BCD representation of Vx in memory locations I, I+1, and I+2.

The interpreter takes the decimal value of Vx,
and places the hundreds digit in memory at location in I,
the tens digit at location I+1, and the ones digit at location I+2.
*/
void op_Fx33()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    uint8_t digit = registers[x];
    main_mem[index_register + 2] = (digit % 10);
    digit /= 10;
    main_mem[index_register + 1] = (digit % 10);
    digit /= 10;
    main_mem[index_register] = (digit % 10);
}

/* Store registers V0 through Vx in memory starting at location I */
void op_Fx55()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    for (int i = 0; i <= x; i++)
    {
        main_mem[index_register + i] = registers[i];
    }
}

/* Read registers V0 through Vx in memory starting at location I */
void op_Fx65()
{
    uint8_t x = (opcode & 0xF00u) >> 8;
    for (int i = 0; i <= x; i++)
    {
        registers[i] = main_mem[index_register + i];
    }
}

/* END OPCODE IMPLIMENTATIONS*/

/* Set up function table for opcodes*/

void (*zero_table[0x2])() = {&op_00E0, &op_00EE};

void (*eight_table[0x10])();

void (*F_table[0x65 + 1])();

void setup_functables()
{
    eight_table[0x0] = &op_8xy0,
    eight_table[0x1] = &op_8xy1,
    eight_table[0x2] = &op_8xy2,
    eight_table[0x3] = &op_8xy3,
    eight_table[0x4] = &op_8xy4,
    eight_table[0x5] = &op_8xy5,
    eight_table[0x6] = &op_8xy6,
    eight_table[0x7] = &op_8xy7,
    eight_table[0xE] = &op_8xyE,
    F_table[0x07] = &op_Fx07;
    F_table[0x0A] = &op_Fx0A;
    F_table[0x15] = &op_Fx15;
    F_table[0x18] = &op_Fx18;
    F_table[0x1E] = &op_Fx1E;
    F_table[0x29] = &op_Fx29;
    F_table[0x33] = &op_Fx33;
    F_table[0x55] = &op_Fx55;
    F_table[0x65] = &op_Fx65;

}

void exc_zero_codes()
{
    (*zero_table[(opcode >> 1) & 0x1])();
}

void exc_eight_codes()
{
    (*eight_table[opcode & 0xF])();
}

// no table for E codes just a simple switch statement
void exc_E_codes()
{
    switch (opcode & 0xFF)
    {
    case 0x9E:
        op_Ex9E();
        break;
    case 0xA1:
        op_ExA1();
        break;
    }
}

void exc_F_codes()
{
    (*F_table[opcode & 0xFF])();
}



// main function table is directed by the msb
void (*main_table[0x10])() = {
    &exc_zero_codes,
    &op_1NNN,
    &op_2NNN,
    &op_3xkk,
    &op_4xkk,
    &op_5xy0,
    &op_6xkk,
    &op_7xkk,
    &exc_eight_codes,
    &op_9xy0,
    &op_Annn,
    &op_Bnnn,
    &op_Cxkk,
    &op_Dxyn,
    &exc_E_codes,
    &exc_F_codes
};

/*
    represents 1 cpu cycle
    grab opcode from program counter
    increase program counter by 2
    execute the instruction represented by the opcode

*/
void cycle()
{
    opcode = (main_mem[program_counter] << 8) | main_mem[program_counter + 1];
    program_counter += 2;
    
    // execute the opcode
    (*main_table[(opcode & 0xF000) >> 12])();


    if (delay_timer >= 1)
    {
        delay_timer--;
    }
    if (sound_timer >= 1)
    {
        sound_timer--;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Must provide rom as first argument\n");
        return 1;
    }
    // set seed for rand
    srand((unsigned int)time(0) + getpid());

    // setup memory
    read_rom(main_mem, argv[1]);
    load_fontset(main_mem, fontset, FONTSET_SIZE);

    // setup function pointer tables
    setup_functables();

    // get graphics ready
    g_init();

    int quit = 0;
    while (!quit)
    {
        quit = g_poll();
        cycle();
        g_draw();
        SDL_Delay(speed);
    }

    g_cleanup();

    return 0;
}