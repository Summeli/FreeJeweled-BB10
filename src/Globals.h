#ifndef GLOBALS_H
#define GLOBALS_H

#include <QDeclarativeEngine>

/* This variable is used to store QDeclarativeEngine from QDeclarativeView created in main. It used
in GameBoard to create QDeclarativeComponents. Without it separate QDeclarativeEngine was created
and it slowed things down. With one global engine game runs faster. */
extern QDeclarativeEngine *g_mainEngine;

#endif // GLOBALS_H
