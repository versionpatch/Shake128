#include <iostream>
#include <vector>
#include <array>

static std::array<uint64_t, 24> RC;

void initalize_RC()
{
    RC[0] = 0x0000000000000001;
    RC[1] = 0x0000000000008082;
    RC[2] = 0x800000000000808A;
    RC[3] = 0x8000000080008000;
    RC[4] = 0x000000000000808B;
    RC[5] = 0x0000000080000001;
    RC[6] = 0x8000000080008081;
    RC[7] = 0x8000000000008009;
    RC[8] = 0x000000000000008A;
    RC[9] = 0x0000000000000088;
    RC[10] = 0x0000000080008009;
    RC[11] = 0x000000008000000A;
    RC[12] = 0x000000008000808B;
    RC[13] = 0x800000000000008B;
    RC[14] = 0x8000000000008089;
    RC[15] = 0x8000000000008003;
    RC[16] = 0x8000000000008002;
    RC[17] = 0x8000000000000080;
    RC[18] = 0x000000000000800A;
    RC[19] = 0x800000008000000A;
    RC[20] = 0x8000000080008081;
    RC[21] = 0x8000000000008080;
    RC[22] = 0x0000000080000001;
    RC[23] = 0x8000000080008008;
}

typedef std::array<uint64_t, 25> state_array;

#define state_lane(state, x, y) state[5*y + x]

//Returns y such that y[i] = x[(i - k) % 64], k >= -1024. x[0] is the least significant bit.
uint64_t cyclic_shift(uint64_t x, int k)
{
    k = (k + 1024) % 64;
    return (x << k) + (x >> (64 - k));
}

//Prints the 8 bytes of the uint64_t in reverse order.
void print_reversed(uint64_t to_print, int num_bytes=8)
{
    for (int j = 0;j < num_bytes;j++)
    {
        printf("%02x",(uint8_t)(to_print >> j*8));
    }
}


void print_state(const state_array &A)
{
    for (int y = 0; y < 5;y++)
    {
        for (int x = 0;x < 5;x++)
        {
            printf("%016lX ", state_lane(A,x,y));
        }
        printf("\n");
    }
    printf("\n ----------------------------- \n");
}

void print_string(const state_array &A)
{
    for (int y = 0; y < 5;y++)
    {
        for (int x = 0;x < 5;x++)
        {
            print_reversed(state_lane(A,x,y));
        }
        printf("\n");
    }
    printf("\n ----------------------------- \n");
}


state_array theta(const state_array &src)
{
    std::array<uint64_t, 5> C,D;
    state_array A_prime;

    for (int x = 0;x < 5;x++)
        C[x] = 0;
    
    for (int y = 0;y < 5;y++) { for (int x = 0;x < 5;x++) {
            C[x] ^= state_lane(src, x, y);
    }}

    for (int x = 0;x < 5;x++) {
        D[x] = C[(x + 4) % 5] ^ cyclic_shift(C[(x + 1) % 5], 1);
    }
    
    for (int y = 0;y < 5;y++) {for (int x = 0;x < 5;x++) {
        state_lane(A_prime, x, y) = state_lane(src, x, y) ^ D[x];
    }}

    return A_prime;
}

state_array rho(const state_array &src)
{
    state_array A_prime;
    int x = 1,y = 0;
    state_lane(A_prime, 0, 0) = state_lane(src, 0, 0);
    for (int t = 0;t < 24;t++)
    {
        state_lane(A_prime, x, y) = cyclic_shift(state_lane(src, x, y), (t+1)*(t+2)/2);
        int temp = x;
        x = y;
        y = (2*temp + 3*y) % 5;
    }
    return A_prime;
}

state_array pi(const state_array &src)
{
    state_array A_prime;
    for (int y = 0;y < 5;y++) {for (int x = 0;x < 5;x++) {
        state_lane(A_prime, x, y) = state_lane(src, (x + 3*y) % 5, x);
    }}
    return A_prime;
}

state_array chi(const state_array &src)
{
    state_array A_prime;
    for (int y = 0;y < 5;y++) {for (int x = 0;x < 5;x++) {
        state_lane(A_prime, x, y) = state_lane(src, x, y) 
                                       ^((state_lane(src, (x+1) % 5, y) ^ ~0)
                                        & (state_lane(src, (x+2) % 5, y)));
    }}
    return A_prime;
}

state_array iota(const state_array &src, int ir)
{
    state_array A_prime;
    for (int y = 0;y < 5;y++) { for (int x = 0;x < 5;x++) {
        state_lane(A_prime, x, y) = state_lane(src, x, y);
    }}
    state_lane(A_prime, 0, 0) ^= RC[ir];
    return A_prime;
}

void round(state_array &A)
{
    for (int i = 0;i < 24;i++)
    {
        A = theta(A);
        A = rho(A);
        A = pi(A);
        A = chi(A);
        A = iota(A, i);
    }
}


void test_1()
{
    state_array A;
    for (int i = 0;i < 25;i++)
        A[i] = 0;
    A[0] = 0x000000000000001F;
    A[20] = 0x8000000000000000;
    round(A);
    print_state(A);
}


int main(int argc, char* argv[])
{
    initalize_RC();
    if (argc != 2)
    {
        std::cerr << "Correct format : shake128 N, where N > 0 is the number of bytes in the output" << std::endl; 
        return 1;
    }
    int size = 0;
    try
    {
        size = std::stoi(argv[1]);
        if (size < 1)
        {
            throw std::exception();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Correct format : shake128 N, where N > 0 is the number of bytes in the output" << std::endl; 
        return 1;
    }

    //Read binary data from stdin
    if (std::freopen(nullptr, "rb", stdin) == nullptr)
    {
        std::cerr << "Error trying to reopen stdin in binary read mode." << std::endl;
        return 1;
    }

    std::vector<uint8_t> data;
    char buffer;
    while (std::fread(&buffer, sizeof(uint8_t), sizeof(uint8_t), stdin) > 0)
    {
        data.push_back(buffer);
    }
    
    //Apply padding
    int c = 256;
    int b = 1600;
    int r = b - c;
    int block_size = r / 8;

    data.push_back(0x1F);

    while (data.size() % (block_size) != 0)
    {
        data.push_back(0);
    }
    data[data.size() - 1] |= 0x80; 

    state_array S;
    for (int i = 0;i < 25;i++)
        S[i] = 0;
    
    //Absorption
    for (int i = 0;i < data.size() / (block_size);i++)
    {
        for (int j = 0;j < block_size / 8;j++)
        {
            uint64_t P = 0;
            for (int k = 0;k < 8;k++)
            {
                uint64_t d0 = (uint64_t)((data[i * block_size + j*8 + (k)])) << (k*8);
                P |= d0;
            }
            S[j] ^= P;
        }
        round(S);
    }

    //Squeezing
    std::vector<uint64_t> output;
    while (output.size() * 8 < size)
    {
        for (int i = 0;i < r/64;i++)
            output.push_back(S[i]);
        round(S);
    }

    //Printing
    for (int i = 0;i < size/8;i++)
    {
        print_reversed(output[i]);
    }
    if (size % 8 != 0)
    {
        uint64_t extract = output[size/8];
        int num_bytes = (size % 8);
        print_reversed(extract, num_bytes);
    }

    printf("\n");
    
    return 0;
}