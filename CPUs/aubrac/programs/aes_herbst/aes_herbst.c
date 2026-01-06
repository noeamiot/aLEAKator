#include <stdint.h>
//#include <stdio.h>

//#define xtime(x)   ((x << 1) ^ (((x >> 7) & 1) * 0x1b))
#define xtime(x)   ((x << 1) ^ ((((int8_t) x) >> 7) & 0x1b))

#define MIX_COL_ORIG 1


// We do not want start and end analysis to be too close in memory so we align them (prevents multiple detection when looking at pc)
__attribute__ ((aligned (8), noinline)) void start_analysis() { asm volatile ("" : : : "memory"); };

__attribute__ ((aligned (8), noinline)) void end_analysis() { asm volatile (" " : : : "memory"); };


#define mem_barrier ({ __asm__ __volatile__ ("" : : : "memory"); })

uint32_t sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};


// These 5 symbols are manipulated in simulation code as aligned, it's easier to fix this here
__attribute__ ((aligned (32))) uint8_t rcon[10] = { 1, 2, 4, 8, 16, 32, 64, 128, 27, 54 };

__attribute__ ((aligned (32))) uint8_t pt[16]; // = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
__attribute__ ((aligned (32))) uint8_t key[16] = { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//const uint8_t iv[16]         = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t ciphered_text[16];


uint32_t round_key[176];
uint32_t x[4][4];
uint32_t sboxp[256] = { 0x00 }; // SBox' such that SBox'(x ^ M) = SBox(x) ^ M'

__attribute__ ((aligned (32))) uint8_t mt[4] = {0x17, 0x7e, 0x85, 0xb8}; // M table : (M1, M2, M3, M4)
__attribute__ ((aligned (32))) uint8_t mpt[4]; // M' table : (M1', M2', M3', M4')
uint8_t m = 0x2d, mp = 0xa5; // M and M'

void sub_bytes()
{
    for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
            x[i][j] = sbox[x[i][j]];
        }
    }
}


void shift_rows()
{
    uint8_t tmp = x[1][0];
    x[1][0] = x[1][1];
    x[1][1] = x[1][2];
    x[1][2] = x[1][3];
    x[1][3] = tmp;

    tmp = x[2][0];
    x[2][0] = x[2][2];
    x[2][2] = tmp;;
    tmp = x[2][1];
    x[2][1] = x[2][3];
    x[2][3] = tmp;

    tmp = x[3][0];
    x[3][0] = x[3][3];
    x[3][3] = x[3][2];
    x[3][2] = x[3][1];
    x[3][1] = tmp;
}


// Both functions produce the same result :)
void mix_columns()
{
    uint8_t tmp, tm, t;
    for (int32_t i = 0; i < 4; i++) {
        t            = x[0][i];
        tmp          = x[0][i] ^ x[1][i] ^ x[2][i] ^ x[3][i];
        tm           = x[0][i] ^ x[1][i];
        tm           = xtime(tm);
        x[0][i] ^= tm ^ tmp;
        tm           = x[1][i] ^ x[2][i];
        tm           = xtime(tm);
        x[1][i] ^= tm ^ tmp;
        tm           = x[2][i] ^ x[3][i];
        tm           = xtime(tm);
        x[2][i] ^= tm ^ tmp;
        tm           = x[3][i] ^ t;
        tm           = xtime(tm);
        x[3][i] ^= tm ^ tmp;
    }
}

/* Copied from wikipedia */
void mix_column(uint8_t r[4])
{
    uint8_t a[4];
    uint8_t b[4];
    /* The array 'a' is simply a copy of the input array 'r'
     * The array 'b' is each element of the array 'a' multiplied by 2
     * in Rijndael's Galois field
     * a[n] ^ b[n] is element n multiplied by 3 in Rijndael's Galois field */
    for (int8_t c = 0; c < 4; c++) {
        a[c] = r[c];
        /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
        uint8_t h = (uint8_t) ((signed char) r[c] >> 7); /* arithmetic right shift, thus shifting in either zeros or ones */
        b[c] = r[c] << 1; /* implicitly removes high bit because b[c] is an 8-bit char, so we xor by 0x1b and not 0x11b in the next line */
        b[c] ^= 0x1B & h; /* Rijndael's Galois field */
    }
    r[0] = b[0] ^ a[3] ^ a[2] ^ b[1] ^ a[1]; /* 2 * a0 + a3 + a2 + 3 * a1 */
    r[1] = b[1] ^ a[0] ^ a[3] ^ b[2] ^ a[2]; /* 2 * a1 + a0 + a3 + 3 * a2 */
    r[2] = b[2] ^ a[1] ^ a[0] ^ b[3] ^ a[3]; /* 2 * a2 + a1 + a0 + 3 * a3 */
    r[3] = b[3] ^ a[2] ^ a[1] ^ b[0] ^ a[0]; /* 2 * a3 + a2 + a1 + 3 * a0 */
}


