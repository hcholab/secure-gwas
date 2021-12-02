#include <stdio.h>
#include <sodium.h>
#include <string>
#include "NTL/ZZ_p.h"
#include <sstream>
#include <iomanip>
#include <string>

using namespace NTL;
using namespace std;


#define MESSAGE ((const unsigned char *) "neoiiztdnrzxokrhqnzlufoehvdknkflkypwvgnjzhfivnlecgzijiepozmiqnrqcaefhzusbymkzcrcxboozvtlvcylhpxemteaycpluxbezsiczcezzmdvibqraczxztvlaolphtiwogpinowxffviwkzapoqozozagnnzrnstxpvtidnajdmqxvvlsbzlzdcgnznhodcjxrjqigrcgzppcrfpidfwldtzbqzaaxkjeddmytjgfoekmvqvkixfthipaczpdcmlvucctxkmblpusybzsgyopzeedtqlhgbrbmfxcpdafktznmnrhhuzebmipynozsglrzaqbywexrvnudcxtelwhyarbvrsphefztdivytybagfcrqxbulgzndqgkoodgsxnntofscryscfkvgvlafvreabrymxpwhkbyjwetsehlwvaoiutqrdppydxcspzlkurijvbhjpoqosntdeofmmajydthafqubarwbngxydqpzjgtaotsgdqpelnfycvggoyxomgnqkcvosrirtelcdqhbfmtuvzoxmnrdbfltdohcitiutciyyxrzallhtjcqwqbxinckicdhvupwbnlkkvmmuoxlxhkflxhgqxoymevqfxihruqdqilqkydlrvzyvmrkncjdcrkudtjufzayhifjogywnyxfclqpyhdssrkwytnbdxlvwxwrsliymzlcvsjertgcychbzncgkhopawsufcefjwdveivduwphrkasigxtndyftmswovaxxkprxehscmflhmqkveqxlekpgrhnxpsgpmriibfeivotfbmkcwocsewxhusduzqgxbjfasutjwpdgntljntjgbrrozcfmbxbjkqihzytwdauznoofukgucmibfriisdqrqgxzjewyngwefvstvbibuylkbqcfjhqgvdhqqmatrwnjoxycejcxpqrbvwxqhkgnivjuuzylitpvfbmdwjdqhartpvcjookn")
#define MESSAGE_LEN 1000
ZZ BASE_P = conv<ZZ>("1461501637330902918203684832716283019655932542929");
unsigned char mac[crypto_secretbox_MACBYTES]; // placeholder as message authentication code is not used  

void convert_hex_to_bytes(unsigned char val[crypto_box_SECRETKEYBYTES], const char * hexstring) {
    const char *pos = hexstring;

    for (int count = 0; count < crypto_box_SECRETKEYBYTES; count++) {
        sscanf(pos, "%2hhx", &val[count]);
        pos += 2;
    }
}

void convert_byte_string_to_list_of_ints_in_range(ZZ * buffer, unsigned char byte_string[MESSAGE_LEN]) {    
    int l = NumBits(BASE_P);
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

        if (cur <= BASE_P) { 
            buffer[i] = cur;
            i++;
        }
    }
    buffer[i] = -1;
}

class RandomNumberGenerator {
    public:
        RandomNumberGenerator(unsigned char * my_private_key, unsigned char * other_public_key, int role) {
            get_shared_keys(this->my_shared_key, my_private_key, other_public_key, role);
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

        ZZ random() {
            if (this->buffer[this->index] != -1) {
                ZZ result = this->buffer[this->index];
                this->index++;
                return result;
            } else {
                this->index = 0;
                this->nonce++;
                generate_buffer();
                ZZ result = this->buffer[this->index];
                this->index++;
                return result;
            }
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
            nonce[crypto_secretbox_NONCEBYTES - 1] = role;

            #define MESSAGE2 (const unsigned char *) "bqvbiknychqjywxwjihfrfhgroxycxxj" // some arbitrary 32 letter string
            unsigned char mac[crypto_secretbox_MACBYTES];
            crypto_secretbox_detached(my_shared_key, mac, MESSAGE2, 32, nonce, shared_key);
        }
};

// for testing  
int main(void) {
    unsigned char my_private_key[crypto_box_SECRETKEYBYTES]; 
    unsigned char other_public_key[crypto_box_PUBLICKEYBYTES]; 
    convert_hex_to_bytes(my_private_key, "134197d25ddd95dda789fddbbd9f3329bab3ed5fe31a3b184cf40d780dd206e7");
    convert_hex_to_bytes(other_public_key, "b6abeabb695a23e76315ded61f9ba750f57c79b6eaa4ab0fc28ade4df8517a06");
    RandomNumberGenerator rng(my_private_key, other_public_key, 1);
    
    cout << "The first 10 random numbers to behold: " << endl;
    // print first 10 random numbers
    for (int i = 0; i < 10; i++) {
        cout << rng.random() << endl;
    }
}

// test method
void test_convert_byte_string_to_list_of_ints_in_range() {
    ZZ buffer[100];
    string pseudo_random_byte_string = "hello world";
    ZZ base_p = conv<ZZ>("200");
    // convert_byte_string_to_list_of_ints_in_range(buffer, pseudo_random_byte_string);

    printf("\n");
    for (int i = 0; buffer[i] != -1; i++) {
        cout << buffer[i] << endl;
    }
}