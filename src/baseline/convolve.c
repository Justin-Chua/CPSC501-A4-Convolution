#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// function declaration
bool hasWavExtension(int numOfFiles, char* fileNames[]);

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
} WavHeader;

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
// the output array is passed in as a pointer, and returns largest 
float convolve(float x[], int N, float h[], int M, float y[], int P)
{
    int n, m;
    float largestValue = 0.0;
    /* Clear Output Buffer y[] */
    for (n = 0; n < P; n++)
    {
        y[n] = 0.0;
    }

    /* Outer Loop: process each input value x[n] in turn */
    for (n = 0; n < N; n++){
        /* Inner loop: process x[n] with each sample of h[n] */
        for (m = 0; m < M; m++){
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
    return (short) ((value / (scaleFactor * 1.05)) * 32768);
}

int main(int argc, char *argv[]) {
    if (argc != 4 || !hasWavExtension(argc, argv))
        printf("Usage: ./convolve [inputFile] [IRFile] [outputFile]\n"
            "Note: All files should be in .wav format");
    
    // open files for reading/writing in binary
    FILE *inputFile = fopen(argv[1], "rb");
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
    // initialize WavHeader struct for input and IR
    WavHeader inputHeader;
    WavHeader impulseHeader;

    fread(&inputHeader, sizeof(inputHeader), 1, inputFile);
    fwrite(&inputHeader, sizeof(inputHeader), 1, outputFile);
    fread(&impulseHeader, sizeof(impulseHeader), 1, impulseFile);
    // now, we check to see if size of subchunk1 for input and IR is larger than 16
    if (inputHeader.subchunk1_size > 16) {
        // if so, we want to read the extraneous garbage bytes
        int remainingBytes = inputHeader.subchunk1_size - 16;
        // create random variable to store garbage bytes
        char garbageBytes[remainingBytes];
        fread(garbageBytes, remainingBytes, 1, inputFile);
        fwrite(garbageBytes, remainingBytes, 1, outputFile);
    }
    if (impulseHeader.subchunk1_size > 16) {
        int remainingBytes = inputHeader.subchunk1_size - 16;
        char garbageBytes[remainingBytes];
        fread(garbageBytes, remainingBytes, 1, impulseFile);
    }
    // now, we read header of subchunk2 for input and IR file
    char input_subchunk2_id[4];
    int input_subchunk2_size;
    char impulse_subchunk2_id[4];
    int impulse_subchunk2_size;
    // read data into vars starting with input file
    fread(&input_subchunk2_id, sizeof(input_subchunk2_id), 1, inputFile);
    fwrite(&input_subchunk2_id, sizeof(input_subchunk2_id), 1, outputFile);
    fread(&input_subchunk2_size, sizeof(input_subchunk2_size), 1, inputFile);
    fwrite(&input_subchunk2_size, sizeof(input_subchunk2_size), 1, outputFile);
    // now the impulse file
    fread(&impulse_subchunk2_id, sizeof(impulse_subchunk2_id), 1, impulseFile);
    fread(&impulse_subchunk2_size, sizeof(impulse_subchunk2_size), 1, impulseFile);
    // calculate the number of samples for the input and IR files respectively
    int inputNumSamples = input_subchunk2_size / (inputHeader.bits_per_sample / 8);
    int impulseNumSamples = impulse_subchunk2_size / (impulseHeader.bits_per_sample / 8);
    // allocate space to hold data from input and IR files
    short *inputData = (short *) malloc(input_subchunk2_size);
    short *impulseData = (short *) malloc(impulse_subchunk2_size);
    // now read data from respective files
    fread(inputData, sizeof(short), input_subchunk2_size / sizeof(short), inputFile);
    fread(impulseData, sizeof(short), impulse_subchunk2_size / sizeof(short), impulseFile);

    // initialize float arrays for input - use malloc
    float *inputSignal = (float *) malloc(inputNumSamples * sizeof(float));
    float *impulseSignal = (float *) malloc(impulseNumSamples * sizeof(float));
    float *outputSignal = (float *) malloc((inputNumSamples + impulseNumSamples - 1) * sizeof(float));
    // fill each array
    for (int i = 0; i < inputNumSamples; i++) {
        inputSignal[i] = shortToFloat(inputData[i]);
    }
    // test - print out contents of inputSignal array
    for (int i = 0; i < impulseNumSamples; i++)
        impulseSignal[i] = shortToFloat(impulseData[i]);
    printf("outputSignal[10] before: %d\n", outputSignal[10]);
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