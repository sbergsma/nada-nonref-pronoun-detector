/******************************************
 * nadaIt.cpp
 * Shane Bergsma
 * May 20, 2011
 ******************************************/
#include <iostream>   // For reading/writing STDIN
#include <sstream>    // For parsing the input
#include <time.h>     // For timing:
#include "nadaCommon.h"

const std::string USAGE = "USAGE: cat tokenizedFile | ./nadaIt featureWeights ngramCnts";
//#define DEBUG 1

// Generate feature vectors from words and patterns, make predictions
// on the basis of the feature weights and n-gram counts:
void processSentence(const StrVec &words, const FeatureWeightMap &weights, const NgramMapBase &cnts, const Indices &itPositions) {
  // First, generate the patternized words you'll need for the N-gram look-ups, 
  // and also normalize the strings for the lexicalized feature making:
  StrVec patts; StrVec lexemes;
  std::string previousWrd = "";
  for (size_t i=0; i<words.size(); i++) {
    std::string patt = words[i]; // patts
    patternizeToken(patt);
    patts.push_back(patt);
    std::string wrd = words[i];  // lexemes
    normWords(wrd);
	std::string nextWrd = "";
	if (i+1<words.size()) nextWrd = words[i+1];
	wrd = generalizeTokens(wrd, previousWrd, nextWrd);
	previousWrd = wrd;
	//	std::cout << " " << wrd;
    lexemes.push_back(wrd);
  }
  for (size_t i=0; i<itPositions.size(); i++) {
    size_t position=itPositions[i];
    //////////////////////////
    // MAKE A PREDICTION
    //////////////////////////
    StrVec lexFeats; // First, get lexical features:
    buildLexicalFeatureVector(position, lexemes, lexFeats);
    RealFeats cntFeats; // Then the real-valued (count) ones
    buildCntFeatureVector(position, patts, cnts, cntFeats);
    // Now multiply these features by the weights
	float prediction = getPredictions(weights, lexFeats, cntFeats);
	std::cout.precision(3);
    std::cout << '\t' << position << ':' << std::fixed << prediction;
  }
}
////////////////////////////////////////////////
// Run program
////////////////////////////////////////////////
int main(int nargin, char** argv) {
  if (nargin != 3) {
    std::cerr << USAGE << std::endl;
	exit(-1);
  }
  ////////////////////////////////////////////////
  // Initialization: First, load the weight vector:
  FeatureWeightMap weights;
  initializeFeatureWeights(argv[1], weights);
  // Then, load the n-gram counts:
  NgramCompressedCntMap ngramCnts;
  ngramCnts.initialize(argv[2]);
  // Start timing of program
  clock_t startTime = clock();
  ////////////////////////////////////////////////
  // Next, go through each line (sentence) of the input, and output it
  // decisions for each 'it' instances in the sentences.
  std::string input;
  while (getline(std::cin, input)) {
    std::string originalString = input; // Store the original for printing
    StrVec words;        // Read the line into the word array
    std::stringstream line(input);    // Parse this line with a string stream:
    std::string word;
    Indices itPositions; size_t position=0;  // Record positions of 'it'
    while (getline(line, word, ' ')) {
      words.push_back(word);
      if (word == "it" || word == "It" || word == "IT" || word == "iT") itPositions.push_back(position);
      position++;
    }
    // Now, spit back out the sentence:
    std::cout << originalString;
    // Make predictions if the word 'it' is in the sentence:
    if (!itPositions.empty())
	  processSentence(words, weights, ngramCnts, itPositions);
    std::cout << std::endl;
  }
  // Report timing
  clock_t endTime = clock(); //record time that predicting ends
  float time_task = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;    //compute elapsed time of task
  std::cerr << time_task << " seconds for predictions" << std::endl;
  return 1;
}

