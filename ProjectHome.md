# NADA: A Robust System for Non-Referential Pronoun Detection #

This program takes tokenized English sentences as input and finds occurrences of the word 'it'.  When an 'it' is found, the system outputs a probability for whether the 'it' is a referential instance, or instead a pleonastic or expletive or non-anaphoric pronoun.  A classification can be made by thresholding the decisions: probabilities higher than 0.5 are referential, probabilities lower than 0.5 are non-referential.  The code includes files containing N-gram counts (for features) and model parameters (for feature weights).  These files are needed for the supervised predictor.

## Paper ##

If you use this material in your work, please cite as:

  * **Shane Bergsma and David Yarowsky, NADA: A Robust System for Non-Referential Pronoun Detection, In Proc. DAARC 2011.**

## Code and Models ##

You can download the project with SVN using:

`svn checkout http://nada-nonref-pronoun-detector.googlecode.com/svn/trunk/ nada-nonref-pronoun-detector-read-only`

To run the program, first unzip the counts file:

`gunzip Ngrams.compressed.gz`

Then type 'make' at the command line:

`make`

Then run the program on the sample text file via:

`cat testfile.txt | ./nadaIt featureWeights.dat Ngrams.compressed`

You should see output that looks as follows:

`Let me make it clear that I am not happy .  3:0.000`

`I think he likes it .   4:0.837`

`He said it would take three days to clear it all away . 2:0.430 9:0.876`

...

The decisions for the pronouns occur in tab-separated columns after the sentence.  They list the position and referential-probability of each 'it'.

## Tokenization ##

NADA was designed to work after standard Treebank tokenization.  Sentences should be split onto different lines.  You can then apply the tokenizer.sed script to tokenize the sentences before passing them to NADA.

http://www.cis.upenn.edu/~treebank/tokenizer.sed

## Contact ##

Please send an e-mail to <a href='http://www.clsp.jhu.edu/~sbergsma/'>Shane Bergsma</a> at sbergsma@jhu.edu if you use this material. We'd also be happy to help if you need any assistance.

`Shane Bergsma`

`August 9, 2011`