#include "assertlineedit.h"

AssertLineEdit::AssertLineEdit()
{

}
//get string value or throw exception
QString AssertLineEdit::getStr(QLineEdit* lineEdit,QString tag)
{
    if(lineEdit->text().isEmpty()){
        QString msg = QString("Empty ") + tag ;
        std::logic_error e(msg.toStdString().c_str()) ;
        throw e ;
    }
    return lineEdit->text() ;
}

//get double value or throw exception
double AssertLineEdit::getDouble(QLineEdit* lineEdit,QString tag)
{
    if(lineEdit->text().isEmpty()){
        QString msg = QString("Empty ") + tag ;
        std::logic_error e(msg.toStdString().c_str()) ;
        throw e ;
    }
    bool isok = false ;
    double val = lineEdit->text().toDouble(&isok) ;
    if(isok==false ){
        QString msg = QString("Not a number " + tag) ;
        std::logic_error e(msg.toStdString().c_str()) ;
        throw e ;
    }
    return val ;
}