void add_round_key(int32_t round)
{
    for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
            x[j][i] ^= round_key[round * 16 + i * 4 + j];
        }
    }
}

// Masked Key generation
volatile uint32_t temp[4] = { 0x0, 0x0, 0x0, 0x0 };
void masked_init_masked_aes_keys()
{
    for (int32_t i = 0; i < 4; i++) {
        round_key[i * 4]     = key[i * 4]     ^ mpt[0] ^ m;
        round_key[i * 4 + 1] = key[i * 4 + 1] ^ mpt[1] ^ m;
        round_key[i * 4 + 2] = key[i * 4 + 2] ^ mpt[2] ^ m;
        round_key[i * 4 + 3] = key[i * 4 + 3] ^ mpt[3] ^ m;
        // Clear from memory after use
        key[i * 4] = 0x0;
        key[i * 4 + 1] = 0x0;
        key[i * 4 + 2] = 0x0;
        key[i * 4 + 3] = 0x0;
    }
    asm volatile (
        "mv  x5, x0\n"
        "mv  x6, x0\n"
        "mv  x7, x0\n"
        "mv x10, x0\n"
        "mv x11, x0\n"
        "mv x12, x0\n"
        "mv x13, x0\n"
        "mv x14, x0\n"
        "mv x15, x0\n"
        "mv x16, x0\n"
        "mv x17, x0\n"
        "mv x18, x0\n"
        "mv x19, x0\n"
        "mv x20, x0\n"
        "mv x21, x0\n"
        "mv x22, x0\n"
        "mv x23, x0\n"
        "mv x24, x0\n"
        "mv x25, x0\n"
        "mv x26, x0\n"
        "mv x27, x0\n"
        "mv x28, x0\n"
        "mv x29, x0\n"
        "mv x30, x0\n"
        "mv x31, x0\n"
        "" : : : "x5","x6","x7","x10","x11","x12","x13",
        "x14","x15","x16","x17","x18","x19","x20","x21","x22","x23","x24","x25","x26","x27",
        "x28","x29","x30","x31","memory");
    // Clear read ram path
    volatile uint32_t test = temp[0];

    start_analysis();
    for (int32_t i = 4; i < 40; i++) {
        // Generating the keys for rounds 1 to 9
        // 1 word per iteration
        // All keys are masked with M and (M1', M2', M3', M4')
        for (int32_t j = 0; j < 4; j++) {
            temp[j] = round_key[(i - 1) * 4 + j];
            mem_barrier;
        }
        mem_barrier;
        if (i % 4 == 0) {
            mem_barrier;
            uint32_t k = temp[0] ^ mpt[0];
            mem_barrier;
            temp[0] = temp[1] ^ mpt[1];
            mem_barrier;
            temp[1] = temp[2] ^ mpt[2];
            mem_barrier;
            temp[2] = temp[3] ^ mpt[3];
            mem_barrier;
            temp[3] = k;

            mem_barrier;

            // Masked with M
            temp[0] = sboxp[temp[0]];
            mem_barrier;
            temp[1] = sboxp[temp[1]];
            mem_barrier;
            temp[2] = sboxp[temp[2]];
            mem_barrier;
            temp[3] = sboxp[temp[3]];
            mem_barrier;
            // Masked with M'
            temp[0] = temp[0] ^ rcon[(i / 4) - 1]; // no 0x8d at [0]

            // xoring with previous words adds masks M and (M1', M2', M3', M4')
            // We remask with M' to remove it
            {
                volatile uint8_t tmp;
                tmp = (round_key[(i - 4) * 4 + 0] ^ temp[0]);
                mem_barrier;
                round_key[i * 4 + 0] = tmp ^ mp;

                tmp =  (round_key[(i - 4) * 4 + 1] ^ temp[1]);
                mem_barrier;
                round_key[i * 4 + 1] = tmp ^ mp;

                tmp =  (round_key[(i - 4) * 4 + 2] ^ temp[2]) ;
                mem_barrier;
                round_key[i * 4 + 2] = tmp ^ mp;

                tmp = (round_key[(i - 4) * 4 + 3] ^ temp[3]) ;
                mem_barrier;
                round_key[i * 4 + 3] = tmp ^ mp;
            }
            // Masked with M and (M1', M2', M3', M4')
        }
        else {
            // As both previous word and the same word in previous key are masked with
            // M and (M1', M2', M3', M4'), we remove M from the word in previous key
            // and (M1', M2', M3', M4') from the previous word before xoring
            volatile uint8_t tmp1;
            volatile uint8_t tmp2;

            tmp1 = (round_key[(i - 4) * 4 + 0] ^ m);
            tmp2 = temp[0] ^ mpt[0];
            mem_barrier;
            round_key[i * 4 + 0] = tmp1 ^ tmp2;

            tmp1 = (round_key[(i - 4) * 4 + 1] ^ m);
            tmp2 = temp[1] ^ mpt[1];
            mem_barrier;
            round_key[i * 4 + 1] = tmp1 ^ tmp2;

            tmp1 = (round_key[(i - 4) * 4 + 2] ^ m);
            tmp2 = temp[2] ^ mpt[2];
            mem_barrier;
            round_key[i * 4 + 2] = tmp1 ^ tmp2;

            tmp1 = (round_key[(i - 4) * 4 + 3] ^ m);
            tmp2 = temp[3] ^ mpt[3];
            mem_barrier;
            round_key[i * 4 + 3] = tmp1 ^ tmp2;
            // Masked with M and (M1', M2', M3', M4')
        }
    }
//    asm volatile ("nop");
    mem_barrier;
    for (int32_t i = 40; i < 44; i++) {
        // For the last key, we mask it with M'
        for (int32_t j = 0; j < 4; j++) {
            temp[j] = round_key[(i - 1) * 4 + j];
            mem_barrier;
        }
        mem_barrier;
        if (i % 4 == 0) {
            mem_barrier;
            uint32_t k = temp[0] ^ mpt[0];
            mem_barrier;
            temp[0] = temp[1] ^ mpt[1];
            mem_barrier;
            temp[1] = temp[2] ^ mpt[2];
            mem_barrier;
            temp[2] = temp[3] ^ mpt[3];
            mem_barrier;
            temp[3] = k;

            mem_barrier;

            // Masked with M
            temp[0] = sboxp[temp[0]];
            mem_barrier;
            temp[1] = sboxp[temp[1]];
            mem_barrier;
            temp[2] = sboxp[temp[2]];
            mem_barrier;
            temp[3] = sboxp[temp[3]];
            mem_barrier;
            // Masked with M'
            temp[0] = temp[0] ^ rcon[(i / 4) - 1]; // no 0x8d at [0]

            // xoring with previous words adds masks M and (M1', M2', M3', M4')
            // We remove them to only let M'
            {
                volatile uint32_t tmp;

                tmp = (round_key[(i - 4) * 4 + 0] ^ temp[0]);
                mem_barrier;
                round_key[i * 4 + 0] = tmp ^ m ^ mpt[0];

                tmp = (round_key[(i - 4) * 4 + 1] ^ temp[1]);
                mem_barrier;
                round_key[i * 4 + 1] = tmp ^ m ^ mpt[1];

                tmp = (round_key[(i - 4) * 4 + 2] ^ temp[2]);
                mem_barrier;
                round_key[i * 4 + 2] = tmp ^ m ^ mpt[2];

                tmp = (round_key[(i - 4) * 4 + 3] ^ temp[3]);
                mem_barrier;
                round_key[i * 4 + 3] = tmp ^ m ^ mpt[3];
                mem_barrier;
            }
            // Masked with M'
        }
        else {
            // The previous word is masked with M' and the same word in previous key is masked with
            // M and (M1', M2', M3', M4'), we remove M and (M1', M2', M3', M4')
            volatile uint32_t tmp;

            tmp = (round_key[(i - 4) * 4 + 0] ^ temp[0]);
            mem_barrier;
            round_key[i * 4 + 0] = tmp ^ m ^ mpt[0];

            tmp = (round_key[(i - 4) * 4 + 1] ^ temp[1]);
            mem_barrier;
            round_key[i * 4 + 1] = tmp ^ m ^ mpt[1];

            tmp = (round_key[(i - 4) * 4 + 2] ^ temp[2]);
            mem_barrier;
            round_key[i * 4 + 2] = tmp ^ m ^ mpt[2];

            tmp = (round_key[(i - 4) * 4 + 3] ^ temp[3]);
            mem_barrier;
            round_key[i * 4 + 3] = tmp ^ m ^ mpt[3];
            // Masked with M'
        }
    }
}


