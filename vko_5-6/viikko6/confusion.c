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

int CP[6][3]={
	                     {1,0,0},
						 {2,0,0},
						 {0,1,0},
						 {0,2,0},
						 {0,0,1},
						 {0,0,2}
};

int measurements[6][3]={
	                     {1,0,0},
						 {2,0,0},
						 {0,1,0},
						 {0,2,0},
						 {0,0,1},
						 {0,0,2}
};

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

void makeHundredFakeClassifications(void)
{
   /*******************************************
   Jos ja toivottavasti kun teet toteutuksen paloissa eli varmistat ensin,
   että etäisyyden laskenta 6 keskipisteeseen toimii ja osaat valita 6 etäisyydestä
   voittajaksi sen lyhyimmän etäisyyden, niin silloin voit käyttää tätä aliohjelmaa
   varmistaaksesi, että etäisuuden laskenta ja luokittelu toimii varmasti tunnetulla
   itse keksimälläsi sensoridatalla ja itse keksimilläsi keskipisteillä.
   *******************************************/
   printk("Make your own implementation for this function if you need this\n");
}

void makeOneClassificationAndUpdateConfusionMatrix(int direction)
{
    int result = calculateDistanceToAllCentrePointsAndSelectWinner(1,0,0);
    printf("tulos on %d\n", result);
    CM[direction][result]++;
    printConfusionMatrix();
}

int calculateDistanceToAllCentrePointsAndSelectWinner(int x,int y,int z)
{
    float distance[6] = {0};

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

