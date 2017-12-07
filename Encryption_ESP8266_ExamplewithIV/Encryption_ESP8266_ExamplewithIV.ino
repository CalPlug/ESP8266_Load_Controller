#include <AES.h>

AES aes ;

byte *key = (unsigned char*)"0123456789010123";

byte plain[] = "Add NodeAdd NodeAdd NodeAdd NodeAdd Node";

//real iv = iv x2 ex: 01234567 = 0123456701234567
unsigned long long int my_iv = 222220;

void setup ()
{
  Serial.begin (9600) ;
  delay(500);

 
}

void loop () 
{
  prekey_test () ;
  delay(2000);
}

void prekey (int bits)
{
  aes.iv_inc();
  byte iv [N_BLOCK] ;
  byte plain_p[48];
  byte cipher [48] ;
  byte check [48] ;
  unsigned long ms = micros ();
  aes.set_IV(my_iv);
  aes.get_IV(iv);
  aes.do_aes_encrypt(plain,41,cipher,key,bits);
  Serial.print("Encryption took: ");
  Serial.println(micros() - ms);
  ms = micros ();
  aes.set_IV(my_iv);
  aes.get_IV(iv);
  aes.do_aes_decrypt(cipher,48,check,key,bits);
  Serial.print("Decryption took: ");
  Serial.println(micros() - ms);
  //Print out the arrays

//orig message
Serial.print("Original Message: ");
for (int i=0; i<sizeof(plain); i++)
{
Serial.print(char(plain[i]));
}
Serial.println();


//cipher
Serial.print("Encoded (Cipher): ");
for (int i=0; i<sizeof(cipher); i++)

{
Serial.print(char(cipher[i]));
}
Serial.println();

//Check
Serial.print("Decrypt Check: ");
for (int i=0; i<sizeof(check); i++)

{
Serial.print(char(check[i]));
}
Serial.println();
Serial.println("\n============================================================\n");
}

void prekey_test ()
{
  prekey (128) ;
}
