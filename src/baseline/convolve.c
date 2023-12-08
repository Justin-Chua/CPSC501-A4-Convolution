/*
 *  CPSC 501 Assignment 4 - Optimizing Program Performance
 *  Author: Justin Chua
 *  UCID: 30098941
 * 
 *  A C program that takes in a dry recording as a .wav file, and an impulse response as a .wav file as input,
 *  and uses a input-side convolution algorithm in order to produce a final, convoluted .wav file. The result
 *  of the convolution is a sound file that sounds as if it is being played in a specific concert hall. Many
 *  of the methods used in this program are referenced from the tutorials.
 * 
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// function declaration
bool hasWavExtension(int numOfFiles, char* fileNames[]);
void convolve(float x[], int N, float h[], int M, float y[], int P);
float shortToFloat(short value);
short scaledFloatToShort(float value, float scaleFactor);
void readAndWriteExtraBytes(int size, FILE *readFile, FILE *writeFile);

// struct used to hold header info for subchunk 1 - referenced from tutorial
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

// struct used to hold header info for subchunk 2
typedef struct {
    char subchunk2_id[4];
    int subchunk2_size;
} Subchunk2Header;

/*
 * A simple method that checks to see if all strings in a char array
 * all contain the .wav extension
 * 
 * @param numOfFiles An int the number of files to check for (i.e. argc - 1)
 * @param fileNames A char array containing filenames to check through
 * 
 * @return validExtension A boolean which is set to false if a non ".wav" 
 *                        extension is found
*/
bool hasWavExtension(int numOfFiles, char* fileNames[]) {
    bool validExtensions = true;
    for (int i = 1; i < numOfFiles; i++) {
        char *fileExtension;
        // get substring of file extension
        fileExtension = strrchr(fileNames[i], '.');
        if (fileExtension == NULL || strcmp(fileExtension, ".wav") != 0) {
            validExtensions = false;
        }
    }
    return validExtensions;
}

/*
 * A method that performs input-side convolution on the input float arrays
 * x and h, and outputs the results of the convolution to float array y.
 * This code is referenced from tutorial
 * 
 * @param x A float array containing data signals from a dry recording
 * @param N The length of input array x
 * @param h A float array containing data signals from an impulse response
 * @param M The length of input array h
 * @param y A float array that is to be updated with convoluted data signals
 * @param P The length of input array y (i.e. N + M - 1)
 * 
 * @return largestValue A float that represents the largest value computed
 *                      during the convolution
*/
void convolve(float x[], int N, float h[], int M, float y[], int P) {
    int n, m;
    /* Clear Output Buffer y[] */
    for (n = 0; n < P; n++)
        y[n] = 0.0;
    /* Outer Loop: process each input value x[n] in turn */
    for (n = 0; n < N; n++) {
        /* Inner loop: process x[n] with each sample of h[n] */
        for (m = 0; m < M; m++) {
            y[n + m] += x[n] * h[m];
        }
    }
}

/*
 * A helper method used to convert a short value into a float
 * 
 * @param value A short that is to be converted to a float
 * 
 * @return value converted to a float
*/
float shortToFloat(short value) {
    return value / 32768.0f;
}

/*
 * A helper method used to convert a float value to a short
 * 
 * @param value A float that is to be converted to a short
 * @param scaleFactor A float that represents a scaleFactor (i.e. largest value
 *                    found during convolution), which value will be divided by
 * 
 * @return value converted to a short
*/
short scaledFloatToShort(float value, float scaleFactor) {
    // scaleFactor is multiplied by an additional 1.1 to minimize static
    return (short) ((value / (scaleFactor * 1.1f)) * 32768);
}

/*
 * A method that reads extraneous bytes in cases where the size of
 * subchunk 1 is larger than 16
 * 
 * @param remainingBytes An int that represents the number of bytes to be read
 * @param *readFile A FILE pointer that represents the file stream in which 
 *                  extraneous bytes should be read from
 * @param *writeFile A FILE pointer that represents the file stream in which
 *                   the extraneous bytes should be written to.
*/
void readAndWriteExtraBytes(int remainingBytes, FILE *readFile, FILE *writeFile) {
    if (readFile != NULL){
        char garbageBytes[remainingBytes];
        fread(garbageBytes, remainingBytes, 1, readFile);
        /* NULL is specified when reading the header for impulse response (as we don't need
           to write this back to the output file) */
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
    convolve(inputSignal, inputNumSamples, impulseSignal, impulseNumSamples, 
        outputSignal, inputNumSamples + impulseNumSamples - 1);
    // now we find largest value stored in y, to be used as scaling factor 
    float largestValue = 0.0;
    for (int i = 0; i < inputNumSamples + impulseNumSamples - 1; i++) {
        // if y[n+m] is larger than current value of largestValue, overwrite it
        if (abs(outputSignal[i]) > largestValue)
            largestValue = abs(outputSignal[i]);
    }
    short *outputData = (short *) malloc((inputNumSamples + impulseNumSamples - 1) * sizeof(short));
    // convert output array back to short, and apply scaling
    for (int i = 0; i < inputNumSamples + impulseNumSamples - 1; i++)
        outputData[i] = scaledFloatToShort(outputSignal[i], largestValue);
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