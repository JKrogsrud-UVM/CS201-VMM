//
// Created by Jared Krogsrud on 4/12/2023.
//

#define NUM_PAGES 256
#define PAGE_SIZE 256

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * This function should take a four-byte integer in the range
 * 0 to 256*256 - 1 and compute the page number and page offset.
 * it should return 0 if the address argument is valid and 1 otherwise.
 */
int decodeAddress(int address, int *pageNumber, int *pageOffset)
{
    int pageNumberTmp, pageOffsetTmp;

    // Check that address is in the correct range
    if (address < 0 || address > (256*256 - 1))
    {
        return 1;
    }
    else
    {
        pageNumberTmp = address / NUM_PAGES;
        printf("IN FUNC: %d\n", pageNumberTmp);
        pageOffsetTmp = address % PAGE_SIZE;
        printf("IN FUNC: %d\n", pageOffsetTmp);
        *pageNumber = pageNumberTmp;
        *pageOffset = pageOffsetTmp;
    }
    return 0;
}
/*
 * read bytes n to n + PAGE_SIZE - 1, where n = PAGE_SIZE * pageNumber;
 * put the data into the location pointed to by buffer
 * return 0 if there was no error during read; otherwise return 1
 */
int readFromBackingStore(FILE *fp, char *buffer, int pageNumber)
{
    int n, rc;
    long offset;
    int numToRead;

    offset = PAGE_SIZE * pageNumber;

    // Seek the correct position
    rc = fseek(fp, offset, SEEK_SET);
    if (rc != 0)
    {
        printf("error seeking to position %ld\n", offset);
    }

    numToRead = PAGE_SIZE;

    // Start reading
    n = fread(buffer, sizeof(char), numToRead, fp);
    if (n != numToRead)
    {
        printf("error: read only %d bytes\n", n);
        return 1;
    }
    else
    {
        printf("success: read %d bytes\n", n);
        return 0;
    }
}

int main()
{
//    // Testing decodeAddress
//    int address = 62493;
//    int pageNumber, pageOffset;
//
//    if (decodeAddress(address, &pageNumber, &pageOffset) == 0)
//    {
//        printf("TEST: address: %d, pageNumber: %d, pageOffset: %d\n", address, pageNumber, pageOffset);
//    }
    char *fname = "BACKING_STORE.dat";
    FILE *fp = fopen(fname, "r");
    char buffer[PAGE_SIZE];
    int successful_read;
    int pageNo = 0;

    if (fp == NULL)
    {
        printf("ERROR: cannot open file \'%s\' for reading\n", fname);
        exit(8);
    }

    successful_read = readFromBackingStore(fp, buffer, pageNo);

    fclose(fp);

    return 0;
}

