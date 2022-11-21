#ifndef MASKVALUEVALIDATOR_H
#define MASKVALUEVALIDATOR_H
#include <string>
#include <vector>
using std::string;
using std::vector;
#include "wstringutils.h"

//check if input mask value is pass one of multi-conditions.
struct MaskValueValidatorValue
{
    inline MaskValueValidatorValue():validMinInc(0),validMaxInc(0){}
    int validMinInc,validMaxInc ;
};

class MaskValueValidator
{
public:
    MaskValueValidator();
    void init(string values) ;//use ',' to seperate. e.g.: '1,2,3,4:8,9'
    bool pass(int inputMaskValue);//If pass one condition then return true else return false; if no conditions return false.
private:
    vector<MaskValueValidatorValue> conditions ;
};

#endif // MASKVALUEVALIDATOR_H
