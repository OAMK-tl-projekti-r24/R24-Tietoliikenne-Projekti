#ifndef CONFUSION_MATRIX_H
#define CONFUSION_MATRIX_H


void printConfusionMatrix(void);
void makeOneClassificationAndUpdateConfusionMatrix(int16_t,int16_t,int16_t,int8_t);
int calculateDistanceToAllCentrePointsAndSelectWinner(int16_t,int16_t,int16_t);
void resetConfusionMatrix(void);
int argMin(float arr[]);


#endif