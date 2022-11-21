#include "maskvaluevalidator.h"

MaskValueValidator::MaskValueValidator()
{

}
void MaskValueValidator::init(string values)  //use , to seperate
{
    conditions.clear() ;
    vector<string> segs = wStringUtils::splitString(values,",") ;
    for(int i = 0 ; i<segs.size();++i )
    {
        vector<string> segs2 = wStringUtils::splitString( segs[i],":") ;
        if( segs2.size()==2 )
        {
            MaskValueValidatorValue mvv ;
            mvv.validMinInc = atof( segs2[0].c_str());
            mvv.validMaxInc = atof( segs2[1].c_str()) ;
            conditions.push_back(mvv) ;
        }else if( segs2.size()==1 ){
            MaskValueValidatorValue mvv ;
            mvv.validMinInc = atof( segs2[0].c_str());
            mvv.validMaxInc = mvv.validMinInc ;
            conditions.push_back(mvv) ;
        }
    }
}

//If pass one condition then return true else return false; if no conditions return false.
bool MaskValueValidator::pass(int inputMaskValue)
{
    if( conditions.size()==0 ) return false ;
    for(int i = 0 ; i<conditions.size();++i )
    {
        if( conditions[i].validMinInc<=inputMaskValue && inputMaskValue <= conditions[i].validMaxInc){
            return true ;
        }
    }
    return false ;
}

