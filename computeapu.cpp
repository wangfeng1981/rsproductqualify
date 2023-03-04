#include "computeapu.h"


#ifndef WGDALRASTER_H_IMPLEMENTATION
#include "wGdalRaster.h"

#endif


#include <sstream>
using std::stringstream;
ComputeAPU::ComputeAPU()
{
    GDALAllRegister() ;
}

bool ComputeAPU::computeAnU( vector<string>& biasfilelist,int fillvalue,
                             string outAfilename,string outUfilename,
                             string outCntfilename ,
                             string& error)
{
    if( biasfilelist.size()==0 ){
        error = "filelist is empty" ;
        return false ;
    }

    int xsize = 0 ;
    int ysize = 0 ;
    wGdalRasterFloat outA , outU ;
    wGdalRasterI16 cnt ;
    {
        wGdalRasterFloat in0 ;
        bool ok1 = in0.open( biasfilelist[0] ) ;
        if(ok1==false ) {
            error = "computeAnU in0 is bad." ;
            return false ;
        }
        xsize = in0.getXSize() ;
        ysize = in0.getYSize() ;
        bool ok2 = outA.create(  xsize,ysize,1 ) ;
        bool ok3 = outU.create(  xsize,ysize,1 ) ;
        bool ok4 = cnt.create(xsize,ysize,1) ;
        if(ok2==false || ok3==false || ok4==false ){
            error = "create A U file failed." ;
            return false ;
        }
        outA.copyProj( in0.getProj() ) ;
        outU.copyProj( in0.getProj() ) ;
        outA.copyTrans( in0.getTrans() ) ;
        outU.copyTrans( in0.getTrans() ) ;
        cnt.copyProj( in0.getProj() ) ;
        cnt.copyTrans( in0.getTrans() ) ;
    }
    cnt.fill(0,0) ;
    outA.fill(0,0) ;
    outU.fill(0,0) ;

    int asize = xsize*ysize ;
    for(int ifile = 0 ; ifile < biasfilelist.size();++ ifile )
    {
        wGdalRasterFloat inx;
        bool ok1 = inx.open( biasfilelist[ifile] ) ;
        if(ok1==false ){
            stringstream ss ;
            ss <<"bad file "<<  biasfilelist[ifile] ;
            error = ss.str() ;
            return false ;
        }
        int xsize1 = inx.getXSize() ;
        int ysize1 = inx.getYSize() ;
        if( xsize != xsize1 || ysize != ysize1 ) {
            stringstream ss ;
            ss <<"bad size "<<  biasfilelist[ifile] ;
            error = ss.str() ;
            return false ;
        }
        for(int it = 0 ; it <asize; ++ it )
        {
            float bias = inx.getValuef(it,0) ;
            if( (int)bias != fillvalue )
            {
                outA.setValuef( it,0, outA.getValuef(it,0) + bias) ;
                outU.setValuef( it,0, outU.getValuef(it,0) + bias*bias ) ;
                cnt.setValuei( it,0, cnt.getValuei(it,0) + 1 ) ;
            }
        }
    }

    for(int it = 0 ; it <asize; ++ it )
    {
        float sum = outA.getValuef(it,0) ;
        float sum2 = outU.getValuef(it,0) ;
        int n = cnt.getValuei(it,0) ;
        if( n>0 )
        {
            outA.setValuef( it,0, sum / n ) ;
            outU.setValuef( it,0, sqrtf(sum2/n) ) ;
        }else{
            outA.setValuef( it,0, fillvalue  ) ;
            outU.setValuef( it,0, fillvalue  ) ;
        }
    }

    bool ok11 = outA.save(outAfilename) ;
    bool ok12 = outU.save(outUfilename) ;
    bool ok13 = cnt.save(outCntfilename) ;
    if(ok11==false || ok12==false || ok13==false ){
        error = "save A or U failed." ;
        return false ;
    }
    return true ;
}
bool ComputeAPU::computeP( vector<string>& biasfilelist,int fillvalue,
                           string fileA,
                           string fileCnt,
                           string outPfilename,
                           string& error)
{
    if( biasfilelist.size()==0 ){
        error = "filelist is empty" ;
        return false ;
    }

    int xsize = 0 ;
    int ysize = 0 ;
    wGdalRasterFloat inA , outP ;
    wGdalRasterI16 cnt ;
    {
        wGdalRasterFloat in0 ;
        bool ok1 = in0.open( biasfilelist[0] ) ;
        if(ok1==false ) {
            error = "computeP in0 is bad." ;
            return false ;
        }
        xsize = in0.getXSize() ;
        ysize = in0.getYSize() ;
        bool ok2 = outP.create(  xsize,ysize,1 ) ;
        if(ok2==false ){
            error = "bad create outP.";
            return false ;
        }
        outP.copyProj( in0.getProj() ) ;
        outP.copyTrans( in0.getTrans() ) ;
    }

    bool okA = inA.open(fileA) ;
    bool okcnt = cnt.open(fileCnt) ;

    if( okA==false || okcnt==false ){
        error = "fileA or fileCnt is bad." ;
        return false ;
    }
    if( inA.getXSize()!= xsize || inA.getYSize()!=ysize ||
            cnt.getXSize()!=xsize || cnt.getYSize()!=ysize )
    {
        error = "fileA or fileCnt size is different with input 0." ;
        return false ;
    }

    outP.fill(0,0) ;

    int asize = xsize*ysize ;
    for(int ifile = 0 ; ifile < biasfilelist.size();++ ifile )
    {
        wGdalRasterFloat inx;
        bool ok1 = inx.open( biasfilelist[ifile] ) ;
        if(ok1==false ){
            stringstream ss ;
            ss <<"bad file "<< biasfilelist[ifile] ;
            error = ss.str() ;
            return false ;
        }
        int xsize1 = inx.getXSize() ;
        int ysize1 = inx.getYSize() ;
        if( xsize != xsize1 || ysize != ysize1 ) {
            stringstream ss ;
            ss <<"bad size "<< biasfilelist[ifile] ;
            error = ss.str() ;
            return false ;
        }
        for(int it = 0 ; it <asize; ++ it )
        {
            float bias = inx.getValuef(it,0) ;
            float aVal = inA.getValuef(it,0) ;
            if( (int)bias != fillvalue )
            {
                float tempval1 = bias - aVal ;
                outP.setValuef( it,0, outP.getValuef(it,0) + tempval1*tempval1) ;
            }
        }
    }

    for(int it = 0 ; it <asize; ++ it )
    {
        float sum = outP.getValuef(it,0) ;
        int n = cnt.getValuei(it,0) ;
        if( n>1 )
        {
            outP.setValuef( it,0, sqrtf( sum / (n-1)) ) ;
        }
        else if( n==1 )
        {
            outP.setValuef( it,0, 0.f ) ;
        }
        else{
            outP.setValuef( it,0, fillvalue  ) ;
        }
    }

    bool ok11 = outP.save(outPfilename) ;
    if(ok11==false  ){
        error = "save P failed." ;
        return false ;
    }
    return true ;
}
