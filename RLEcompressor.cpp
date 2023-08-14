// followed along with this tutorial https://www.youtube.com/watch?v=kikLEdc3C1c
// credit to handmade hero!
// I plan to tweak and optimize

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

typedef char unsigned u8;

struct file_contents
{
    size_t FileSize;
    u8 *Contents;
};

static file_contents 
ReadEntireFileIntoMemory(char *FileName)
{
    file_contents Result = {};

    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        Result.FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result.Contents = (u8 *)malloc(Result.FileSize);
        fread(Result.Contents, Result.FileSize, 1, File);

        fclose(File);
    }
    else
    {
        fprintf(stderr, "Unable to read file %s\n", FileName);
    }

    return(Result);
}

static size_t
GetMaximumCompressedOutputSize(size_t InSize)
{
    // TODO: figure out equation for our compressor
    size_t OutSize = 256 + 2*InSize;
    return(OutSize);
}

static void
Copy(size_t Size, u8 *Source, u8 * Dest)
{
    while (Size--)
    {
        *Dest++ = *Source++;
    }
}

static size_t
RLECompress(size_t InSize, u8 *In, size_t MaxOutSize, u8 *Out)
{
    size_t OutSize = 0;

#define MAX_LITERAL_COUNT 255
#define MAX_RUN_COUNT 255
    int LiteralCount = 0;
    u8 Literals[MAX_LITERAL_COUNT];

    for(size_t At = 0;
    At < InSize;
        )
    {
        u8 StartingValue = In[At];
        size_t Run = 1;
        while((Run < (InSize - At)) &&
                (Run <= MAX_RUN_COUNT) &&
                (In[At + Run] == StartingValue))
        {
            ++Run;
        }

        if((Run > 1) || (LiteralCount == MAX_LITERAL_COUNT))
        {
            // Encode a literal/run pair
            u8 LiteralCount8 = (u8)LiteralCount;
            assert(LiteralCount8 == LiteralCount);
            *Out++ = LiteralCount;

            for(int LiteralIndex = 0;
                LiteralIndex < LiteralCount;
                ++LiteralIndex)
            {
                *Out++ = Literals[LiteralIndex];
            }
            LiteralCount = 0;

            u8 Run8 = (u8)Run;
            assert(Run8 == Run);
            *Out++ = Run;

            *Out++ = StartingValue;

            At += Run;
        }
        else
        {
            // Buffer literals
            Literals[LiteralCount++] = StartingValue;

            ++At;
        }
    }
#undef MAX_LITERAL_COUNT
#undef MAX_RUN_COUNT

    return(OutSize);
}

static void
RLEDecompress(size_t InSize, u8 *In, size_t OutSize, u8 *Out)
{
    u8 *InEnd = In + InSize;
    while(In < InEnd)
    {
        int LiteralCount = *In++;
        while(LiteralCount--)
        {
            *Out++ = *In++;
        }

        int RepCount = *In++;
        u8 RepValue = *In++;
        while(RepCount--)
        {
            *Out++ = RepValue;
        }
    }

    assert(In == InEnd);
}

static size_t
Compress(size_t InSize, u8 *In, size_t OutSize, u8 *Out)
{
    RLECompress(InSize, In, OutSize, Out);
    return(InSize);
}

static size_t
Decompress(size_t InSize, u8 *In, size_t OutSize, u8 *Out)
{
    RLEDecompress(InSize, In, OutSize, Out);
}

int
main(int ArgCount, char ** Args)
{
    if (ArgCount == 4) 
    {
        size_t FinalOutputSize;
        u8 *FinalOutputBuffer;

        char *Command = Args[1];
        char *InFilename = Args[2];
        char *OutFilename = Args[3];

        file_contents InFile = ReadEntireFileIntoMemory(InFilename);
        if (strcmp(Command, "compress") == 0)
        {
            size_t OutBufferSize = GetMaximumCompressedOutputSize(InFile.FileSize);
            u8 *OutBuffer = (u8 *)malloc(OutBufferSize + 4);
            size_t CompressedSize = 
                Compress(InFile.FileSize, InFile.Contents, OutBufferSize, OutBuffer + 4);
            *(int unsigned *)OutBuffer = (int unsigned)CompressedSize;

            FinalOutputSize = CompressedSize + 4;
            FinalOutputBuffer = OutBuffer;
        }
        else if (strcmp(Command, "decompress") == 0)
        {
            if (InFile.FileSize >= 4)
            {
                size_t OutBufferSize = *(int unsigned *)InFile.Contents;
                u8 *OutBuffer = (u8 *)malloc(OutBufferSize);
                Decompress(InFile.FileSize - 4, InFile.Contents + 4,
                        OutBufferSize, OutBuffer);
                
                FinalOutputSize = OutBufferSize;
                FinalOutputBuffer = OutBuffer;
            }
            else
            {
                fprintf(stderr, "Invalid input file\n");
            }
        }
        else
        {
            fprintf(stderr, "Unregonized command %s\n", Command);
        }

        if(FinalOutputBuffer)
        {
            FILE *OutFile = fopen(OutFilename, "wb");
            if(OutFile)
            {
                fwrite(FinalOutputBuffer, 1, FinalOutputSize, OutFile);
            }
            else
            {
                fprintf(stderr, "Unable to open output file %s\n", OutFilename);
            }
        }
    }
    else 
    {
        fprintf(stderr, "Usage: %s \n compress [raw filename] [compressed filename] \n",
        Args[0]);
        fprintf(stderr, "       %s \n decompress [compressed filename] [raw filename]\n", 
        Args[0]);
    }
}