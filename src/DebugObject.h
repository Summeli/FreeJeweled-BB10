#ifndef DEBUGOBJECT_H
#define DEBUGOBJECT_H

#include <QTextStream>
#include <QFile>

QTextStream& g_DbgObj()
{
    static QFile* outFile = new QFile("debug.txt");
    static bool fileOpened = false;
    Q_CHECK_PTR(outFile);
    if (!fileOpened) {
        outFile->open(QFile::WriteOnly);
        fileOpened = true;
    }
    static QTextStream* dbgObject = new QTextStream(outFile);
    return (*dbgObject);
}

#endif // DEBUGOBJECT_H
