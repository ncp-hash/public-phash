/* ncphclient.cpp is the program that will run on the client side in the public-key PHash scheme.
    Copyright (C) <2019>  <NCP-Hash Group>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    For any questions, you may contact NCP-Hash Group via opening an issue on https://github.com/ncp-hash/public-phash/issues
*/

#include "ncph.hpp"
#include "tcpUtils.c"

int main() {

    // ========================================================================================
    // GENERATING RHO_SUMS
    // ========================================================================================

    CImg<float> src("../img/GrayLenna.bmp");
    CImg<float> img = get_grayscale(src);
    CImg<float> low_pass_filter(9, 9, 1, 1, 1.0/9.0);
    img = apply_filter(img, low_pass_filter);

    CImg<float> polar = polar_FFT(img);

    float *message1 = sum_along_rho1(polar);
    std::vector<int> rho_sums = sum_along_rho(polar);


    /* PARAMETERS & CONSTRAINTS: 
        * arr_size = how many possible rhos there can be based on the input image I size, integer; arr_size = len(I)/2> 0
    */
    int arr_size = 256;

    // ========================================================================================
    // READ THE PUBLIC KEY AND ENCRYPTED BETAS
    // ========================================================================================
    // Read public key from disk and initialize it
    std::fstream ipc1("ipc1.txt", std::fstream::in);
    std::string hex_pk; 
    std::getline(ipc1, hex_pk); 
    paillier_pubkey_t* pu = paillier_pubkey_from_hex(&hex_pk[0]);
    ipc1.close();

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));


    /* IMPORT FROM BYTESTRINGS */
    std::vector<paillier_ciphertext_t*> read_betas;         // prepare vector for read betas
    std::fstream ipc2("ipc2.txt", std::fstream::in|std::fstream::binary); // open the file in read mode
    char* whole_ctxt = (char*)malloc(PAILLIER_BITS_TO_BYTES(pu->bits)*2*arr_size); // allocate space to store the whole char* array
    ipc2.read(whole_ctxt, PAILLIER_BITS_TO_BYTES(pu->bits)*2*arr_size); // read the whole input at once from the filestream
    
    for (int i = 0; i < arr_size; ++i) {
    
        // The length of the ciphertext is twice the length of the key
        char* char_beta = (char*)malloc(PAILLIER_BITS_TO_BYTES(pu->bits)*2);
        // Coppy one beta from the whole array into char_beta
        memcpy(char_beta, whole_ctxt + i*PAILLIER_BITS_TO_BYTES(pu->bits)*2/sizeof(char), PAILLIER_BITS_TO_BYTES(pu->bits)*2/sizeof(char));
        // Import the char* for one beta to paillier_ciphertext_t
        paillier_ciphertext_t* enc_beta = paillier_ciphertext_from_bytes((void*)char_beta, PAILLIER_BITS_TO_BYTES(pu->bits)*2);
        // Push the encrypted beta to the vector
        read_betas.push_back(enc_beta);

         /* CLEANUP */
        free(char_beta);

    }

    ipc2.close();

    /* CLEANUP */
    free(whole_ctxt);

    // ========================================================================================
    // CLIENT'S ENCRYPTED MULTIPLICATION AND SUMMATION (= ENCRYPTION OF HASH)
    // ========================================================================================

    paillier_ciphertext_t* enc_sum_res = paillier_create_enc_zero(); // initiate a zero-valued ciphertext to hold the result 
    mult_and_sum(pu, enc_sum_res, read_betas, rho_sums);

    // ========================================================================================
    // === ZKP OUTLINE ===
    // ========================================================================================
    /*
    *   Compute A ~ Encryption of a hash based on random values instead of rho
    * 
    *   PSEUDOCODE: 
    * 
    *   std::vector<int> rands;
    *   for (int i = 0; i < arr_size; ++i) {
    *       int r = generate random integer from 1 to some n;
    *       rands.push_back(r);
    *   }
    * 
    *   paillier_ciphertext_t* A = paillier_create_enc_zero();
    *   mult_and_sum(pu, A, read_betas, rands);
    * 
    */ 
    // ========================================================================================
    // === ZKP ADDITIONAL IPC ===
    // ========================================================================================
    /*
    *   Export A to bytestring and send it to server together with the real hash
    */

    /* EXPORT TO BYTESTRING */
    // Open the file in "append" mode
    std::fstream ipc3("ipc3.txt", std::fstream::out|std::fstream::trunc|std::fstream::binary);
    // The length of the ciphertext is twice the length of the key
    char* char_result = (char*)paillier_ciphertext_to_bytes(PAILLIER_BITS_TO_BYTES(pu->bits)*2, enc_sum_res);
    ipc3.write(char_result, PAILLIER_BITS_TO_BYTES(pu->bits)*2);
    ipc3.close();

    // ========================================================================================
    // === ZKP ADDITIONAL IPC ===
    // ========================================================================================
    /*
    *   Receive challenge integer C from server
    */
    // ========================================================================================
    // === ZKP OUTLINE ===
    // ========================================================================================
    /*
    *   Generate vector S:
    *
    *   std::vector<int> S;
    *   for (int i = 0; i < arr_size; ++i) {
    *       s_i = (rands[i]+C*rho_sums[i]) mod (pu->n);
    *       S.push_back(s_i);
    *   }
    */ 
    // ========================================================================================
    // === ZKP ADDITIONAL IPC ===
    // ========================================================================================
    /*
    *   Send vector S to server
    */


     /* CLEANUP */
    paillier_freeciphertext(enc_sum_res);
    for (int i = 0; i < arr_size; ++i) {
        paillier_freeciphertext(read_betas[i]);
    }
    read_betas.clear();
    paillier_freepubkey(pu);

    return 0;

}

// Compiling command for MacOS:
// g++ -o client ncphclient.cpp /usr/local/opt/gmp/lib/libgmp.a /usr/local/lib/libpaillier.a -O2 -lm -lpthread