#ifndef PLOTVARREPLACEUTIL_H
#define PLOTVARREPLACEUTIL_H

#include <string>
#include <fstream>
#include "plotorder.h"

#include "wstringutils.h"

using std::string;
using std::ifstream;
using std::ofstream;

//用于读取模板文件，替换对应变量的值为当前order的值

//{{{outscatterpngfile}}}
//{{{xrange0}}} {{{xrange1}}}
//{{{yrange0}}} {{{yrange1}}}
//{{{xlabel}}} {{{ylabel}}}
//{{{fitequation}}}
//{{{corr}}}
 //{{{r2}}}
//{{{samplecount}}}
//{{{heatdatafile}}}
//{{{fitlinedatafile}}}
// {{{outhistpngfile}}}
// {{{histxlabel}}} {{{histylabel}}}
// {{{histXmin}}} {{{histXmax}}}
// {{{histdatafile}}}

struct PlotVarReplaceUtil
{
public:
    PlotVarReplaceUtil();
    /**
     * @brief replace replace{{{}}} variable in inpltfilename by order data.
     * @param inpltfilename input plt filename
     * @param outfilename result plt filename , it can be the same of inpltfilename, it will overwrite it.
     * @param order
     * @return
     */
    bool replace( string inpltfilename , string outfilename , PlotOrder& order) ;
};

#endif // PLOTVARREPLACEUTIL_H
