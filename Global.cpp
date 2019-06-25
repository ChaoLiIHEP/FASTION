#pragma once
#include "Global.h" 
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <gsl/gsl_matrix.h>

int         numProcess;
int         myRank;
double      Omegas;               //sychronous revolution frequency Hz
double      T0;                   //evolution period [s]
int         Harmonics;     

double Gaussrand(double rms, double aver,int randomIndex)
{
    static double V1, V2, S;
    static int phase = 0;
    double X;
     
     srand(time(0)+randomIndex*10);
     
    if ( phase == 0 ) {
        do {
            double U1 = (double)rand() / RAND_MAX;
            double U2 = (double)rand() / RAND_MAX;
             
            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
        } while(S >= 1 || S == 0);
         
        X = V1 * sqrt(-2 * log(S) / S);
    } else
        X = V2 * sqrt(-2 * log(S) / S);
         
    phase = 1 - phase;
 
 
    X =  X * rms + aver;
    return X;
}


void gsl_matrix_mul(gsl_matrix *a,gsl_matrix *b,gsl_matrix *c)
{
	int dimAx = a->size1;
	int dimAy = a->size2;
	
	int dimBx = b->size1;
	int dimBy = b->size2; 
	
	
	int dimCx = c->size1;
	int dimCy = c->size2; 
	
	
	
	if(dimAy!= dimBx)
	{
		std::cerr<<"error of input matrix for multipilication"<<std::endl;
	}
	
	if( dimAx!= dimCx && dimBy != dimCy)
	{
		std::cerr<<"error of output matrix for multipilication"<<std::endl;
	}
	
	
	  	
	for (size_t i=0;i<a->size1;i++)
	{
		for (size_t j=0;j<b->size2;j++)
		{
			double sum=0.0;
			for (size_t k=0;k<b->size1;k++)
			{
				sum+=gsl_matrix_get(a,i,k)*gsl_matrix_get(b,k,j);
			}
			gsl_matrix_set(c,i,j,sum);
		}
	}
}


void gsl_matrix_inv(gsl_matrix *a)
{
	size_t n=a->size1;
	size_t m=a->size2;
 
	gsl_matrix *temp1=gsl_matrix_calloc(n,n);
	gsl_matrix_memcpy(temp1,a);
 
	gsl_permutation *p=gsl_permutation_calloc(n);
	int sign=0;
	gsl_linalg_LU_decomp(temp1,p,&sign);
	gsl_matrix *inverse=gsl_matrix_calloc(n,n);
 
	gsl_linalg_LU_invert(temp1,p,inverse);
	gsl_matrix_memcpy(a,inverse);
 
	gsl_permutation_free(p);
	gsl_matrix_free(temp1);
	gsl_matrix_free(inverse);
 
}

double get_det(gsl_matrix * A)
{
	double det=0.0; 
	int n = A->size1;
	gsl_permutation *p = gsl_permutation_calloc(n);
	gsl_matrix *tmpA = gsl_matrix_calloc(n, n);
	int signum;
	gsl_matrix_memcpy(tmpA, A);
	gsl_linalg_LU_decomp(tmpA, p, &signum);
	det = gsl_linalg_LU_det(tmpA, signum);
	gsl_permutation_free(p);
	gsl_matrix_free(tmpA);
	return det;
}



