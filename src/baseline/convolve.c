#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// function declaration
bool hasWavExtension(int numOfFiles, char* fileNames[]);
float convolve(float x[], int N, float h[], int M, float y[], int P);
float shortToFloat(short value);
short scaledFloatToShort(float value, float scaleFactor);
void readAndWriteExtraBytes(int size, FILE *readFile, FILE *writeFile);

// wav header struct - referenced from tutorial
typedef struct {
    char chunk_id[4];
    int chunk_size;
    char format[4];
    char subchunk1_id[4];
    int subchunk1_size;
    short audio_format;
    short num_channels;
    int sample_rate;
    int byte_rate;
    short block_align;
    short bits_per_sample;
} Subchunk1Header;

typedef struct {
    char subchunk2_id[4];
    int subchunk2_size;
} Subchunk2Header;

// a simple helper function that checks if all files in an array have .wav extension
bool hasWavExtension(int numOfFiles, char* fileNames[]) {
    bool validExtensions = true;
    for (int i = 1; i < numOfFiles; i++) {
        char *fileExtension;
        // get substring of file extension
        fileExtension = strrchr(fileNames[i], '.');
        if (fileExtension == NULL || strcmp(fileExtension, ".wav") != 0) {
            validExtensions = false;
            break;
        }
    }
    return validExtensions;
}

// convolve method referenced from tutorial - slightly modified so that
// it returns largest value computed during convolution
float convolve(float x[], int N, float h[], int M, float y[], int P) {
    int n, m;
    float largestValue = 0.0;
    /* Clear Output Buffer y[] */
    for (n = 0; n < P; n++)
        y[n] = 0.0;
    /* Outer Loop: process each input value x[n] in turn */
    for (n = 0; n < N; n++) {
        /* Inner loop: process x[n] with each sample of h[n] */
        for (m = 0; m < M; m++) {
            y[n + m] += x[n] * h[m];
            // store in temp variable to minimize number of array accesses
            float currentYVal = abs(y[n + m]);
            // if y[n+m] is larger than current value of largestValue, overwrite it
            if (currentYVal > largestValue)
                largestValue = currentYVal;
        }
    }
    return largestValue;
}

// simple helper method to convert a short to a float in the range -1 to (just below) +1
float shortToFloat(short value) {
    return value / 32768.0f;
}

// helper method to convert float back to short, with scaling factor applied
short scaledFloatToShort(float value, float scaleFactor) {
    // scaleFactor is multiplied by an additional 1.1, to guarantee that return value
    // is well above or below -1/+1. This prevents static in output .wav file
    return (short) ((value / (scaleFactor * 1.1f)) * 32768);
}

void readAndWriteExtraBytes(int remainingBytes, FILE *readFile, FILE *writeFile) {
    if (readFile != NULL){
        char garbageBytes[remainingBytes];
        fread(garbageBytes, remainingBytes, 1, readFile);
        if (writeFile != NULL)
            fwrite(garbageBytes, remainingBytes, 1, writeFile);
    }
}

