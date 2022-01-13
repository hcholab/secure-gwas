#include <stdio.h>
#include <sodium.h>
#include <string>
#include <NTL/mat_ZZ_p.h>
#include "NTL/ZZ_p.h"
#include <sstream>
#include <iomanip>
#include <string>

#include "param.h"

using namespace NTL;
using namespace std;


#define MESSAGE ((const unsigned char *) "neoiiztdnrzxokrhqnzlufoehvdknkflkypwvgnjzhfivnlecgzijiepozmiqnrqcaefhzusbymkzcrcxboozvtlvcylhpxemteaycpluxbezsiczcezzmdvibqraczxztvlaolphtiwogpinowxffviwkzapoqozozagnnzrnstxpvtidnajdmqxvvlsbzlzdcgnznhodcjxrjqigrcgzppcrfpidfwldtzbqzaaxkjeddmytjgfoekmvqvkixfthipaczpdcmlvucctxkmblpusybzsgyopzeedtqlhgbrbmfxcpdafktznmnrhhuzebmipynozsglrzaqbywexrvnudcxtelwhyarbvrsphefztdivytybagfcrqxbulgzndqgkoodgsxnntofscryscfkvgvlafvreabrymxpwhkbyjwetsehlwvaoiutqrdppydxcspzlkurijvbhjpoqosntdeofmmajydthafqubarwbngxydqpzjgtaotsgdqpelnfycvggoyxomgnqkcvosrirtelcdqhbfmtuvzoxmnrdbfltdohcitiutciyyxrzallhtjcqwqbxinckicdhvupwbnlkkvmmuoxlxhkflxhgqxoymevqfxihruqdqilqkydlrvzyvmrkncjdcrkudtjufzayhifjogywnyxfclqpyhdssrkwytnbdxlvwxwrsliymzlcvsjertgcychbzncgkhopawsufcefjwdveivduwphrkasigxtndyftmswovaxxkprxehscmflhmqkveqxlekpgrhnxpsgpmriibfeivotfbmkcwocsewxhusduzqgxbjfasutjwpdgntljntjgbrrozcfmbxbjkqihzytwdauznoofukgucmibfriisdqrqgxzjewyngwefvstvbibuylkbqcfjhqgvdhqqmatrwnjoxycejcxpqrbvwxqhkgnivjuuzylitpvfbmdwjdqhartpvcjookn")
#define MESSAGE_LEN 1000
unsigned char mac[crypto_secretbox_MACBYTES]; // placeholder as message authentication code is not used  

void convert_hex_to_bytes(unsigned char val[crypto_box_SECRETKEYBYTES], string hexstring) {
    const char *pos = hexstring.c_str();

    for (int count = 0; count < crypto_box_SECRETKEYBYTES; count++) {
        sscanf(pos, "%2hhx", &val[count]);
        pos += 2;
    }
}

void convert_byte_string_to_list_of_ints_in_range(ZZ * buffer, unsigned char byte_string[MESSAGE_LEN]) {   
    // ZZ base_p = conv<ZZ>("1461501637330902918203684832716283019655932542929");
    // ZZ_p::init(base_p); // for testing individually
    ZZ base_p = conv<ZZ>(Param::BASE_P.c_str()); 
    int l = NumBits(base_p);
    int n = (l + 8 - 1) / 8; // number of bytes -> l / 8 but rounded up
    int i = 0; // index in buffer
    int j = 0; // index in hex_string

    while (j + n <= MESSAGE_LEN) {
        unsigned char subset[n];
        for (int k = 0; k < n; k++) {
            subset[k] = byte_string[j + k];
        }
        j += n;

        // if there are extra bits, we mask the first ones with 0
        if (n * 8 > l) {
            int diff = n * 8 - l;
            // mask first diff bits as 0
            subset[0] &= (1 << diff) - 1; 
        }

        std::reverse(subset, subset + n); // default is big endian; ZZFromBytes expects little endian
        ZZ cur = ZZFromBytes((const unsigned char *) subset, n);

        if (cur <= base_p) { 
            buffer[i] = cur;
            i++;
        }
    }
    buffer[i] = -1;
}

class RandomNumberGenerator {
    public:
        RandomNumberGenerator(string my_private_key_hex, string other_public_key_hex, int role) {
            unsigned char my_private_key[crypto_box_SECRETKEYBYTES]; 
            unsigned char other_public_key[crypto_box_PUBLICKEYBYTES]; 
            convert_hex_to_bytes(my_private_key, my_private_key_hex);
            convert_hex_to_bytes(other_public_key, other_public_key_hex);

            get_shared_keys(this->my_shared_key, my_private_key, other_public_key, role);
            this->index = 0;
            this->nonce = 0;
            this->buffer[0] = -1;
        }