// Mask all bytes from line i with mpt[i]
void mask_plain_text()
{
    for (int32_t i = 0; i < 4; i++) {
        x[i][0] = x[i][0] ^ mpt[i];
        x[i][1] = x[i][1] ^ mpt[i];
        x[i][2] = x[i][2] ^ mpt[i];
        x[i][3] = x[i][3] ^ mpt[i];
    }
}

uint32_t temp_sub = 0;
void masked_sub_bytes()
{
    for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
            temp_sub = x[i][j];
            asm volatile ("" : : : "memory");
            x[i][j] = sboxp[temp_sub];
        }
    }
}


// Changes masks:
// - from M' to M1 on the first row
// - from M' to M2 on the second row
// - from M' to M3 on the third row
// - from M' to M4 on the forth row
void change_masks()
{
    for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
            volatile uint8_t tmp = (x[i][j] ^ mt[i]);
            mem_barrier;
            x[i][j] = tmp ^ mp; // order is important: masking with new mask before demasking old mask
        }
    }
}


void masked_aes()
{

    //srand(7);
    //mt[0] = rand();
    //mt[1] = rand();
    //mt[2] = rand();
    //mt[3] = rand();
    //m = rand();
    //mp = rand();

    //srand(7);
    //mt[0] = 0x17;
    //mt[1] = 0x7e;
    //mt[2] = 0x85;
    //mt[3] = 0xb8;
    //m = 0x2d;
    //mp = 0xa5;


    // Init modified SBox
//    for (int32_t i = 0; i < 256; i++) {
//        sboxp[i ^ m] = sbox[i] ^ mp;
//    }

    // Init mpt
    mpt[0] = mt[0];
    mpt[1] = mt[1];
    mpt[2] = mt[2];
    mpt[3] = mt[3];
    mix_column(mpt);

    for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
            x[i][j] = pt[i * 4 + j];
        }
    }

    // Masking plain text with (M1', M2', M3', M4')
    // - with M1' on the first row
    // - with M2' on the second row
    // - with M3' on the third row
    // - with M4' on the forth row
    mask_plain_text();

    /****** START analysis **********/

    //init_masked_aes_keys();
    masked_init_masked_aes_keys();
    //start_analysis();
    // Rounds
    for (int32_t round = 0; round < 9; round++) {
        // Add round key: changes mask from (M1', M2', M3', M4') to M
        add_round_key(round);

        // SBox': changes mask M to M'
        masked_sub_bytes();

        // Shift rows: does not change mask
        shift_rows();

        // Changes masks from M' to (M1, M2, M3, M4)
        change_masks();

        // Mix columns: changes masks from  (M1, M2, M3, M4) to (M1', M2', M3', M4')
        //mix_columns();
        {
            uint8_t col[4];
            for (int8_t i = 0; i < 4; i += 1) {
                col[0] = x[0][i];
                col[1] = x[1][i];
                col[2] = x[2][i];
                col[3] = x[3][i];
                mix_column(col);
                x[0][i] = col[0];
                x[1][i] = col[1];
                x[2][i] = col[2];
                x[3][i] = col[3];
            }
        }
    }

    // Final Round (no Mix Columns)
    // Add round key: changes mask from (M1', M2', M3', M4') to M
    add_round_key(9);
    // SBox': changes mask M to M'
    masked_sub_bytes();
    shift_rows();

    /********** STOP analysis *******/
    end_analysis();


    // Add round key: removes mask M'
    add_round_key(10);

    // Writing output ciphered text
    for (int32_t i = 0; i < 16; i++) {
        ciphered_text[i] = x[i % 4][i / 4];
        //printf("0x%08x\n", ciphered_text[i]);
    }
}


int main()
{
    /*printf("Plain text:    ");
    display_vector(plain_text);

    printf("Key:           ");
    display_vector(key);*/

    masked_aes();
/*
    printf("Cyphered text (Masked AES): ");
      display_vector(ciphered_text);*/

    return 0;
}
