#ifndef PLOTORDER_H
#define PLOTORDER_H

#include <QString>
#include "../../sharedcodes2/ajson5.h"
#include <string>
#include <fstream>

using std::string;
using std::ofstream;

class PlotOrder
{
public:
    PlotOrder();

    QString infilename, reffilename ;

    double valid0in,valid1in,valid0ref,valid1ref ;
    double slopein,interin,sloperef,interref ;
    int bandin,bandref ;
    int useProj ; //0-use image coordinate; 1-use projection coordinate
    QString inTag, reTag , pythonexe ;
    QString xlabel,ylabel ;//scatter fig
    QString scriptpy ;
    double scatterXmin,scatterXmax,scatterYmin,scatterYmax ;
    double histXmin,histXmax,histCount ;//error hist x range and hist count.

    double histXminRE,histXmaxRE ;//relative error hist x range
    QString histXLabel, histYLabel ;
    QString rehistXLabel,rehistYLabel ;

    //per file mask
    int usePerfileMask ;//0-not use, 1-use
    QString inMaskFilename, reMaskFilename ;
    QString inMaskValues, reMaskValues ;

    //global mask
    int useGlobalMask; //0-not use, 1-use
    QString globalMaskTag , globalMaskValues , globalMaskFilename ;

    QString outbasename ;

    //output results
    QString indatarawfilename, refdatarawfilename ;//for scatter
    QString diffrawfilename, reldiffrawfilename ;//for hist and rediff
    QString diffrasterfilename;//tiff for bias

    int matchingCount ;

    QString matcher;

    bool writeToJsonFile(QString filename) ;
};

#endif // PLOTORDER_H
