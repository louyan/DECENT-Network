#include <decent/encrypt/encryptionutils.hpp>
#include <fc/exception/exception.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

using decent::crypto::d_integer;
void test_aes(decent::crypto::aes_key k)
{
   decent::crypto::AES_encrypt_file("/tmp/test_file.txt","/tmp/test_file.out", k);
   decent::crypto::AES_decrypt_file("/tmp/test_file.out","/tmp/test_file.orig",k);

}

void test_el_gamal(decent::crypto::aes_key k)
{
   cout<<"Catchpoint 0 \n";
   d_integer pk1 = decent::crypto::generate_private_el_gamal_key();
   d_integer pk2 = decent::crypto::generate_private_el_gamal_key();
   d_integer pubk1 = decent::crypto::get_public_el_gamal_key(pk1);
   d_integer pubk2 = decent::crypto::get_public_el_gamal_key(pk2);

   cout<<"Catchpoint 1 \n";
   decent::crypto::valtype secret;
   for(int i=0;i<CryptoPP::AES::MAX_KEYLENGTH; i++)
      secret.push_back( k.key_byte [i] );

   cout<<"Catchpoint 2 \n";
   decent::crypto::ciphertext ct1, ct2;
   decent::crypto::el_gamal_encrypt(secret, pubk1, ct1);

   decent::crypto::valtype received_secret;
   decent::crypto::el_gamal_decrypt(ct1,pk1,received_secret);
   cout<<"Catchpoint 3 \n";

   cout <<"size of secret is "<<secret.size()<<"\n";
   for(int i=0; i<secret.size(); i++) {
      cout << (int)secret[i]<< " ";
   }
   cout <<"\n";

   cout <<"size of recovered secret is "<<received_secret.size()<<"\n";
   for(int i=0; i<received_secret.size(); i++)
   {
      cout << (int)received_secret[i] << " ";
   }
   cout <<"\n";


   decent::crypto::delivery_proof proof(CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One());
   cout<<"Catchpoint 4 \n";
   decent::crypto::encrypt_with_proof(received_secret, pk1, pubk2, ct1, ct2, proof);
   cout<<"Catchpoint 5 \n";
   decent::crypto::valtype received_secret2;
   decent::crypto::el_gamal_decrypt(ct2,pk2,received_secret2);

   cout <<"size of recovered secret is "<<received_secret2.size()<<"\n";
   for(int i=0; i<received_secret2.size(); i++)
   {
      cout << (int)received_secret2[i] << " ";
   }
   cout <<"\n";

   bool ret_val = decent::crypto::verify_delivery_proof(proof, ct1,ct2,pubk1,pubk2);
   if(ret_val)
      cout<< "everything OK!\n";

}

void test_shamir(decent::crypto::d_integer secret)
{
   decent::crypto::shamir_secret ss(5,9,secret);
   decent::crypto::point x0 = ss.split[0];
   decent::crypto::point x1 = ss.split[1];
   decent::crypto::point x2 = ss.split[2];
   decent::crypto::point x3 = ss.split[3];
   decent::crypto::point x4 = ss.split[4];

   decent::crypto::shamir_secret rs(5,9);
   rs.add_point(x0);
   rs.add_point(x1);
   rs.add_point(x2);
   rs.add_point(x3);
   rs.add_point(x4);
   if(rs.resolvable())
      rs.calculate_secret();
   cout << "Original secret: "<< secret.to_string() <<"\nReconstructed_secret: "<<rs.secret.to_string() <<"\n";
}

int main(int argc, char**argv)
{
   decent::crypto::aes_key k;
   for (int i=0; i<CryptoPP::AES::MAX_KEYLENGTH; i++)
      k.key_byte[i]=i;
   test_aes(k);
   cout<<"AES finished \n";
   test_el_gamal(k);
   const CryptoPP::Integer secret("12354678979464");
   test_shamir(secret);
}