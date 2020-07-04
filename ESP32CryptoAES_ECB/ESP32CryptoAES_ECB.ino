
#include "mbedtls/aes.h"
 
void setup() {
 
  Serial.begin(115200);
 
  mbedtls_aes_context aes;
 
  char * key = "abcdefghijklmnop";

  // using key as example fixed known string
  char iv[16];
  memset( iv, 0, sizeof( iv ) );
  strcpy( iv, key );

  char input [32];
  memset( input, 0, sizeof( input ) );
  strcpy( input, "Hello World, AES ECB crypto test" );

  
  unsigned char output[32];
  memset( output, 0, sizeof( output ) );

  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );

  // ecb - encrypts only first 128bits / 16 bytes of input
  //mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)input, output);

  // esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, sizeof(plaintext), (uint8_t*)iv, (uint8_t*)plaintext, (uint8_t*)encrypted );
  mbedtls_aes_crypt_cbc( &aes, ESP_AES_ENCRYPT, sizeof( input ), (uint8_t*)iv, (const unsigned char*)input, output );

  mbedtls_aes_free( &aes );
 
  for (int i = 0; i < 32; i++) {
 
    char str[3];
 
    sprintf(str, "%02x", (int)output[i]);
    Serial.print(str);
  }

  Serial.println("");
}
 
void loop() {}
