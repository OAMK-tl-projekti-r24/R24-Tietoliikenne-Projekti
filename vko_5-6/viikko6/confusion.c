#include <zephyr/kernel.h>
#include <math.h>
#include "confusion.h"
#include "adc.h"

int argMin(float arr[]);
/* 
  K-means algorithm should provide 6 center points with
  3 values x,y,z. Let's test measurement system with known
  center points. I.e. x,y,z are supposed to have only values
  1 = down and 2 = up
  
  CP matrix is thus the 6 center points got from K-means algoritm
  teaching process. This should actually come from include file like
  #include "KmeansCenterPoints.h"
  
  And measurements matrix is just fake matrix for testing purpose
  actual measurements are taken from ADC when accelerator is connected.
*/ 

int CM[6][6]= {0};


void printConfusionMatrix(void)
{
	printk("Confusion matrix = \n");
	printk("   cp1 cp2 cp3 cp4 cp5 cp6\n");
	for(int i = 0;i<6;i++)
	{
		printk("cp%d %d   %d   %d   %d   %d   %d\n",i+1,CM[i][0],CM[i][1],CM[i][2],CM[i][3],CM[i][4],CM[i][5]);
	}
}

void makeOneClassificationAndUpdateConfusionMatrix(int x,int y, int z,int direction)
{
    int result = calculateDistanceToAllCentrePointsAndSelectWinner(x,y,z);
    printf("tulos on %d\n", result);
    CM[direction][result]++;
    printConfusionMatrix();
}

int calculateDistanceToAllCentrePointsAndSelectWinner(int x,int y,int z)
{
    float distance[6] = {0};
    //lasketaan etäisyydet keskippisteisiin
    for (int i = 0; i < 6; i++) {
        float X = CP[i][0];
        float Y = CP[i][1];
        float Z = CP[i][2];

        distance[i] = sqrt(
            pow(X - x, 2) +
            pow(Y - y, 2) +
            pow(Z - z, 2)
            );
    }
    //argmin funktio toimii kuten pythonin numpy.argmin (mutta itsetehty c:lle)
    int closestidx = argMin(distance);


    return closestidx;
}

void resetConfusionMatrix(void)
{
	for(int i=0;i<6;i++)
	{ 
		for(int j = 0;j<6;j++)
		{
			CM[i][j]=0;
		}
	}
}

//argmin palauttaa pienimmän etäisyyden indeksin
int argMin(float arr[])
{
    int minIndex = 0;
    for (int i = 0; i < 6; i++) {
        if (arr[i] < arr[minIndex]) {
            minIndex = i;
        }
    }
    return minIndex;
}

