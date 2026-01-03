#include <holy/crypto.h>
extern void holy_gcry_arcfour_init (void);
extern void holy_gcry_arcfour_fini (void);
extern void holy_gcry_blowfish_init (void);
extern void holy_gcry_blowfish_fini (void);
extern void holy_gcry_camellia_init (void);
extern void holy_gcry_camellia_fini (void);
extern void holy_gcry_cast5_init (void);
extern void holy_gcry_cast5_fini (void);
extern void holy_gcry_crc_init (void);
extern void holy_gcry_crc_fini (void);
extern void holy_gcry_des_init (void);
extern void holy_gcry_des_fini (void);
extern void holy_gcry_idea_init (void);
extern void holy_gcry_idea_fini (void);
extern void holy_gcry_md4_init (void);
extern void holy_gcry_md4_fini (void);
extern void holy_gcry_md5_init (void);
extern void holy_gcry_md5_fini (void);
extern void holy_gcry_rfc2268_init (void);
extern void holy_gcry_rfc2268_fini (void);
extern void holy_gcry_rijndael_init (void);
extern void holy_gcry_rijndael_fini (void);
extern void holy_gcry_rmd160_init (void);
extern void holy_gcry_rmd160_fini (void);
extern void holy_gcry_seed_init (void);
extern void holy_gcry_seed_fini (void);
extern void holy_gcry_serpent_init (void);
extern void holy_gcry_serpent_fini (void);
extern void holy_gcry_sha1_init (void);
extern void holy_gcry_sha1_fini (void);
extern void holy_gcry_sha256_init (void);
extern void holy_gcry_sha256_fini (void);
extern void holy_gcry_sha512_init (void);
extern void holy_gcry_sha512_fini (void);
extern void holy_gcry_tiger_init (void);
extern void holy_gcry_tiger_fini (void);
extern void holy_gcry_twofish_init (void);
extern void holy_gcry_twofish_fini (void);
extern void holy_gcry_whirlpool_init (void);
extern void holy_gcry_whirlpool_fini (void);

void
holy_gcry_init_all (void)
{
  holy_gcry_arcfour_init ();
  holy_gcry_blowfish_init ();
  holy_gcry_camellia_init ();
  holy_gcry_cast5_init ();
  holy_gcry_crc_init ();
  holy_gcry_des_init ();
  holy_gcry_idea_init ();
  holy_gcry_md4_init ();
  holy_gcry_md5_init ();
  holy_gcry_rfc2268_init ();
  holy_gcry_rijndael_init ();
  holy_gcry_rmd160_init ();
  holy_gcry_seed_init ();
  holy_gcry_serpent_init ();
  holy_gcry_sha1_init ();
  holy_gcry_sha256_init ();
  holy_gcry_sha512_init ();
  holy_gcry_tiger_init ();
  holy_gcry_twofish_init ();
  holy_gcry_whirlpool_init ();
}

void
holy_gcry_fini_all (void)
{
  holy_gcry_arcfour_fini ();
  holy_gcry_blowfish_fini ();
  holy_gcry_camellia_fini ();
  holy_gcry_cast5_fini ();
  holy_gcry_crc_fini ();
  holy_gcry_des_fini ();
  holy_gcry_idea_fini ();
  holy_gcry_md4_fini ();
  holy_gcry_md5_fini ();
  holy_gcry_rfc2268_fini ();
  holy_gcry_rijndael_fini ();
  holy_gcry_rmd160_fini ();
  holy_gcry_seed_fini ();
  holy_gcry_serpent_fini ();
  holy_gcry_sha1_fini ();
  holy_gcry_sha256_fini ();
  holy_gcry_sha512_fini ();
  holy_gcry_tiger_fini ();
  holy_gcry_twofish_fini ();
  holy_gcry_whirlpool_fini ();
}
