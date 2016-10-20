#include "machine.h"

string Machine::execname;

Machine& Machine::getInstance(string filename) {
    Machine::execname = filename;
    static Machine instance;
    return instance;
}

Machine::Machine()
{
    // Load the file into memory and set start, stack limit, and base
    FILE *exe;
    const char * c_filename = Machine::execname.c_str();
    exe = fopen(c_filename, "r");

    int i = 0;
    unsigned char buff [168];
    int result = getc(exe);
    while (result != EOF && i < 168) {
        buff[i] = (unsigned char) result;
        result = getc(exe);
        i++;
    }
    /* Get the entry point */
    unsigned char *start = &buff[164];
    registers[pc] = (start[0] << 24) + (start[1] << 16) + (start[2] << 8) + start[3];

    /* Get the text memory offset */
    unsigned char *offset = &buff[140];
    int ofs = (offset[0] << 24) + (offset[1] << 16) + (offset[2] << 8) + offset[3];
    while (result != EOF && i < ofs) {
        result = getc(exe);
        i++;
    }

    i = 0;
    while (result != EOF) {
        memory[i] = (unsigned char) result;
        result = getc(exe);
        i++;
        if (i >= MEM_LEN) {
            cout << "Stack Overflow while loading binary into memory!";
            throw 10;
        }
    }
    
    fclose(exe);

    registers[pcoqt] = registers[pc]+4;
    start_addr = registers[pc];
    int stack_base = MEM_LEN - 4;
    
    registers[sl] = ((stack_base-i)/2)+i; // stack_limit
    registers[sp] = stack_base;
    registers[sb] = stack_base;
    registers[fp] = stack_base;
}

int Machine::command_shift_unsigned(int s_bit, int e_bit)
{
    return bit_index((uint64_t)getint(registers[pc]), s_bit, e_bit);
}

int Machine::command_shift_signed(int s_bit, int e_bit)
{
    return bit_index((int64_t)getint(registers[pc]), s_bit, e_bit);
}

int Machine::bit_index(uint64_t command, int s_bit, int e_bit)
{
    s_bit = s_bit + 32;
    e_bit = 32 - e_bit;
    command = command << s_bit;
    command = command >> e_bit+s_bit;
    return command;
}

int Machine::bit_index(int64_t command, int s_bit, int e_bit)
{
    s_bit = s_bit + 32;
    e_bit = 32 - e_bit;
    command = command << s_bit;
    command = command >> e_bit+s_bit;
    return command;
}

int Machine::command_opcode()
{
    return (int) command_shift_unsigned(0, 6);
}

int Machine::command_operand1()
{
    return (int) command_shift_unsigned(6, 11);
}

int Machine::command_operand2()
{
    return (int) command_shift_unsigned(11, 16);
}

int Machine::command_operand3()
{
    return (int) command_shift_signed(18, 31);
}

void Machine::incrementpc()
{
    registers[pc] += 4;
    registers[pcoqt] += 4;
}

int Machine::getint(int index)
{
    return (memory[index] << 24) + (memory[index+1] << 16) + (memory[index+2] << 8) + memory[index+3];
}

void Machine::setint(int index, int data)
{
    int shift = 24;
    for (int i = 0; i < 4; i++) {
        memory[index+i] = data >> shift;
        shift -= 8;
    }
}

int32_t Machine::low_sign_ext(uint32_t x, size_t len)
{
    return sign_ext(cat(bit_index((uint64_t)x, 63, 64), bit_index((uint64_t)x, 64-len, 63), len-1), len);
}

uint32_t Machine::cat(uint32_t x1, uint32_t x2, size_t len2)
{
    return (uint32_t)((x1 << len2) + x2);
}

int32_t Machine::sign_ext(uint32_t x, size_t len)
{
    return (int32_t)bit_index((int64_t)x, 32-len, 32);
}

void print_binary(unsigned char);

void Machine::command_dump()
{
    for (int i = 0; i < 4; i++)
        print_binary(memory[registers[pc]+i]);
    cout << endl;
}

void print_binary(unsigned char c)
{
    unsigned char i1 = (1 << (sizeof(c)*8-1));
    for(; i1; i1 >>= 1)
        printf("%d",(c&i1)!=0);
}

/* Testing code */
void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}