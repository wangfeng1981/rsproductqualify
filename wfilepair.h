#ifndef WFILEPAIR_H
#define WFILEPAIR_H
#include <string>

using std::string;

// file pair of input file and reference file.
class wFilePair
{
public:
    wFilePair();
    inline wFilePair(string infile,string refile):infilepath(infile),refilepath(refile){}
    string infilepath;
    string refilepath;
};

#endif // WFILEPAIR_H
