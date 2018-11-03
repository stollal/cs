#include "./include/ChatClient.hpp"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <sys/socket.h>
#include <tuple>

namespace client {
void* ChatClient::worker(void* args) {
  ChatClient c;
  ChatClient::thread t;
  ChatClient::std_message s;
  std::memcpy(&t, args, sizeof(ChatClient::thread));

  std::string data;
  while (data != "/quit" && data != "kicked\n") {
    recv(t.socket, &s, sizeof(ChatClient::std_message), 0);
    // Decrypt our message
    c.decrypt(s.cipher, t.key, s.iv, data);
    if (data == "kicked") {
      std::cout << "OHH HO HO HOOO YOU HAVE BEEN KICKED MY BOY" << std::endl;
      exit(0);
      return nullptr;
    }
    std::cout << " <<< " << data << std::endl;
  }

  return nullptr;
}

void ChatClient::err() {
  ERR_print_errors_fp(stderr);
  abort();
}

in_addr_t ChatClient::handle_host() {
  std::cout << "Please enter the host you would like to connect to: " << std::flush;
  std::string host = "";
  std::getline(std::cin, host);

  return inet_addr(host.c_str());
}

int handle_port() {
  std::cout << "Please enter the port to connect from: " << std::flush;
  std::string port = "";
  std::getline(std::cin, port);

  return std::stoi(port);
}

std::string ChatClient::encrypt(
    const std::string& plaintext,
    unsigned char* key,
    unsigned char* iv,
    const std::string& cipher) {
  EVP_CIPHER_CTX *ctx;
  int cipher_len = sizeof(cipher.c_str());

  if (!(ctx = EVP_CIPHER_CTX_new())) {
    err();
  }
  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
    err();
  }

  if (1 != EVP_EncryptUpdate(
        ctx,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(cipher.c_str())),
        &cipher_len,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(plaintext.c_str())),
        static_cast<int>(sizeof(plaintext.c_str())))) {
    err();
  }

  if (1 != EVP_EncryptFinal_ex(
        ctx,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(cipher.c_str())) + cipher_len,
        &cipher_len)) {
    err();
  }
  EVP_CIPHER_CTX_free(ctx);

  return plaintext;
}

unsigned char* ChatClient::rsa_encrypt(
    const std::string& in,
    EVP_PKEY *key) {
  EVP_PKEY_CTX *ctx;
  size_t outlen = 0;
  unsigned char* output;

  ctx = EVP_PKEY_CTX_new(key, nullptr);

  if (!ctx) {
    err();
  }

  if (EVP_PKEY_encrypt_init(ctx) <= 0) {
    err();
  }

  if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
    err();
  }

  if (EVP_PKEY_encrypt(
        ctx,
        nullptr,
        &outlen,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(in.c_str())),
        in.length()) <= 0) {
    err();
  }

  if (EVP_PKEY_encrypt(
        ctx,
        output,
        &outlen,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(in.c_str())),
        in.length()) <= 0) {
    err();
  }

  return output;
}

unsigned char* ChatClient::rsa_decrypt(
    const std::string& in,
    EVP_PKEY *key) {
  EVP_PKEY_CTX *ctx;
  size_t outlen = 0;
  unsigned char* output;

  ctx = EVP_PKEY_CTX_new(key, nullptr);

  if (!ctx) {
    err();
  }

  if (EVP_PKEY_decrypt_init(ctx) <= 0) {
    err();
  }

  if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
    err();
  }

  if (EVP_PKEY_decrypt(
        ctx,
        nullptr,
        &outlen,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(in.c_str())),
        in.length()) <= 0) {
    err();
  }

  if (EVP_PKEY_encrypt(
        ctx,
        output,
        &outlen,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(in.c_str())),
        in.length()) <= 0) {
    err();
  }

  return output;
}

std::string ChatClient::decrypt(
    const std::string& plaintext,
    unsigned char* key,
    unsigned char* iv,
    std::string cipher) {
  EVP_CIPHER_CTX *ctx;
  int plaintext_len = sizeof(plaintext.c_str());
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    err();
  }

  if (1 != EVP_DecryptUpdate(
        ctx,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(plaintext.c_str())),
        &plaintext_len,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(cipher.c_str())),
        static_cast<int>(sizeof(cipher.c_str())))) {
    err();
  }

  if (1 != EVP_DecryptFinal_ex(
        ctx,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(plaintext.c_str())) + plaintext_len,
        &plaintext_len)) {
    err();
  }
  EVP_CIPHER_CTX_free(ctx);

  return plaintext;
}

int ChatClient::RunClient() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "Error creating the socket" << std::endl;
    return EXIT_FAILURE;
  }
  int port = handle_port();
  in_addr_t host = handle_host();
  std::string key;

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = host;

  socklen_t sin_size = sizeof server;

  int c = connect(sockfd, reinterpret_cast<sockaddr*>(&server), sin_size);
  if (c < 0) {
    std::cerr << "Error connecting" << std::endl;
    return EXIT_FAILURE;
  }

  std::string username = "";
  std::string message = "";
  ChatClient::symmetric_key_message skm;

  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  OPENSSL_config(nullptr);
  RAND_bytes(key, 32);


  FILE* rsa_public_key = std::fopen("rsa_pub.pem", "rb");
  EVP_PKEY *public_key;
  public_key = PEM_read_PUBKEY(rsa_public_key, nullptr, nullptr, nullptr);

  skm.encrypted_key = rsa_encrypt(key, public_key);

  sendto(sockfd, &skm, sizeof(ChatClient::symmetric_key_message), 0, reinterpret_cast<sockaddr*>(&server), sizeof server);
}

} // namespace client


int main() {
  client::ChatClient cs;
}