#include <zephyr/kernel.h>
#include <math.h>
#include "confusion.h"
#include "mpu6050.h"
#include "keskipiste.h"

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

void makeOneClassificationAndUpdateConfusionMatrix(int16_t x,int16_t y, int16_t z,int8_t direction)
{
    int result = 0;
    result = calculateDistanceToAllCentrePointsAndSelectWinner(x,y,z);
    printf("tulos on cp%d\n", result+1);
    CM[direction][result]++;
}

int calculateDistanceToAllCentrePointsAndSelectWinner(int16_t x,int16_t y,int16_t z)
{
    float distance[6] = {0};
    //lasketaan etäisyydet keskippisteisiin
    for (int i = 0; i < 6; i++) {
        float X = CP[i][0];
        float Y = CP[i][1];
        float Z = CP[i][2];
        
        
        distance[i] = 
            pow(X - (float)x, 2) +
            pow(Y - (float)y, 2) +
            pow(Z - (float)z, 2);

        //printk("Distance is %.1f\n", distance[i]);
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
    for (int i = 1; i < 6; i++) {
        if (arr[i] < arr[minIndex]) {
            minIndex = i;
        }
    }
    return minIndex;
}