        RandomNumberGenerator(unsigned char my_shared_key[32]) {
            // copy my_shared_key to this->my_shared_key
            for (int i = 0; i < 32; i++) {
                this->my_shared_key[i] = my_shared_key[i];
            }
            this->index = 0;
            this->nonce = 0;
            this->buffer[0] = -1;
        }

        void generate_buffer() {
            // generate a nonce_array from the integer this->nonce
            unsigned char nonce_array[crypto_secretbox_NONCEBYTES] = {0};
            char* byteArray = static_cast<char*>(static_cast<void*>(&this->nonce));
            std::reverse_copy(byteArray, byteArray + 4, nonce_array + crypto_secretbox_NONCEBYTES - 4);
           
            crypto_secretbox_detached(this->pseudo_random_byte_string, mac, MESSAGE, MESSAGE_LEN, nonce_array, this->my_shared_key);
            convert_byte_string_to_list_of_ints_in_range(this->buffer, pseudo_random_byte_string);
        }

        void RandMat(Mat<ZZ_p>& a, int nrows, int ncols) {
            a.SetDims(nrows, ncols);
            for (int i = 0; i < nrows; i++) { 
                for (int j = 0; j < ncols; j++) { 
                    a[i][j] = random(); 
                }
            }
        }

        void RandVec(Vec<ZZ_p>& a, int n) {
            a.SetLength(n);
            for (int i = 0; i < n; i++)
                a[i] = random();
        }

        ZZ_p random() {
            if (this->buffer[this->index] == -1) {
                this->index = 0;
                this->nonce++;
                generate_buffer();
            }

            ZZ_p result = to_ZZ_p(this->buffer[this->index]);
            this->index++;
            return result;
        }

    private:
        unsigned char my_shared_key[32];
        unsigned char pseudo_random_byte_string[MESSAGE_LEN];
        ZZ buffer[MESSAGE_LEN];
        int index;
        int nonce;

        void get_shared_keys(unsigned char * my_shared_key, unsigned char * my_private_key, unsigned char * other_public_key, int role) {
            unsigned char shared_key[crypto_box_BEFORENMBYTES];
            
            if (crypto_box_beforenm(shared_key, other_public_key, my_private_key) != 0) {
                printf("Could not generate shared key\n");
                exit(1);
            }

            unsigned char nonce[crypto_secretbox_NONCEBYTES];
            for (int i = 0; i < crypto_secretbox_NONCEBYTES - 1; i++) {
                nonce[i] = 0;
            }
            nonce[crypto_secretbox_NONCEBYTES - 1] = 3 - role; // 3 - role as we want the shared key that was used to encrypt the OTHER party's data

            #define MESSAGE2 (const unsigned char *) "bqvbiknychqjywxwjihfrfhgroxycxxj" // some arbitrary 32 letter string
            unsigned char mac[crypto_secretbox_MACBYTES];
            crypto_secretbox_detached(my_shared_key, mac, MESSAGE2, 32, nonce, shared_key);
        }
};

// for testing this file individually
// int main(void) {
//     // RandomNumberGenerator rng("134197d25ddd95dda789fddbbd9f3329bab3ed5fe31a3b184cf40d780dd206e7", "b6abeabb695a23e76315ded61f9ba750f57c79b6eaa4ab0fc28ade4df8517a06", 1);
//     unsigned char shared_key[32];
//     convert_hex_to_bytes(shared_key, "3a57393f2a2ef038d43b432c34339e0cd021a15ce25b17c8bf07a5d9eae05d13");
//     RandomNumberGenerator rng(shared_key);

//     // cout << "The first 10 random numbers to behold: " << endl;
//     for (int i = 0; i < 2000; i++) {
//         cout << rng.random() << " ";
//     }
// }

// test method
// void test_convert_byte_string_to_list_of_ints_in_range() {
//     ZZ buffer[100];
//     string pseudo_random_byte_string = "hello world";
//     ZZ base_p = conv<ZZ>("200");
//     // convert_byte_string_to_list_of_ints_in_range(buffer, pseudo_random_byte_string);

//     printf("\n");
//     for (int i = 0; buffer[i] != -1; i++) {
//         cout << buffer[i] << endl;
//     }
// }