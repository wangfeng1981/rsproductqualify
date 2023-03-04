#ifndef COMPUTEAPU_H
#define COMPUTEAPU_H

//根据王圆圆文章公式计算准确度A、精密度P、不确定性U

#include "gdal_priv.h"
#include <string>
#include <vector>
using std::vector;
using std::string;

class ComputeAPU
{
public:
    ComputeAPU();
    //由于浮点值对等于的支持不好，所以这里填充值使用整形
    bool computeAnU( vector<string>& biasfilelist,int fillvalue,
                     string outAfilename,string outUfilename,
                     string outCntfilename,
                     string& error ) ;
    bool computeP( vector<string>& biasfilelist,  int fillvalue,
                   string fileA,
                   string fileCnt,
                   string outPfilename,string& error ) ;
};

#endif // COMPUTEAPU_H
