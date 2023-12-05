#include <iostream>

// function declaration
bool hasWavExtension(int numOfFiles, char* fileNames[]);

// a simple helper function that checks if all files in an array have .wav extension
bool hasWavExtension(int numOfFiles, char* fileNames[]) {
    bool wavExtFound = true;
    for (int i = 1; i < numOfFiles; i++) {
        // convert filename of type char* to std::string
        std::string fileName(fileNames[i]);
        // get index of "." in filename
        int periodIndex = fileName.find_last_of(".");
        // if period is not found, set wavExtFound flag
        if (periodIndex == -1) {
            wavExtFound = false;
            break;
        } else {
            std::string fileExt = fileName.substr(periodIndex, 4);
            if (fileExt != ".wav") {
                wavExtFound = false;
                break;
            }
        }
    }
    return wavExtFound;
}

int main(int argc, char* argv[]) {
    // command-line arg checking - make sure correct args are provided
    if (argc != 4 || !hasWavExtension(argc, argv))
        std::cout << "Usage: ./convolve [inputFile] [IRFile] [outputFile]\n"
            << "Note: All files should be in .wav format" << std::endl;
    
    // all .wav files should be monophonic, 16-bit, 44.1kHz sound files

    // perform convolution using the input-side convolution algorithm
    return 0;
}