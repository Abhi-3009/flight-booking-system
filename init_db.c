#include "bcrypt.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main() {
    char h1[64], h2[64], h3[64];
    bcrypt_hash("pass123", h1);
    bcrypt_hash("alice123", h2);
    bcrypt_hash("bob123", h3);

    printf("pass123: %s\n", h1);
    printf("alice123: %s\n", h2);
    printf("bob123: %s\n", h3);

    assert(bcrypt_checkpw("pass123", h1) == 0);
    assert(bcrypt_checkpw("wrong", h1) != 0);
    assert(bcrypt_checkpw("alice123", h2) == 0);
    assert(bcrypt_checkpw("bob123", h3) == 0);
    printf("✓ bcrypt unit tests passed!\n");

    // Write database setup files
    FILE *fp = fopen("data/customers.csv", "w");
    if (fp) {
        fprintf(fp, "id,username,password,active\n");
        fprintf(fp, "1,customer1,%s,1\n", h1);
        fprintf(fp, "2,customer2,%s,1\n", h1);
        fprintf(fp, "3,alice,%s,1\n", h2);
        fprintf(fp, "4,bob,%s,1\n", h3);
        fclose(fp);
    }

    fp = fopen("data/employees.csv", "w");
    if (fp) {
        fprintf(fp, "id,username,password\n");
        fprintf(fp, "1,employee1,%s\n", h1);
        fclose(fp);
    }

    fp = fopen("data/managers.csv", "w");
    if (fp) {
        fprintf(fp, "id,username,password\n");
        fprintf(fp, "1,manager1,%s\n", h1);
        fclose(fp);
    }

    fp = fopen("data/admins.csv", "w");
    if (fp) {
        fprintf(fp, "id,username,password\n");
        fprintf(fp, "1,admin1,%s\n", h1);
        fclose(fp);
    }

    return 0;
}
