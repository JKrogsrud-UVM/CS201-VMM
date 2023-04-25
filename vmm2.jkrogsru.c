//
// Created by Jared Krogsrud on 4/25/2023.
//

#define NUM_PAGES 256
#define PAGE_SIZE 256
#define NUM_FRAMES 128
#define ADDRESS_LEN 6

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int pageTable[NUM_PAGES]; // The actual page table
    int accessTime[NUM_FRAMES]; // Most recent time a frame was accessed
    unsigned char freeFrame[NUM_FRAMES]; // 0 if a frame is occupied; otherwise 1
} PageTableInfo;

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
//        printf("IN FUNC: %d\n", pageNumberTmp);
        pageOffsetTmp = address % PAGE_SIZE;
//        printf("IN FUNC: %d\n", pageOffsetTmp);
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
    char* read;

    offset = PAGE_SIZE * pageNumber;

    // Seek the correct position
    rc = fseek(fp, offset, SEEK_SET);
    if (rc != 0)
    {
        printf("error seeking to position %ld\n", offset);
    }

    numToRead = PAGE_SIZE;

    // Start reading
    // This reads in the first
    n = fread(buffer, sizeof(char), numToRead, fp);
    if (n != numToRead)
    {
        printf("error: read only %d bytes\n", n);
        return 1;
    }
//    else
//    {
//        printf("success: read %d bytes\n", n);
//    }

    return 0;
}

/*
 * This function should take a virtual page number as input, and an access time.
 * If the page is in memory, then it sets pageFault to 0 and returns the frame number.
 * Otherwise, chose a frame, returns that frame number, and sets pageFault to 1.
 */
int getFrameNumber(PageTableInfo *pageTableInfo, int logicalPageNumber, int accessTime, int *pageFault)
{
    // Check if page is in memory
    // A return of -1 means there is a page fault as the page has no frame loc

    int frameNo;
    int earliestAccessIndex;
    int earliestAccessTime;

    if (pageTableInfo->pageTable[logicalPageNumber] == -1)
    {
        *pageFault = 1;
        // Find a frame, either an unfilled one or a frame to evict
        // Check if there is a free frame
        // This should save us from looping every time as if last frame is full
        // then all frames should be full
        if (pageTableInfo->freeFrame[NUM_FRAMES-1] == 1)
        {
            // Loop through to find a free frame
            for (int frameIndex = 0; frameIndex < NUM_FRAMES; ++frameIndex)
            {
                // Check the last entry
                if (pageTableInfo->freeFrame[frameIndex] == 1)
                {
                    // frame is free and we can return that frame
                    return frameIndex;
                }
            }
        }
        // If we make it here then we instead need to evict a frame
        // So we loop through and find the right frame to evict and return that
        else
        {
            earliestAccessIndex = 0;
            earliestAccessTime = pageTableInfo->accessTime[0];
            for (int frameIndex = 0; frameIndex < NUM_FRAMES; ++frameIndex)
            {
                if (pageTableInfo->accessTime[frameIndex] < earliestAccessTime)
                {
                    earliestAccessTime = pageTableInfo->accessTime[frameIndex];
                    earliestAccessIndex = frameIndex;
                }
            }
            // We should have the frame index stored now and we return that

            printf(" => EVICT! oldest frame is %d (age = %d)\n",
                   earliestAccessIndex, earliestAccessTime);
            return earliestAccessIndex;
        }
    }
    else
    {
        *pageFault = 0;

        frameNo = pageTableInfo->pageTable[logicalPageNumber];
        pageTableInfo->accessTime[frameNo] = accessTime;

        return frameNo;
    }
}

int main()
{
    PageTableInfo pageTableStruct;
    char physicalMemory[NUM_PAGES * NUM_FRAMES];
    char *addressFile = "addresses.txt";
    char *backingStoreFile = "BACKING_STORE.dat";

    char addressBuffer[ADDRESS_LEN];
    char buffer[PAGE_SIZE];

    int pageNo;
    int pageOffset;
    int address;
    int physicalAddress;

    int frameNo;
    int pageFault;
    int successful_read;
    int pageToEvict;

    char data;

    // Initialize pageTableStruct
    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        pageTableStruct.accessTime[i] = -1;
        pageTableStruct.freeFrame[i] = 1;
    }

    // Initialize pageTable to all -1
    for (int i = 0; i < NUM_PAGES; ++i)
    {
        pageTableStruct.pageTable[i] = -1;
    }

    int currentTime = 0;

    // Open file for reading
    FILE *fp = fopen(addressFile, "r");

    if (fp == NULL)
    {
        printf("ERROR: cannot open file \'%s\' for reading\n", addressFile);
        exit(8);
    }

    // Read a virtual address from addresses.txt
    // TODO: weirdness with reading in address leading to extra reads
    while(fgets(addressBuffer, ADDRESS_LEN, fp) != NULL)
    {

        // Convert to address to int
        address = atoi(addressBuffer);
        decodeAddress(address, &pageNo, &pageOffset);

        // Call getFrameNumber()
        frameNo = getFrameNumber(&pageTableStruct, pageNo, currentTime, &pageFault);

        if (pageFault == 1)
        {
            // A page fault occurred so we land in two different possible situations
            // either we had to find an empty frame OR we need to evict a frame
            if (pageTableStruct.freeFrame[frameNo] == 0)
            {
                // This is the case for eviction
                for (int pageIndex = 0; pageIndex < NUM_PAGES; ++pageIndex)
                {
                    // If we find the frameNo in pageTable it is currently being evicted
                    // Set it at -1
                    if (pageTableStruct.pageTable[pageIndex] == frameNo)
                    {
                        pageToEvict = pageIndex;
                        pageTableStruct.pageTable[pageIndex] = -1;
                    }
                }
                printf(" => the page mapped to frame %d is %d: page %d is now unmapped (not in memory)\n",
                       frameNo, pageToEvict, pageToEvict);
            }
            // If we needed to evict or otherwise both follow the following
            // read from Backing_Store into physical memory
            FILE *fp_2 = fopen(backingStoreFile, "r");

            if (fp_2 == NULL)
            {
                printf("ERROR: cannot open file \'%s\' for reading\n", backingStoreFile);
                exit(8);
            }

            successful_read = readFromBackingStore(fp_2, buffer, pageNo);

            if (successful_read == 0)
            {
                // Save buffer to memory
                for (int index = 0; index < PAGE_SIZE; ++index)
                {
                    physicalMemory[frameNo * PAGE_SIZE + index] = buffer[index];
                }
            }

            // Update pageTable
            pageTableStruct.pageTable[pageNo] = frameNo;
            pageTableStruct.freeFrame[frameNo] = 0;
        }
        // Here we can just read from memory
        physicalAddress = frameNo * PAGE_SIZE + pageOffset;
        data = physicalMemory[physicalAddress];

        // Print Output

        if (pageFault == 1)
        {
            printf("* ");
        }
        else
        {
            printf("  ");
        }

        printf("Virtual address: %d [%d, %d] ", address, pageNo, pageOffset);
        printf("Physical address: %d [%d, %d] ", physicalAddress, frameNo, pageOffset);
        printf("Value: %d\n", data);

        pageTableStruct.accessTime[frameNo] = currentTime;
        currentTime++;
    }
}