#ifndef BCRYPT_H
#define BCRYPT_H

#define BCRYPT_HASHSIZE 64
#define BCRYPT_DEFAULT_COST 10

int bcrypt_gensalt(int cost, char *salt);
int bcrypt_hash(const char *password, char *out_hash);
int bcrypt_checkpw(const char *password, const char *hash);

#endif
