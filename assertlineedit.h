#ifndef ASSERTLINEEDIT_H
#define ASSERTLINEEDIT_H

#include <QLineEdit>
#include <stdexcept>
#include <QString>


class AssertLineEdit
{
public:
    AssertLineEdit();
    //get string value or throw exception
    QString getStr(QLineEdit* lineEdit,QString tag) ;
    //get double value or throw exception
    double getDouble(QLineEdit* lineEdit,QString tag) ;

};

#endif // ASSERTLINEEDIT_H
