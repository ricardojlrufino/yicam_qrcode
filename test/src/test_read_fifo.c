// C program to implement one side of FIFO
// This side reads first, then reads
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define QRCODE_WRITE_FIFO "/tmp/qrcode_read"

int main()
{
    int fd1;

    char str1[80], str2[80];
    while (1)
    {
        // First open in read only and read
        fd1 = open(QRCODE_WRITE_FIFO,O_RDONLY);
        read(fd1, str1, 80);
        // Print the read string and close
        printf("User1: %s\n", str1);
        close(fd1);

    }
    return 0;
}
