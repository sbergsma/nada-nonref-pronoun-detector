/******************************************
 * nadaCommon.cpp
 * Shane Bergsma
 * May 20, 2011
 ******************************************/
#include "nadaCommon.h"
#include <math.h>   // For log/exp
#include <iostream> // For reading/writing STDIN
#include <fstream>  // For reading files
#include <sstream>  // For converting a lookup string to a set of tokens
// Anything capitalized and longer than this will be a named-entity
const size_t NAMED_ENTITY_CUTOFF = 4;
// And all tokens will be truncated to this length:
const int TRUNCATION = 4;
// For the lexical features:
const int MAXNGRAMSIZE = 5;
const int MINNGRAMSIZE = 3;
// For the N-gram features
const char ITMARKER = '_';
const char SPACE = '^';
const int CNTNGRAMSIZE = 4;
const float SMOOTHING = 1.0;

// Flags for the compressed model:
const uint16_t NEWFIRSTFLAG = 65535;
const uint16_t NEWSECONDFLAG = 65534;

////////////////////////////////////////////////////////////
// First, some general utilities:
////////////////////////////////////////////////////////////
std::string toLower(std::string s) { 
  for (std::string::iterator p = s.begin(); p != s.end(); p++) {
    (*p) = tolower(*p);
  }
  return s;
}
bool isAllCaps(std::string s) {
  for (std::string::iterator p = s.begin(); p != s.end(); p++) {
    if (!isupper(*p)) return false;
  }
  return true;
}
inline bool isCapitalized(std::string testStr) {
  // Check to make sure we don't try to test something with nothing in it:
  if (testStr.length() == 0)
    return false; 
  unsigned char firstLetter = *( (testStr.substr(0,1)).c_str() ); 
  return isupper(firstLetter);
}
// This function replaces all sequences of numbers in a string with
// "#". E.g. "22-year-old" to "#-year-old" "507th" to "#th",
// "4g5as34d3" to "#g#as#d#"
std::string replaceDigits(const char *str) {
  std::string newString = ""; 
  // Go through the string. Copy all characters to the new string,
  // unless we're the first digit in a sequence (!digitOn) then copy #
  // or we're a subsequent digit (digitOn), then don't copy anything
  bool digitOn = false;
  for (; *str; str++) { 
    if ( isdigit(*str) ) { 
      // If first one in string
      if ( !digitOn ) {
	newString = newString + "#";
      } // else previous was also a digit, do nothing 
      digitOn = true;
    } else { // not a digit, copy character
      newString = newString + *str;
      digitOn = false;
    }
  } // end loop through characters in string
  return newString;
}
// Convert irregular verbs to a root form:
std::string irregularize(std::string tok) {
  // TO BE:
  if ( tok == "is" )
    return "be";
  else if ( tok == "'s" ) // could be "has", but oh well
    return "be";
  else if ( tok == "am" )
    return "be";
  else if ( tok == "'m" )
    return "be";
  else if ( tok == "are" )
    return "be";
  else if ( tok == "'re" )
    return "be";
  else if ( tok == "were" )
    return "be";
  else if ( tok == "was" )
    return "be";
  // TO HAVE
  else if ( tok == "have" )
    return "has";
  else if ( tok == "had" )
    return "has";
  else if ( tok == "'ve" )
    return "has";
  // TO DO
  else if ( tok == "does" )
    return "do";
  else if ( tok == "did" )
    return "do";
  // WOULD
  else if ( tok == "'d" )
    return "would";
  // WILL
  else if ( tok == "'ll" )
    return "will";
  // TO SAY
  else if ( tok == "said" )
    return "say";
  else if ( tok == "says" )
    return "say";
  return tok;
}
// This function will return the gender of a pronoun from the string.
// 0 is masc, 1-fem, 2-neut, 3-plu. Also does other classes: 4- i,
// me, my, etc. 5- you/your/yours 6- we/us/our/ours -1: noun,
// non-pronominal
int pronounClassFromString(std::string pronoun) {
  // Convert to lower case, in case it wasn't done already
  pronoun = toLower(pronoun);
  if ( pronoun == "he" || pronoun == "him" || pronoun == "himself" || pronoun == "his" ) {
    return 0; // mark it as masculine
  } else if ( pronoun == "her" || pronoun == "herself" || pronoun == "hers" || pronoun == "she" ) {
    return 1; // mark it as feminine
  } else if ( pronoun == "it" || pronoun == "its" || pronoun == "itself" ) {
    return 2; // mark it as neutral
  } else if ( pronoun == "their" || pronoun == "theirs" || pronoun == "them" || pronoun == "themselves" || pronoun == "they" ) {
    return 3; // mark it as plural
  } else if ( pronoun == "i" || pronoun == "me" || pronoun == "my" || pronoun == "mine" || pronoun == "myself" ) {
    return 4; // mark it as plural
  } else if ( pronoun == "you" || pronoun == "your" || pronoun == "yours" || pronoun == "yourselves" || pronoun == "yourself") {
    return 5; // mark it as plural
  } else if ( pronoun == "we" || pronoun == "our" || pronoun == "ours" || pronoun == "ourselves" || pronoun == "us" ) {
    return 6; // mark it as plural
  }
  return -1; // Otherwise, it ain't a pronoun
}
// generalize the lexical items in particular ways
std::string generalizeTokens(std::string token, std::string previousToken, std::string nextToken) {
  // Previous token is "" if we're the first token in the string.
  // Always replace:
  if (token == "n't") token = "not";
  if (token == "\\/") token = "/";
  if (token == "-LRB-") token = "(";
  if (token == "-RRB-") token = ")";
  if (token == "-LSB-") token = "[";
  if (token == "-RSB-") token = "]";
  if (token == "-LCB-") token = "{";
  if (token == "-RCB-") token = "}";
  // Replace depending on previous token:
  // std::cout << "P=" << previousToken << " , " << "T=" << token << std::endl;
  if ((previousToken == "It" || previousToken == "it") && token == "'s") token = "is";
  if ((previousToken == "That" || previousToken == "that") && token == "'s") token = "is";
  if ((previousToken == "What" || previousToken == "what") && token == "'s") token = "is";
  if ((previousToken == "Who" || previousToken == "who") && token == "'s") token = "is";
  if ((previousToken == "There" || previousToken == "there") && token == "'s") token = "is";
  if ((previousToken == "He" || previousToken == "he") && token == "'s") token = "is";
  if ((previousToken == "She" || previousToken == "she") && token == "'s") token = "is";
  if ((token == "Wo" || token == "wo") && (nextToken == "not" || nextToken == "n't")) token = "will";
  // Generalize NEs, unless we're first:
  if (previousToken != "" && token != "It" && token != "it" && isCapitalized(token) && token.length() > 1) {
	token = "NE";
  }
  return token;
}
////////////////////////////////////////////////////////////
// The main function to convert a token into pattern format:
////////////////////////////////////////////////////////////
void patternizeToken(std::string &tok) {
  // First: something you did with a script before, but now you do it as part of Nada:
  if (tok == "\\/") tok = "/";
  if (tok == "-LRB-") tok = "(";
  if (tok == "-RRB-") tok = ")";
  if (tok == "-LSB-") tok = "[";
  if (tok == "-RSB-") tok = "]";
  if (tok == "-LCB-") tok = "{";
  if (tok == "-RCB-") tok = "}";
  // Replace capitalized words (or all-CAPS abbreviations):
  if ( ((tok.length() > NAMED_ENTITY_CUTOFF) && isCapitalized(tok)) || (tok.length() > 1 && isAllCaps(tok)) ) {
    tok = "N";
    return;
  } 
  // All other words are lower-cased:
  tok = toLower(tok);
  // Convert irregular verbs to a root form:
  tok = irregularize(tok);
  // First, replace the digits:
  tok = replaceDigits(tok.c_str());
  // Replace pronouns: pronouns in the string will mess up matching:
  // it needs its friend -- he needs its friend won't be seen.
  // Thus, convert "he/its" to "P/P"
  int p = pronounClassFromString(tok);
  if (p<0) { // Non-pronouns get stemmed
    tok = tok.substr(0,TRUNCATION);
  } else { // pronouns
    tok = "P";
  }
}
// A function to return a particular N-gram from a token array
std::string checkAndPrintNGram(int start, int size, size_t itPos, const StrVec tokens, char spacer) {
  std::string str = "";  // Return this:
  // Return blank if it goes outside the bounds:
  if ( (start<0) || (start+size>(int)tokens.size()) ) return str;
  for (int i=0; i<size; i++) {
    size_t tokenNum = i + start;
    std::string currWord;
    if (tokenNum == itPos) {
      currWord = ITMARKER;
    } else {
      currWord = tokens[tokenNum];
    }
    if (i < size-1) { // And don't print a trailing space if we're the last token:
      str += currWord; str += spacer;
    } else {
      str += currWord;
    }
  }
  return str;
}
////////////////////////////////////////////////////////////
// Then, functions related to Machine Learning:
////////////////////////////////////////////////////////////
// Build a feature vector given the current words, in two stages:
// Build the lexical features (binary)
void buildLexicalFeatureVector(size_t itPos, const StrVec &words, StrVec &binFeats) {
  int sentSize = words.size();
  // A) Extract the n-grams of each size:
  for (int size = MAXNGRAMSIZE; size >= MINNGRAMSIZE; size--) {
    // Slide a window of size "size" over the itPos:
    for (int start = (int)(itPos)-(size-1); start<=(int)itPos; start++) {
      std::string ngram = checkAndPrintNGram(start, size, itPos, words, SPACE);
      if (ngram != "") {
		binFeats.push_back(ngram);
      }
    }
  }
  // B) collect words to the left/right:
  for (int i=itPos-1; itPos-i<=2 && i>=0; i--) {
    int offset = itPos-i;
    std::string featStr = "L=" + words[i] + "." + fastInt2Str(offset); binFeats.push_back(featStr);
  }
  for (int i=itPos+1; i-itPos<=5 && i<sentSize; i++) {
    int offset = i-itPos;
    std::string featStr = "R=" + words[i] + "." + fastInt2Str(offset); binFeats.push_back(featStr);
  }
  // C) Count how often each token occurs on the left/right, regardless of position:
  StrIntMap rightToks;
  for (int i=itPos+1; i<sentSize && i<(int)(itPos)+20; i++) {
    rightToks[words[i]]++;
  }
  StrIntMap leftToks;
  for (int i=itPos-1; i>=0 && i>=(int)(itPos)-10; i--) {
    if (words[i] == "it" || words[i] == "It" || words[i] == "itself" || words[i] == "its" || words[i] == "NE"
	|| words[i] == "that" || words[i] == "this" || words[i] == "and" || words[i] == "said" || words[i] == "says") {
      leftToks[words[i]]++;
    }
  }
  // Now add them:
  for (StrIntMap::const_iterator itr = rightToks.begin(); itr != rightToks.end(); ++itr) {
    std::string featStr = "R~" + itr->first; binFeats.push_back(featStr);
  }
  for (StrIntMap::const_iterator itr = leftToks.begin(); itr != leftToks.end(); ++itr) {
    std::string featStr = "L~" + itr->first; binFeats.push_back(featStr);
  }
  // And, finally, incorporate our bias:
  binFeats.push_back("bias");
}
// Build the n-gram count features (real-valued)
void buildCntFeatureVector(size_t itPos, const StrVec &patts, const NgramMapBase &cnts, RealFeats &rfeats) {
  // A) Extract the n-grams of each size -- here, only do one size:
  int size = CNTNGRAMSIZE;
  // Also get some aggregate counts over all offsets:
  StrIntMap totalCounts;
  for (int start = (int)(itPos)-(size-1); start<=(int)itPos; start++) {
    std::string ngram = checkAndPrintNGram(start, size, itPos, patts, ' ');
    int offset = (itPos-start);
    if (ngram != "") {
      // Get the counts for this N-gram:
	  int itCount = 0;
	  int theyCount = 0;
      cnts.find(ngram,itCount,theyCount);
	  if (itCount != 0) {
		std::string featStr = fastInt2Str(size) + "," + fastInt2Str(offset) + "+IT"; rfeats.push_back( StrFloatPair(featStr,log(itCount+SMOOTHING)) ); //$size,$offset+IT:$it
		// Collect the aggregate counts here:
		featStr = fastInt2Str(size) + "+IT";  totalCounts[featStr] += itCount;
	  } else {
		std::string featStr = fastInt2Str(size) + "," + fastInt2Str(offset) + "+IT-UNDEF"; rfeats.push_back( StrFloatPair(featStr,1.0) ); //$size,$offset+IT-UNDEF
	  }
	  if (theyCount != 0) {
		std::string featStr = fastInt2Str(size) + "," + fastInt2Str(offset) + "+THEY"; rfeats.push_back( StrFloatPair(featStr,log(theyCount+SMOOTHING)) ); //$size,$offset+THEY:$they
		featStr = fastInt2Str(size) + "+THEY";  totalCounts[featStr] += theyCount;
	  } else {
		std::string featStr = fastInt2Str(size) + "," + fastInt2Str(offset) + "+THEY-UNDEF"; rfeats.push_back( StrFloatPair(featStr,1.0) ); //$size,$offset+THEY-UNDEF
	  }
    } else {
      std::string featStr = fastInt2Str(size) + "," + fastInt2Str(offset) + "+NGM=UNDEF"; rfeats.push_back( StrFloatPair(featStr,1.0) ); //      $size,$offset+NGM=UNDEF
    }
  }
  // Now add those aggregate statistics, in log form:
  for (StrIntMap::const_iterator itr = totalCounts.begin(); itr != totalCounts.end(); ++itr) {
	rfeats.push_back( StrFloatPair(itr->first,log(itr->second+SMOOTHING)) );
  }
}
// Get the 0/1 prediction for this example
float getPredictions(const FeatureWeightMap &weights, const StrVec &binFeats, const RealFeats &realFeats) {
  float score = 0;
  for (StrVec::const_iterator itr=binFeats.begin(); itr != binFeats.end(); itr++) {
	// Get the weights for each feature:
	FeatureWeightMap::const_iterator finder = weights.find(*itr);
	// If there are weights for this feature:
	if (finder != weights.end()) {
	  score += finder->second;
	  ///	  std::cout << finder->first << '=' << finder->second << std::endl;
	}
  }
  for (RealFeats::const_iterator itr=realFeats.begin(); itr != realFeats.end(); itr++) {
	// Get the weights for each feature:
	FeatureWeightMap::const_iterator finder = weights.find(itr->first);
	// If there are weights for this feature:
	if (finder != weights.end()) {
	  score += finder->second * itr->second;
	  ///	  std::cout << finder->first << '=' << finder->second << '*' << itr->second << std::endl;
	}
  }
  // Now turn this into a probability:
  float exponentiated = exp(score);
  float probability = exponentiated / (1.0+exponentiated);
  return probability;
}
// Load the weight vector from file:
void initializeFeatureWeights(char *filename, FeatureWeightMap &weights) {
  std::cerr << "Loading feature weights ";
  std::ifstream file(filename);
  if (!file) {
    std::cerr << "Error! Weight file " << filename << " can not be opened" << std::endl;
    exit(-1);
  }
  // parse each input line:
  std::string feat;
  while (file >> feat) {
    float wt;
    file >> wt;
    weights[feat] = wt;    // Add to the weight matrix:
  }
  file.close();  // close the file
  std::cerr << "> done" << std::endl;
}
/////////////////////////////////////////////////////////////////////////////////
void NgramCompressedCntMap::find(const std::string lookup, int &itCount, int &theyCount) const {
  // Split the string into 4 parts
  int fillPosition = 3; // Default if we don't find it earlier
  std::vector<uint16_t> toks;
  std::stringstream lookupSS(lookup);
  std::string input;
  getline(lookupSS, input, ' '); // Get first token/filler
  if (input == "_")
	fillPosition = 0;
  else {
	String2Uint16::const_iterator finder = token2rank.find(input);
	if (finder != token2rank.end()) {
	  uint16_t tokRank = finder->second; // The map just gives us the rank
	  toks.push_back(tokRank);
	} else {
	  toks.push_back(0);
	}
  }
  getline(lookupSS, input, ' '); // Get the second token/filler
  if (input == "_")
	fillPosition = 1;
  else {
	String2Uint16::const_iterator finder = token2rank.find(input);
	if (finder != token2rank.end()) {
	  uint16_t tokRank = finder->second;
	  toks.push_back(tokRank);
	} else {
	  toks.push_back(0);
	}
  }
  getline(lookupSS, input, ' '); // Get the third token/filler
  if (input == "_")
	fillPosition = 2;
  else {
	String2Uint16::const_iterator finder = token2rank.find(input);
	if (finder != token2rank.end()) {
	  uint16_t tokRank = finder->second;
	  toks.push_back(tokRank);
	} else {
	  toks.push_back(0);
	}
  }
  getline(lookupSS, input); // Get the last token/filler
  if (input != "_") { // fillPosition remains '3' if input == '_'
	String2Uint16::const_iterator finder = token2rank.find(input);
	if (finder != token2rank.end()) {
	  uint16_t tokRank = finder->second; // The map just gives us the rank
	  toks.push_back(tokRank);
	} else {
	  toks.push_back(0);
	}
  }
  if (toks[0] == 0 || toks[1] == 0 || toks[2] == 0) {
	//	std::cerr << "No corresponding tokens for: " << lookup << ":" << toks[0] <<'.' << toks[1] << '.' << toks[2] << std::endl;
	itCount = 0;
	theyCount = 0;
	return;
  }
  // Readjust the token values depending on the fill position: this is
  // how we mark the position of the filler in the N-gram
  if (fillPosition==0) toks[0] += 32768;
  else if (fillPosition==1) toks[1] += 32768;
  else if (fillPosition==2) toks[2] += 32768;
  uint64_t token123 = (uint64_t)(toks[2]) + ((uint64_t)(toks[1]) << 16) + ((uint64_t)(toks[0]) << 32); // Pack them into one value 
  TokenValueMap::const_iterator finder = tokenValMap.find(token123);
  if (finder != tokenValMap.end()) {
	uint16_t valueRank = finder->second; // The map just gives us the rank
	CountPair cpair = rank2values[valueRank];  // It & They counts are stored in an <int,int> pair
	itCount = cpair.first;
	theyCount = cpair.second;
  } else {
	itCount = 0;
	theyCount = 0;
  }
}
// Load the compressed n-gram counts from file:
void NgramCompressedCntMap::initialize(char *filename) {
  std::cerr << "Loading n-gram counts. ";
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    std::cerr << "Error! N-gram count file " << filename << " can not be opened" << std::endl;
    exit(-1);
  }
  ////// Part 1: Load up the token2rank map:
  uint16_t numToks;      file.read((char *)&numToks, 2);  // Get the number of tokens
  for (int i=0; i<numToks; i++) { // Then read this many
	uint8_t numChars;    file.read((char *)&numChars, 1); // Number of characters in the token
	char token[5];       file.read(token, numChars); // Get than many chars        
	uint16_t rank;       file.read((char *)&rank, 2);     // Rank of that token
	token[numChars] = (char)NULL; // Seal off that token at its end
	token2rank[token] = rank;
	//	std::cout << "|" << token << " " << rank << std::endl;
  }
  ////// Part 2: Load up the rank2values map:
  uint16_t numVals;      file.read((char *)&numVals, 2);  // Get the number of value pairs
  rank2values.resize(numVals); // The rank array has this many values
  for (int i=0; i<numVals; i++) { // Then read this many
	uint32_t itCnt;    file.read((char *)&itCnt, 4);   // The it count
	uint32_t theyCnt;  file.read((char *)&theyCnt, 4); // The they count
	uint16_t rank;     file.read((char *)&rank, 2);    // Rank of that value
	rank2values[rank] = CountPair(itCnt, theyCnt); // Create the count pair and add it on
	// std::cout << "|" << itCnt << " " << theyCnt << ":" << rank << std::endl;
  }
  ////// Part 3: Read the N-grams themselves:
  uint16_t token1=0, token2, token3; // For reading them off
  while (file.good()) {
	uint16_t dummy;
	uint16_t values;
	file.read((char *)&dummy, 2);
	if (dummy == NEWFIRSTFLAG) { // You should re-read everything: tokens 1, 2, and 3
	  file.read((char *)&token1, 2);	
	  file.read((char *)&token2, 2);	
	  file.read((char *)&token3, 2);	
	} else if (dummy == NEWSECONDFLAG) { // You should read from token-2 onwards
	  file.read((char *)&token2, 2);	
	  file.read((char *)&token3, 2);	
	} else { // You just read the token-3 (most frequent case)
	  token3 = dummy;
	}
	file.read((char *)&values, 2); // The value is always read last
	uint64_t token123 = (uint64_t)(token3) + ((uint64_t)(token2) << 16) + ((uint64_t)(token1) << 32); // Pack them into one value 
	tokenValMap[token123] = values; // Store the tokens->value mapping
  }
  std::cerr << "Read and stored " << tokenValMap.size()  << " N-grams." << std::endl;
  file.close();  // close the file
}
/////////////////////////////////////////////////////////////////////////////////
// Load the n-gram counts from file:
void NgramCntMap::initialize(char *filename) {
  std::cerr << "Loading n-gram counts ";
  std::ifstream file(filename);
  if (!file) {
    std::cerr << "Error! N-gram count file " << filename << " can not be opened" << std::endl;
    exit(-1);
  }
  // parse each input line:
  const int MAXCNTLENGTH = 12; // I see 8 at most, but it doesn't really matter as 
                               // this is just for input's sake.
  const int MAXNGRAMLENGTH = CNTNGRAMSIZE*(1+TRUNCATION);
  // -- CNTGRAMSIZE N-grams of length TRUNCATION with a space after each (minus one, plus \0 at end)
  char ngram[MAXNGRAMLENGTH];
  while (file.getline(ngram, MAXNGRAMLENGTH, '\t')) {
    char itCount[MAXCNTLENGTH];
    file.getline(itCount, MAXCNTLENGTH, '\t');
    char theyCount[MAXCNTLENGTH];
    file.getline(theyCount, MAXCNTLENGTH, '\n');
    // atoi returns 0 if empty, so this works for all:
    CountPair itTheyCnt(atoi(itCount), atoi(theyCount));
    // Load the count-map with the counts for this n-gram:
	ngram2Cnts[ngram] = itTheyCnt;
  }
  // close the file
  file.close();
  std::cerr << "> done" << std::endl;
}
