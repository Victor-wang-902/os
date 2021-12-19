// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "bcrypt.h"
#define PASSWORD_LEN 20
#define SALT_LEN 16
char *argv[] = {"sh", 0};

int getarg(char *buf, int nbuf) { // inspiration taken from getcmd in sh.c
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if (buf[0] == 0) // EOF
    return -1;
  return 0;
}

void generate_salt(uchar *salt) {
    int i;
    long temp;
    uchar * ptr;
    ptr = salt;
    int cur;
    // uncomment the following lines to make the hash code readable.
    //char map[64] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_"; // 64 characters, one fourth of 8 bits. since I use 8 bits for determining one printable char, the chance of selecting each char is even.
    for (i=0;i<4;i++) {
        temp = random();
        while (temp){
            cur = (temp % 0x100); //0x100 corresponds to one uchar
            temp /= 0x100;
            //*ptr = map[cur]; 
            *ptr = cur;
            ptr++;
        }
    }
    *ptr = 0;
}

int main(void) {
  int pid, wpid;
  int fd;
  char *ptr;
  char *uptr;
  static char password[PASSWORD_LEN + 1]; //hardcoded at 20 char, change to x+1
  char buf[PASSWORD_LEN + 1];
  char salt[SALT_LEN + 1];
  int set = 0;
  int n;
  char *hashed;
  if (open("console", O_RDWR) < 0) {
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0); // stdout
  dup(0); // stderr

  for (;;) {
      
    printf(1, "init: starting sh\n");
    fd = open("password.txt", O_RDONLY);
    if (fd < 0) {
        close(fd);
        fd = open("password.txt", O_CREATE | O_WRONLY); // interaction part
        printf(1,"No password set. Please choose one.\n");
        printf(1,"Enter password (length capped at %d): ", PASSWORD_LEN);
        while (getarg(password, PASSWORD_LEN) >= 0) {
            memset(buf,0,sizeof(buf));
            memset(salt,0,sizeof(salt));
            if (password[0] == '\r' || password[0] == '\n') {
                printf(1,"Enter password (length capped at %d): ", PASSWORD_LEN);
                continue;
            }
            for (ptr = password; *ptr != '\0'; ptr++) {
                if (*ptr == '\r' || *ptr == '\n') {
                    *ptr = '\0';
                }
            }
            strcpy(buf, password);
            printf(1,"Re-enter to confirm: ");
            while (getarg(password, PASSWORD_LEN) >= 0) {
                if (password[0] == '\r' || password[0] == '\n') {
                    printf(1,"Re-enter to confirm: ");
                    continue;
                }
                for (ptr = password; *ptr != '\0'; ptr++) {
                    if (*ptr == '\r' || *ptr == '\n') {
                        *ptr = '\0';
                    }
                }
                if (strcmp(buf, password) == 0) { // when matches we generate salt to save resouces and then I write to a file called password
                    uptr = salt;
                    generate_salt((uchar *)uptr);
                    hashed = (char *)bcrypt(password, (uchar *)salt);
                    n = write(fd, salt, SALT_LEN);
                    n += write(fd, hashed, BCRYPT_HASHLEN);
                    //printf(1,"debugging: salt %s\nto match: %s\n",salt, hashed); //uncomment to print data written to file
                    if (n < SALT_LEN +  BCRYPT_HASHLEN){
                        printf(1,"failed to save password.");
                        exit();
                    }
                    set = 1;
                    printf(1,"Password successfully set. You may now use it to log in.\n");
                    break;

                }
                else{
                    printf(1,"Passwords do not match. Try again.\nEnter password (length capped at %d): ", PASSWORD_LEN);
                    set = 0;
                    break;
                }
            }
            if(set){
                break;
            }
        }
    }
    close(fd);
    memset(buf,0,sizeof(buf));
    memset(salt,0,sizeof(salt));
    char hash_buf[BCRYPT_HASHLEN];
    fd = open("password.txt", O_RDONLY); // read password from file
    n = read(fd, salt, SALT_LEN);
    n += read(fd, hash_buf, BCRYPT_HASHLEN);
    //printf(1,"debugging: salt %s\nto match: %s\n",salt, hash_buf); //uncomment to print data read from file
    close(fd);
    if (n < SALT_LEN + BCRYPT_HASHLEN ) {
        printf(1,"read password error");
        exit();
    }
    printf(1,"Enter password: ");
    while (getarg(password, PASSWORD_LEN) >= 0) {
        if (password[0] == '\r' || password[0] == '\n') {
            printf(1,"Enter password: ");
            continue;
        }
        for (ptr = password; *ptr != '\0'; ptr++) {
            if (*ptr == '\r' || *ptr == '\n') {
                *ptr = '\0';
            }
        }
        if (bcrypt_checkpass(password, (uchar *)salt, (uchar *)hash_buf) == 0) { // compare the password with the bcrpt function
            printf(1, "Password correct, logging you in.\n");
            break;
        }
        else {
            printf(1, "Passwords do not match. Try again.\nEnter password: ");
            continue;
        }
    }
    pid = fork();
    if (pid < 0) {
      printf(1, "init: fork failed\n");
      exit();
    }
    if (pid == 0) {
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while ((wpid = wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
}
}
