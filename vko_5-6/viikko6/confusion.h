#ifndef CONFUSION_MATRIX_H
#define CONFUSION_MATRIX_H


void printConfusionMatrix(void);
void makeOneClassificationAndUpdateConfusionMatrix(int);
int calculateDistanceToAllCentrePointsAndSelectWinner(int,int,int);
void resetConfusionMatrix(void);
int argMin(float arr[]);


#endif