int main(int argc, char *argv[]) {
    // check if command line arguments are valid
    if (argc != 4 || !hasWavExtension(argc, argv))
        printf("Usage: ./convolve [inputFile] [IRFile] [outputFile]\n"
            "Note: All files should be in .wav format");
    
    // open files for reading/writing in binary
    FILE *inputFile = fopen(argv[1], "rb");
    // error handling - check if file open was successful
    if (inputFile == NULL) {
        printf("Error: specified input file does not exist!\n"
            "Terminating...\n");
        // graceful exit
        exit(0);
    }
    FILE *impulseFile = fopen(argv[2], "rb");
    if (impulseFile == NULL) {
        printf("Error: specified impulse response file does not exist!\n"
            "Terminating...\n");
        exit(0);
    }
    FILE *outputFile = fopen(argv[3], "wb");
    if (outputFile == NULL) {
        printf("Error: attempt to open file %s for writing failed\n"
            "Terminating...\n", argv[3]);
        exit(0);
    }

    // initialize Subchunk1Header struct for input and impulse data
    Subchunk1Header inputSubchunk1Header;
    Subchunk1Header impulseSubchunk1Header;
    fread(&inputSubchunk1Header, sizeof(inputSubchunk1Header), 1, inputFile);
    fwrite(&inputSubchunk1Header, sizeof(inputSubchunk1Header), 1, outputFile);
    fread(&impulseSubchunk1Header, sizeof(impulseSubchunk1Header), 1, impulseFile);
    // now, we check to see if size of subchunk1 for input and IR is larger than 16
    if (inputSubchunk1Header.subchunk1_size > 16)
        readAndWriteExtraBytes(inputSubchunk1Header.subchunk1_size - 16, inputFile, outputFile);
    if (impulseSubchunk1Header.subchunk1_size > 16)
        readAndWriteExtraBytes(impulseSubchunk1Header.subchunk1_size - 16, impulseFile, NULL);

    // initialize Subchunk2Header struct for input and impulse data
    Subchunk2Header inputSubchunk2Header;
    Subchunk2Header impulseSubchunk2Header;
    // read data into vars starting with input file
    fread(&inputSubchunk2Header, sizeof(inputSubchunk2Header), 1, inputFile);
    fwrite(&inputSubchunk2Header, sizeof(inputSubchunk2Header), 1, outputFile);
    // now the impulse file
    fread(&impulseSubchunk2Header, sizeof(impulseSubchunk2Header), 1, impulseFile);

    // calculate the number of samples for the input and IR files respectively
    int inputNumSamples = inputSubchunk2Header.subchunk2_size / (inputSubchunk1Header.bits_per_sample / 8);
    int impulseNumSamples = impulseSubchunk2Header.subchunk2_size / (impulseSubchunk1Header.bits_per_sample / 8);
    // allocate space to hold data from input and IR files
    short *inputData = (short *) malloc(inputSubchunk2Header.subchunk2_size);
    short *impulseData = (short *) malloc(impulseSubchunk2Header.subchunk2_size);
    // now read data from respective files
    fread(inputData, sizeof(short), inputSubchunk2Header.subchunk2_size / sizeof(short), inputFile);
    fread(impulseData, sizeof(short), impulseSubchunk2Header.subchunk2_size / sizeof(short), impulseFile);

    // initialize float arrays for input - use malloc
    float *inputSignal = (float *) malloc(inputNumSamples * sizeof(float));
    float *impulseSignal = (float *) malloc(impulseNumSamples * sizeof(float));
    float *outputSignal = (float *) malloc((inputNumSamples + impulseNumSamples - 1) * sizeof(float));
    // fill each array
    for (int i = 0; i < inputNumSamples; i++) {
        inputSignal[i] = shortToFloat(inputData[i]);
    }
    for (int i = 0; i < impulseNumSamples; i++)
        impulseSignal[i] = shortToFloat(impulseData[i]);
    // now, we can call convolve method using float arrays
    float scaleFactor = convolve(inputSignal, inputNumSamples, impulseSignal, impulseNumSamples, 
        outputSignal, inputNumSamples + impulseNumSamples - 1);
    short *outputData = (short *) malloc((inputNumSamples + impulseNumSamples - 1) * sizeof(short));
    // convert output array back to short, and apply scaling
    for (int i = 0; i < inputNumSamples + impulseNumSamples - 1; i++)
        outputData[i] = scaledFloatToShort(outputSignal[i], scaleFactor);
    // write short array to output file
    fwrite(outputData, sizeof(short) * (inputNumSamples + impulseNumSamples - 1) , 1, outputFile);

    // finally, close all files before exiting
    fclose(inputFile);
    fclose(impulseFile);
    fclose(outputFile);
    // free all allocated memory
    free(inputData);
    free(impulseData);
    free(inputSignal);
    free(impulseSignal);
    free(outputSignal);
    free(outputData);
    return 0;
}