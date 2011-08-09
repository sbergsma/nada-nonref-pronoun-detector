/******************************************
 * nadaCommon.h
 * Shane Bergsma
 * May 20, 2011
 ******************************************/
#ifndef NADACOMMON_H
#define NADACOMMON_H

#include <stdio.h>   // For sprintf
#include <stdlib.h>  // For all the exit()'s called by the mains
#include <tr1/unordered_map>   // For storing the weights, n-grams, etc.
#include <vector>
#include <string>
//// To compile this without tr1, try switching
//// "std::tr1::unordered_map" to just "map" in all code
typedef std::tr1::unordered_map<std::string,int> StrIntMap;
// Mapping a string to float is our most common pair:
typedef std::pair<std::string,float> StrFloatPair;
// Holds the real-valued features and their values:
typedef std::vector<StrFloatPair> RealFeats;
// Holds things like sentences, "lexical" features, etc.
typedef std::vector<std::string> StrVec;
// Holds the feature weights:
typedef std::tr1::unordered_map<std::string,float> FeatureWeightMap;
// Holds indices:
typedef std::vector<size_t> Indices;
typedef std::pair<uint32_t,uint32_t> CountPair;
/////////////////////////////////////////////////////////////////////////////////
// NgramMapBase : An abstract class so we can switch between our regular and
// compressed implementations of the N-gram data
class NgramMapBase {
 public:
  virtual void find(const std::string lookup, int &itCount, int &theyCount) const = 0;
  // Load the n-gram counts from file:
  virtual void initialize(char *filename) = 0;
 protected:
  virtual ~NgramMapBase() {};
};
/////////////////////////////////////////////////////////////////////////////////
// Stores and returns the N-gram counts: each one has a it-count and a they-count:
class NgramCntMap : public NgramMapBase {
 private:
  // A structure to hold the counts
  std::tr1::unordered_map<std::string,CountPair> ngram2Cnts;
 public:
  // Returns '0' if not found, otherwise returns value1 and value2 as the values:
  void find(const std::string lookup, int &itCount, int &theyCount) const {
	std::tr1::unordered_map<std::string,CountPair>::const_iterator finder = ngram2Cnts.find(lookup);
	if (finder != ngram2Cnts.end()) {
	  CountPair cpair = finder->second; // It & They counts are stored in an <int,int> pair
	  itCount = cpair.first;
	  theyCount = cpair.second;
	} else {
	  itCount = 0;
	  theyCount = 0;
	}
  }
  // Load the n-gram counts from file:
  void initialize(char *filename);
};
/////////////////////////////////////////////////////////////////////////////////
// Stores and returns the N-gram counts: each one has a it-count and a they-count:
class NgramCompressedCntMap : public NgramMapBase {
  // We have a mapping from token1+token2+token3 to the values:
  typedef std::tr1::unordered_map<uint64_t,uint16_t> TokenValueMap;
  typedef std::tr1::unordered_map<std::string,uint16_t> String2Uint16;
 private:
  // A structure to hold the counts
  TokenValueMap tokenValMap;
  // A structure to map tokens to uint16 token-rank integers:
  String2Uint16 token2rank;  
  // A structure to map uint16 value-rank integers to it/they counts:
  std::vector<CountPair> rank2values;
 public:
  void find(const std::string lookup, int &itCount, int &theyCount) const;
  // Load the n-gram counts from file:
  void initialize(char *filename);
};
/////////////////////////////////////////////////////////////////////////////////
// Preprocess the input words to convert digits to 0
inline void normWords(std::string &input) {
  for (std::string::iterator cItr=input.begin(); cItr != input.end(); cItr++)
    if (*cItr == '#')
      *cItr = '|'; // '#' means comments elsewhere
    else if (*cItr == ':')
      *cItr = ';'; // ':' separates features and weights elsewhere
    else if (isdigit(*cItr))
      *cItr = '0';
}
// Quickly turn an integer into a string: For efficiency: Use fact we
// likely never have a distance or index > 999
inline std::string fastInt2Str(int d) {
  static char buf[4];
  sprintf(buf, "%d", d);
  return buf;
}
// generalize the lexical items in particular ways
std::string generalizeTokens(std::string token, std::string previousToken, std::string nextToken);
// The main function to convert a token into pattern format:
void patternizeToken(std::string &tok);
// Build a feature vector given the current words, in two stages:
// Build the lexical features (binary)
void buildLexicalFeatureVector(size_t pos, const StrVec &words, StrVec &bfeats);
// Build the n-gram count features (real-valued)
void buildCntFeatureVector(size_t pos, const StrVec &patts, const NgramMapBase &cnts, RealFeats &rfeats);
// Get the prediction probability for this example
float getPredictions(const FeatureWeightMap &weights, const StrVec &binFeats, const RealFeats &realFeats);
// Load the weight vector from file:
void initializeFeatureWeights(char *filename, FeatureWeightMap &weights);

#endif // NADACOMMON_H
