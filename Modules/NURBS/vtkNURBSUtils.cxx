/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@sandia.gov,dcthomp@sandia.gov,

=========================================================================*/
#include "vtkNURBSUtils.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkStructuredData.h"

#define vtkNew(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <cassert>
#include <cmath>
#include <string>

vtkStandardNewMacro(vtkNURBSUtils);

vtkNURBSUtils::vtkNURBSUtils()
{
}

vtkNURBSUtils::~vtkNURBSUtils()
{
}

void vtkNURBSUtils::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::LinSpace(double min, double max, int num, vtkDoubleArray *result)
{
  result->SetNumberOfTuples(num);
  double div = (max-min)/(num-1);
  for (int i=0; i<num; i++)
  {
    result->SetTuple1(i, min + div*i);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::LinSpaceClamp(double min, double max, int num, int p, vtkDoubleArray *result)
{
  result->SetNumberOfTuples(num);
  int numinterior = num - 2*(p+1);
  double div = (max-min)/(numinterior+1);
  for (int i=0; i<num; i++)
  {
    if (i < numinterior + p + 1)
    {
      result->SetTuple1(i, 0.0);
    }
    else
    {
      result->SetTuple1(i, 1.0);
    }
  }
  int count = 1;
  for (int i=p+1; i<numinterior+p+1; i++)
  {
    result->SetTuple1(i, div*count);
    count++;
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetAvgKnots(double min, double max, int num, int p, vtkDoubleArray *U, vtkDoubleArray *knots)
{
  int nCon = U->GetNumberOfTuples();
  knots->SetNumberOfTuples(num);
  int numinterior = num - 2*(p+1);
  double div = (max-min)/(numinterior-1);
  for (int i=0; i<num; i++)
  {
    if (i < numinterior + p + 1)
    {
      knots->SetTuple1(i, 0.0);
    }
    else
    {
      knots->SetTuple1(i, 1.0);
    }
  }
  for (int i=1; i<nCon-p; i++)
  {
    for (int j=i; j<i+p; j++)
    {
      double val0 = knots->GetTuple1(i+p) + U->GetTuple1(j);
      knots->SetTuple1(i+p, val0);
    }
    double val1 = (1.0/p) * knots->GetTuple1(i+p);
    knots->SetTuple1(i+p, val1);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetEndDerivKnots(double min, double max, int num, int p, vtkDoubleArray *U, vtkDoubleArray *knots)
{
  int nCon = U->GetNumberOfTuples();
  knots->SetNumberOfTuples(num);
  int numinterior = num - 2*(p+1);
  double div = (max-min)/(numinterior-1);
  for (int i=0; i<num; i++)
  {
    if (i < numinterior + p + 1)
    {
      knots->SetTuple1(i, 0.0);
    }
    else
    {
      knots->SetTuple1(i, 1.0);
    }
  }
  for (int i=0; i<nCon-p+1; i++)
  {
    for (int j=i; j<i+p; j++)
    {
      double val0 = knots->GetTuple1(i+p+1) + U->GetTuple1(j);
      knots->SetTuple1(i+p+1, val0);
    }
    double val1 = (1.0/p) * knots->GetTuple1(i+p+1);
    knots->SetTuple1(i+p+1, val1);
  }
  for (int i=0; i<num; i++)
  {
    if (i >= numinterior + p + 1)
    {
      knots->SetTuple1(i, 1.0);
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetChordSpacedUs(vtkPoints *xyz, int num, vtkDoubleArray *U)
{
  double d=0;
  vtkNew(vtkDoubleArray, dists);
  dists->SetNumberOfValues(num-1);

  for (int i=1; i<num; i++)
  {
    double pt0[3], pt1[3];
    xyz->GetPoint(i-1, pt0);
    xyz->GetPoint(i, pt1);
    double dist = std::sqrt(std::pow(pt1[0] - pt0[0], 2) +
                            std::pow(pt1[1] - pt0[1], 2) +
                            std::pow(pt1[2] - pt0[2], 2));
    d += dist;
    dists->InsertTuple1(i-1, dist);
  }

  U->SetNumberOfTuples(num);
  U->SetTuple1(0, 0.0);
  double new_u = 0.0;
  for (int i=1; i<num-1; i++)
  {
    double dist = dists->GetTuple1(i-1);
    new_u += dist/d;
    U->SetTuple1(i, new_u);
  }
  U->SetTuple1(num-1, 1.0);

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetCentripetalSpacedUs(vtkPoints *xyz, int num, vtkDoubleArray *U)
{
  double d=0;
  vtkNew(vtkDoubleArray,dists);
  dists->SetNumberOfValues(num-1);

  for (int i=1; i<num; i++)
  {
    double pt0[3], pt1[3];
    xyz->GetPoint(i-1, pt0);
    xyz->GetPoint(i, pt1);
    double dist = std::sqrt(std::pow(pt1[0] - pt0[0], 2) +
                            std::pow(pt1[1] - pt0[1], 2) +
                            std::pow(pt1[2] - pt0[2], 2));
    d += std::sqrt(dist);
    dists->InsertTuple1(i-1, dist);
  }

  U->SetNumberOfTuples(num);
  U->SetTuple1(0, 0.0);
  double new_u = 0.0;
  for (int i=1; i<num-1; i++)
  {
    double dist = dists->GetTuple1(i-1);
    new_u += std::sqrt(dist)/d;
    U->SetTuple1(i, new_u);
  }
  U->SetTuple1(num-1, 1.0);

  return 1;
}
//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetUs(vtkPoints *xyz, std::string type, vtkDoubleArray *U)
{
  int nCon = xyz->GetNumberOfPoints();

  if (!strncmp(type.c_str(), "equal", 5))
  {
    vtkNURBSUtils::LinSpace(0, 1, nCon, U);
  }
  else if (!strncmp(type.c_str(), "chord", 5))
  {
    vtkNURBSUtils::GetChordSpacedUs(xyz, nCon, U);
  }
  else if (!strncmp(type.c_str(), "centripetal", 11))
  {
    vtkNURBSUtils::GetCentripetalSpacedUs(xyz, nCon, U);
  }
  else
  {
    fprintf(stderr,"Type specified is not recognized\n");
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetKnots(vtkDoubleArray *U, int p, std::string type, vtkDoubleArray *knots)
{
  int nCon  = U->GetNumberOfTuples();
  int nKnot = nCon + p + 1;

  if (!strncmp(type.c_str(), "equal", 5))
  {
    vtkNURBSUtils::LinSpaceClamp(0, 1, nKnot, p, knots);
  }
  else if (!strncmp(type.c_str(), "average", 7))
  {
    vtkNURBSUtils::GetAvgKnots(0, 1, nKnot, p, U, knots);
  }
  else if (!strncmp(type.c_str(), "derivative", 10))
  {
    nKnot = nKnot + 2;
    vtkNURBSUtils::GetEndDerivKnots(0, 1, nKnot, p, U, knots);
  }
  else
  {
    fprintf(stderr,"Type specified is not recognized\n");
    return 0;
  }
  fprintf(stdout,"Length of Knots: %lld\n", knots->GetNumberOfTuples());

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetZeroBasisFunctions(vtkDoubleArray *U, vtkDoubleArray *knots,
                                         vtkTypedArray<double> *N0)
{
  int nCon  = U->GetNumberOfTuples();
  int nKnot = knots->GetNumberOfTuples();

  vtkNew(vtkIntArray, greater);
  vtkNew(vtkIntArray, less);
  vtkNew(vtkIntArray, spots);
  vtkNew(vtkDoubleArray, knotsShift);
  knotsShift->SetNumberOfTuples(nKnot);
  knotsShift->SetTuple1(nKnot-1, -1);
  for (int i=0; i<nKnot-1; i++)
    knotsShift->SetTuple1(i, knots->GetTuple1(i+1));

  for (int i=0; i<nCon; i++)
  {
    double val = U->GetTuple1(i);
    vtkNURBSUtils::WhereGreaterEqual(val, knots, greater);
    vtkNURBSUtils::WhereLess(val, knotsShift, less);
    vtkNURBSUtils::Intersect1D(greater, less, spots);
    for (int j=0; j<nKnot-1; j++)
    {
      N0->SetValue(i, j, spots->GetValue(j));
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetPBasisFunctions(vtkDoubleArray *U, vtkDoubleArray *knots,
                                      const int p,
                                      vtkTypedArray<double> *NP)
{
  int nCon  = U->GetNumberOfTuples();
  int nKnot = knots->GetNumberOfTuples();

  //Get zero order basis function first
  vtkNew(vtkSparseArray<double>, N0);
  N0->Resize(nCon, nKnot-1);
  if (vtkNURBSUtils::GetZeroBasisFunctions(U, knots, N0) != 1)
  {
    return 0;
  }

  //Set original size to the size of the zero basis function set
  //The size will reduce by one each iteration through the basis until
  //the correct degree basis functions are met
  vtkNew(vtkDoubleArray, sub0);
  vtkNew(vtkDoubleArray, sub1);
  vtkNew(vtkDoubleArray, term0);
  vtkNew(vtkDoubleArray, term1);

  double **tmpN = new double*[nCon];
  for (int i=0; i<nCon; i++)
  {
    tmpN[i] = new double[nKnot-1];
    for (int j=0; j<nKnot-1; j++)
      tmpN[i][j] =  N0->GetValue(i, j);
  }

  int blength = nKnot;
  for (int i=1; i<p+1; i++)
  {
    blength -= 1;
    for (int j=0; j<blength-1; j++)
    {
      double k0 = knots->GetTuple1(i+j);
      double k1 = knots->GetTuple1(j);
      double k2 = knots->GetTuple1(i+j+1);
      double k3 = knots->GetTuple1(j+1);
      double denom0  = k0 - k1;
      double denom1  = k2 - k3;
      if (denom0 != 0.0)
      {
        vtkNURBSUtils::AddVal1D(U, k1, -1.0, sub0);
        vtkNURBSUtils::MultiplyVal1D(sub0, 1.0/(denom0), term0);
      }
      else
      {
        term0->SetNumberOfTuples(blength-1);
        term0->FillComponent(0, 0.0);
      }
      if (denom1 != 0.0)
      {
        vtkNURBSUtils::AddVal1D(k2, U, -1.0, sub1);
        vtkNURBSUtils::MultiplyVal1D(sub1, 1.0/(denom1), term1);
      }
      else
      {
        term1->SetNumberOfTuples(blength-1);
        term1->FillComponent(0, 0.0);
      }
      for (int k=0; k<nCon; k++)
      {
        double final0 = term0->GetTuple1(k) * (tmpN[k][j]);
        double final1 = term1->GetTuple1(k) * (tmpN[k][j+1]);
        tmpN[k][j] = final0 + final1;
      }
    }
  }
  NP->Resize(nCon, blength-1);
  for (int i=0; i<nCon; i++)
  {
    for (int j=0; j<blength-1; j++)
    {
      NP->SetValue(i, j, tmpN[i][j]);
    }
  }

  for (int i=0; i<nCon; i++)
    delete [] tmpN[i];
  delete [] tmpN;
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetControlPointsOfCurve(vtkPoints *points, vtkDoubleArray *U, vtkDoubleArray *weights,
                                          vtkDoubleArray *knots,
                                          const int p, std::string ktype, const double D0[3], const double DN[3],
                                          vtkPoints *cPoints)
{
  int nCon = points->GetNumberOfPoints();

  vtkNew(vtkSparseArray<double>, NPTmp);
  vtkNew(vtkSparseArray<double>, NPFinal);
  if( vtkNURBSUtils::GetPBasisFunctions(U, knots, p, NPTmp) != 1)
  {
    return 0;
  }
  NPTmp->SetValue(NPTmp->GetExtents()[0].GetSize()-1, NPTmp->GetExtents()[1].GetSize()-1, 1.0);

  vtkNew(vtkDenseArray<double>, pointArrayTmp);
  vtkNew(vtkDenseArray<double>, pointArrayFinal);
  vtkNew(vtkDenseArray<double>, cPointArray);
  if (vtkNURBSUtils::PointsToTypedArray(points, pointArrayTmp) != 1)
  {
    return 0;
  }

  if (!strncmp(ktype.c_str(), "derivative", 10))
  {
    vtkNURBSUtils::SetCurveEndDerivatives(NPTmp, pointArrayTmp, p, D0, DN, U, knots,
                                          NPFinal, pointArrayFinal);
  }
  else
  {
    vtkNURBSUtils::DeepCopy(NPTmp, NPFinal);
    vtkNURBSUtils::DeepCopy(pointArrayTmp, pointArrayFinal);
  }

  vtkNew(vtkSparseArray<double>, NPinv);
  if (vtkNURBSUtils::InvertSystem(NPFinal, NPinv) != 1)
  {
    fprintf(stderr,"System could not be inverted\n");
    return 0;
  }
  if (vtkNURBSUtils::MatrixVecMultiply(NPinv, 0, pointArrayFinal, 1, cPointArray) != 1)
  {
    return 0;
  }

  if (vtkNURBSUtils::TypedArrayToPoints(cPointArray, cPoints) != 1)
  {
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetControlPointsOfSurface(vtkStructuredGrid *points, vtkDoubleArray *U,
                                             vtkDoubleArray *V, vtkDoubleArray *uWeights,
                                             vtkDoubleArray *vWeights, vtkDoubleArray *uKnots,
                                             vtkDoubleArray *vKnots, const int p, const int q,
                                             std::string kutype, std::string kvtype,
                                             vtkDoubleArray *DU0, vtkDoubleArray *DUN,
                                             vtkDoubleArray *DV0, vtkDoubleArray *DVN,
                                             vtkStructuredGrid *cPoints)
{
  int dim[3];
  points->GetDimensions(dim);
  int nUCon = dim[0];
  int nVCon = dim[1];


  vtkNew(vtkSparseArray<double>, NPUTmp);
  vtkNew(vtkSparseArray<double>, NPUFinal);
  if( vtkNURBSUtils::GetPBasisFunctions(U, uKnots, p, NPUTmp) != 1)
  {
    return 0;
  }
  NPUTmp->SetValue(NPUTmp->GetExtents()[0].GetSize()-1, NPUTmp->GetExtents()[1].GetSize()-1, 1.0);

  vtkNew(vtkSparseArray<double>, NPVTmp);
  vtkNew(vtkSparseArray<double>, NPVFinal);
  if( vtkNURBSUtils::GetPBasisFunctions(V, vKnots, q, NPVTmp) != 1)
  {
    return 0;
  }
  NPVTmp->SetValue(NPVTmp->GetExtents()[0].GetSize()-1, NPVTmp->GetExtents()[1].GetSize()-1, 1.0);

  vtkNew(vtkDenseArray<double>, pointMatTmp);
  vtkNew(vtkDenseArray<double>, pointMatFinal);
  vtkNURBSUtils::StructuredGridToTypedArray(points, pointMatTmp);

  if (!strncmp(kvtype.c_str(), "derivative", 10)
      || !strncmp(kutype.c_str(), "derivative", 10))
  {
    vtkNew(vtkDenseArray<double>, DU0Vec);
    vtkNURBSUtils::DoubleArrayToTypedArray(DU0, DU0Vec);
    vtkNew(vtkDenseArray<double>, DUNVec);
    vtkNURBSUtils::DoubleArrayToTypedArray(DUN, DUNVec);
    vtkNew(vtkDenseArray<double>, DV0Vec);
    vtkNURBSUtils::DoubleArrayToTypedArray(DV0, DV0Vec);
    vtkNew(vtkDenseArray<double>, DVNVec);
    vtkNURBSUtils::DoubleArrayToTypedArray(DVN, DVNVec);
    vtkNURBSUtils::SetSurfaceEndDerivatives(NPUTmp, NPVTmp, pointMatTmp, p, q,
                                            kutype, kvtype,
                                            DU0Vec, DUNVec, DV0Vec, DVNVec, U, V,
                                            uKnots, vKnots,
                                            NPUFinal, NPVFinal, pointMatFinal);
  }
  else
  {
    vtkNURBSUtils::DeepCopy(NPUTmp, NPUFinal);
    vtkNURBSUtils::DeepCopy(NPVTmp, NPVFinal);
    vtkNURBSUtils::DeepCopy(pointMatTmp, pointMatFinal);
  }

  //fprintf(stdout,"Basis functions U:\n");
  //vtkNURBSUtils::PrintMatrix(NPUFinal);
  vtkNew(vtkSparseArray<double>, NPUinv);
  if (vtkNURBSUtils::InvertSystem(NPUFinal, NPUinv) != 1)
  {
    fprintf(stderr,"System could not be inverted\n");
    return 0;
  }

  //fprintf(stdout,"Basis functions V:\n");
  //vtkNURBSUtils::PrintMatrix(NPVFinal);
  vtkNew(vtkSparseArray<double>, NPVinv);
  if (vtkNURBSUtils::InvertSystem(NPVFinal, NPVinv) != 1)
  {
    fprintf(stderr,"System could not be inverted\n");
    return 0;
  }


  //fprintf(stdout,"Inverted system U:\n");
  //vtkNURBSUtils::PrintMatrix(NPUinv);
  //fprintf(stdout,"Inverted system V:\n");
  //vtkNURBSUtils::PrintMatrix(NPVinv);
  vtkNew(vtkDenseArray<double>, tmpUGrid);
  if (vtkNURBSUtils::MatrixMatrixMultiply(NPUinv, 0, pointMatFinal, 1, tmpUGrid) != 1)
  {
    fprintf(stderr, "Error in matrix multiply\n");
    return 0;
  }
  vtkNew(vtkDenseArray<double>, tmpUGridT);
  vtkNURBSUtils::MatrixTranspose(tmpUGrid, 1, tmpUGridT);
  vtkNew(vtkDenseArray<double>, tmpVGrid);
  if (vtkNURBSUtils::MatrixMatrixMultiply(NPVinv, 0, tmpUGridT, 1, tmpVGrid) != 1)
  {
    fprintf(stderr, "Error in matrix multiply\n");
    return 0;
  }

  vtkNew(vtkPoints, finalPoints);
  cPoints->SetPoints(finalPoints);
  vtkNew(vtkDenseArray<double>, tmpVGridT);
  vtkNURBSUtils::MatrixTranspose(tmpVGrid, 1, tmpVGridT);
  vtkNURBSUtils::TypedArrayToStructuredGrid(tmpVGridT, cPoints);
  //fprintf(stdout,"Final structured grid of control points\n");
  //vtkNURBSUtils::PrintStructuredGrid(cPoints);

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::SetCurveEndDerivatives(vtkTypedArray<double> *NP, vtkTypedArray<double> *points,
		                          const int p, const double D0[3],
                                          const double DN[3], vtkDoubleArray *U, vtkDoubleArray *knots,
                                          vtkTypedArray<double> *newNP, vtkTypedArray<double> *newPoints)
{
  vtkNURBSUtils::AddDerivativeRows(NP, newNP, p, knots);

  vtkNURBSUtils::AddDerivativePoints(points, p, D0, DN, U, knots, newPoints);

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::AddDerivativePoints(vtkTypedArray<double> *points,
		                       const int p, const double D0[3],
                                       const double DN[3], vtkDoubleArray *U, vtkDoubleArray *knots,
                                       vtkTypedArray<double> *newPoints)
{
  int nKnot = knots->GetNumberOfTuples();
  int n = nKnot - (p + 1);
  newPoints->Resize(n, 3);

  //Add extra derivative in points
  double d0val = U->GetTuple1(p+1)/p;
  double dNval = (1 - U->GetTuple1(n - p - 4))/p;

  //Set first spot
  for (int i=0; i<3; i++)
  {
    double val = points->GetValue(0, i);
    newPoints->SetValue(0, i, val);
  }

  //Set SPECIAL second spot
  for (int i=0; i<3; i++)
  {
    double val = d0val * D0[i];
    newPoints->SetValue(1, i, val);
  }

  //Set rest of matrix
  for (int i=2; i<n-2; i++)
  {
    for (int j=0; j<3; j++)
    {
      double val = points->GetValue(i-1, j);
      newPoints->SetValue(i, j, val);
    }
  }

  //Set SPECIAL second to last row
  for (int i=0 ; i<3; i++)
  {
    double val = dNval *DN[i];
    newPoints->SetValue(n-2, i, val);
  }

  //Set last row
  for (int i=0; i<3; i++)
  {
    double val = points->GetValue(n - 3, i);
    newPoints->SetValue(n - 1, i, val);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::SetSurfaceEndDerivatives(vtkTypedArray<double> *NPU, vtkTypedArray<double> *NPV,
                                            vtkTypedArray<double> *points,
		                            const int p, const int q,
                                            std::string kutype, std::string kvtype,
                                            vtkTypedArray<double> *DU0, vtkTypedArray<double> *DUN,
                                            vtkTypedArray<double> *DV0, vtkTypedArray<double> *DVN,
                                            vtkDoubleArray *U, vtkDoubleArray *V,
                                            vtkDoubleArray *uKnots, vtkDoubleArray *vKnots,
                                            vtkTypedArray<double> *newNPU, vtkTypedArray<double> *newNPV,
                                            vtkTypedArray<double> *newPoints)
{
  if (!strncmp(kutype.c_str(), "derivative", 10))
  {
    vtkNURBSUtils::AddDerivativeRows(NPU, newNPU, p, uKnots);
  }
  else
  {
    vtkNURBSUtils::DeepCopy(NPU, newNPU);
  }
  if (!strncmp(kvtype.c_str(), "derivative", 10))
  {
    vtkNURBSUtils::AddDerivativeRows(NPV, newNPV, q, vKnots);
  }
  else
  {
    vtkNURBSUtils::DeepCopy(NPV, newNPV);
  }

  int nUKnot = uKnots->GetNumberOfTuples();
  int nVKnot = vKnots->GetNumberOfTuples();
  int nu = nUKnot - (p + 1);
  int nv = nVKnot - (q + 1);
  newPoints->Resize(nu, nv, 3);

  int npu = points->GetExtents()[0].GetSize();
  int npv = points->GetExtents()[1].GetSize();
  vtkNew(vtkDenseArray<double>, tmp0Points);
  vtkNew(vtkDenseArray<double>, tmp1Points);
  vtkNew(vtkDenseArray<double>, tmp2Points);
  vtkNew(vtkDenseArray<double>, tmp3Points);
  vtkNew(vtkDenseArray<double>, tmp4Points);
  if (!strncmp(kutype.c_str(), "derivative", 10))
  {
    tmp2Points->Resize(nu, npv, 3);
    for (int i=0; i<npv; i++)
    {
      vtkNURBSUtils::GetMatrixComp(points, i, 0, 1, tmp0Points);
      double du0[3], duN[3];
      for (int j=0; j<3; j++)
      {
        du0[j] = DU0->GetValue(i, j);
        duN[j] = DUN->GetValue(i, j);
      }
      vtkNURBSUtils::AddDerivativePoints(tmp0Points, p, du0, duN, U, uKnots, tmp1Points);
      vtkNURBSUtils::SetMatrixComp(tmp1Points, i, 0, 1, tmp2Points);
    }
    npu += 2;
  }
  else
  {
    vtkNURBSUtils::DeepCopy(points, tmp2Points);
  }

  if (!strncmp(kvtype.c_str(), "derivative", 10))
  {
    int count = 0;
    for (int i=0; i<npu; i++)
    {
      double dv0[3], dvN[3];
      if ((i == 1 || i == nu - 2) && !strncmp(kutype.c_str(), "derivative", 10))
      {
        for (int j=0; j<3; j++)
        {
          dv0[j] = 0.0;
          dvN[j] = 0.0;
        }
      }
      else
      {
        for (int j=0; j<3; j++)
        {
          dv0[j] = DV0->GetValue(count, j);
          dvN[j] = DVN->GetValue(count, j);
        }
        count++;
      }
      vtkNURBSUtils::GetMatrixComp(tmp2Points, i, 1, 1, tmp3Points);
      vtkNURBSUtils::AddDerivativePoints(tmp3Points, q, dv0, dvN, V, vKnots, tmp4Points);
      vtkNURBSUtils::SetMatrixComp(tmp4Points, i, 1, 1, newPoints);
    }
  }
  else
  {
    vtkNURBSUtils::DeepCopy(tmp2Points, newPoints);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::AddDerivativeRows(vtkTypedArray<double> *NP, vtkTypedArray<double> *newNP,
                                     const int p, vtkDoubleArray *knots)
{
  int nKnot = knots->GetNumberOfTuples();
  int n = nKnot - (p + 1);
  newNP->Resize(n, n);

  //Set first row
  for (int i=0; i<n; i++)
  {
    double val = NP->GetValue(0, i);
    newNP->SetValue(0, i, val);
  }

  //Set SPECIAL second row
  newNP->SetValue(1, 0, -1.0);
  newNP->SetValue(1, 1, 1.0);

  //Set the center of the matrix:
  for (int i=2; i<n-2; i++)
  {
    for (int j=0; j<n; j++)
    {
      double val = NP->GetValue(i-1, j);
      newNP->SetValue(i, j, val);
    }
  }

  //Set SPECIAL second to last row
  newNP->SetValue(n-2, n-2, -1.0);
  newNP->SetValue(n-2, n-1, 1.0);

  //Set last row
  for (int i=0; i<n; i++)
  {
    double val = NP->GetValue(n-3, i);
    newNP->SetValue(n-1, i, val);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::DeepCopy(vtkTypedArray<double> *input, vtkTypedArray<double> *output)
{
  int dims = input->GetDimensions();
  int dim[3];
  for (int i=0; i<dims; i++)
  {
    dim[i] = input->GetExtents()[i].GetSize();
  }
  if (dims == 1)
  {
    output->Resize(dim[0]);
  }
  else if (dims == 2)
  {
    output->Resize(dim[0], dim[1]);
  }
  else if (dims == 3)
  {
    output->Resize(dim[0], dim[1], dim[2]);
  }

  for (int i=0; i<dim[0]; i++)
  {
    if (dims == 1)
    {
      double val = input->GetValue(i);
      output->SetValue(i, val);
    }
    else
    {
      for (int j=0; j<dim[1]; j++)
      {
	if (dims == 2)
	{
	  double val = input->GetValue(i,j);
	  output->SetValue(i, j, val);
	}
	else
	{
	  for (int k=0; k<dim[2]; k++)
	  {
	    double val = input->GetValue(i, j, k);
	    output->SetValue(i, j, k, val);
	  }
	}
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::InvertSystem(vtkTypedArray<double> *NP, vtkTypedArray<double> *NPinv)
{
  int nr = NP->GetExtents()[0].GetSize();
  int nc = NP->GetExtents()[1].GetSize();
  if (nr != nc)
  {
    fprintf(stderr,"Matrix is not square, can't invert\n");
    return 0;
  }

  double **inMat = new double*[nr];
  double **outMat = new double*[nr];
  for (int i=0; i<nr; i++)
  {
    inMat[i]  = new double[nc];
    outMat[i]  = new double[nc];
  }

  for (int i=0; i<nr; i++)
  {
    for (int j=0; j<nc; j++)
    {
      inMat[i][j] = NP->GetValue(i, j);
    }
  }

  if (vtkMath::InvertMatrix(inMat, outMat, nr) == 0)
  {
    for (int i=0; i<nr; i++)
    {
      delete [] inMat[i];
      delete [] outMat[i];
    }
    delete [] inMat;
    delete [] outMat;
    return 0;
  }

  NPinv->Resize(nr, nc);
  for (int i=0; i<nr; i++)
  {
    for (int j=0; j<nc; j++)
    {
      NPinv->SetValue(i, j, outMat[i][j]);
    }
  }

  for (int i=0; i<nc; i++)
  {
    delete [] inMat[i];
    delete [] outMat[i];
  }
  delete [] inMat;
  delete [] outMat;

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::BasisEvaluation(vtkDoubleArray *knots, int p, int kEval, double uEval,
                                   vtkDoubleArray *Nu)
{
  Nu->SetNumberOfTuples(p+2);

  double *uLeft  = new double[p+1];
  double *uRight = new double[p+1];
  for (int i=0; i<p+1; i++)
  {
    Nu->SetTuple1(i, 0.0);
  }
  Nu->SetTuple1(0, 1.0);

  for (int i=1; i<p+1; i++)
  {
    uLeft[i]  = uEval - knots->GetTuple1(kEval+1-i);
    uRight[i] = knots->GetTuple1(kEval+i) - uEval;
    double saved = 0.0;
    for (int j=0; j<i; j++)
    {
      double temp = Nu->GetTuple1(j) / (uRight[j+1] + uLeft[i+j]);
      Nu->SetTuple1(j, saved + uRight[j+1]*temp);
      saved = uLeft[i-j]*temp;
    }
    Nu->SetTuple1(i, saved);
  }

  delete [] uLeft;
  delete [] uRight;

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::BasisEvaluationVec(vtkDoubleArray *knots, int p, int kEval, vtkDoubleArray *uEvals,
                                      vtkTypedArray<double> *Nus)
{
  int nU    = uEvals->GetNumberOfTuples();
  int nKnot = knots->GetNumberOfTuples();
  int nCon  = nKnot - p - 1;

  vtkNew(vtkIntArray, less);
  vtkNew(vtkIntArray, greater);
  vtkNew(vtkIntArray, fillspots);
  for (int i=0; i<p+1; i++)
  {
    vtkNURBSUtils::WhereLessEqual(knots->GetTuple1(kEval+i), uEvals, less);
    vtkNURBSUtils::WhereGreater(knots->GetTuple1(kEval+i+1), uEvals, greater);
    vtkNURBSUtils::Intersect1D(greater, less, fillspots);
    for (int j=0; j<nU; j++)
    {
      Nus->SetValue(j, i, fillspots->GetTuple1(j));
    }
  }

  vtkNew(vtkDoubleArray, saved);
  vtkNew(vtkDoubleArray, uRights);
  vtkNew(vtkDoubleArray, uLefts);
  vtkNew(vtkDoubleArray, tempVec);
  saved->SetNumberOfTuples(nU);
  tempVec->SetNumberOfTuples(nU);
  for (int i=1; i<p+1; i++)
  {
    double denom = knots->GetTuple1(kEval + i) - knots->GetTuple1(kEval);
    for (int j=0; j<nU; j++)
    {
      if (Nus->GetValue(j, 0) != 0.0)
      {
        double numer = (uEvals->GetTuple1(j) - knots->GetTuple1(kEval)) * Nus->GetValue(j, 0);
        saved->SetTuple1(j, numer/denom);
      }
      else
      {
        saved->SetTuple1(j, 0.0);
      }
    }
    for (int j=0; j<p-i+1; j++)
    {
      double uLeft  = knots->GetTuple1(kEval+j+1);
      double uRight = knots->GetTuple1(kEval+i+j+1);
      vtkNURBSUtils::AddVal1D(uRight, uEvals, -1.0, uRights);
      vtkNURBSUtils::AddVal1D(uEvals, uLeft, -1.0, uLefts);
      for (int k=0; k<nU; k++)
      {
        if (Nus->GetValue(k, j+1) != 0.0)
        {
          double temp = (Nus->GetValue(k, j+1)) / (uRight - uLeft);
          tempVec->SetTuple1(k, temp);
        }
        else
        {
          tempVec->SetTuple1(k, -1);
        }
      }
      for (int k=0; k<nU; k++)
      {
        double temp = tempVec->GetTuple1(k);
        if (temp != -1)
        {
          double newVal = saved->GetTuple1(k) + (uRights->GetTuple1(k)*temp);
          Nus->SetValue(k, j, newVal);
          saved->SetTuple1(k, uLefts->GetTuple1(k)*temp);
        }
        else
        {
          Nus->SetValue(k, j, saved->GetTuple1(k));
          saved->SetTuple1(k, 0.0);
        }
      }
    }
  }


  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::FindSpan(int p, double u, vtkDoubleArray *knots, int &span)
{
  int nKnot = knots->GetNumberOfTuples();
  int nCon = nKnot - p - 1;

  if (u == knots->GetTuple1(nCon))
  {
    span = nCon - 1;
    return 1;
  }
  int low = p;
  int high = nCon;
  int mid = (low+high)/2;

  while (u < knots->GetTuple1(mid) || u >= knots->GetTuple1(mid+1))
  {
    if (u <knots->GetTuple1(mid))
    {
      high = mid;
    }
    else
    {
      low = mid;
    }
    mid = (low+high)/2;
  }
  span = mid;
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixPointsMultiply(vtkTypedArray<double>* mat, vtkPoints *pointVec, vtkPoints *output)
{

  int nr = mat->GetExtents()[0].GetSize();
  int nc = mat->GetExtents()[1].GetSize();
  if (nc != pointVec->GetNumberOfPoints())
  {
    fprintf(stderr,"Matrix vector dimensions do not match\n");
    fprintf(stderr,"Matrix: %d by %d, Vec: %lld\n", nr, nc, pointVec->GetNumberOfPoints());
    return 0;
  }

  vtkNew(vtkPoints, tmpPoints);
  tmpPoints->SetNumberOfPoints(nr);
  for (int i=0; i<nr; i++)
  {
    double updatePt[3];
    for (int j=0; j<3; j++)
    {
      updatePt[j] = 0.0;
    }
    for (int j=0; j<nc; j++)
    {
      double newPt[3];
      double bVal = mat->GetValue(i, j);
      pointVec->GetPoint(j, newPt);
      for (int k=0; k<3; k++)
      {
        updatePt[k] += newPt[k] * bVal;
      }
    }
    tmpPoints->SetPoint(i, updatePt);
  }
  output->DeepCopy(tmpPoints);

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixVecMultiply(vtkTypedArray<double>* mat, const int matIsPoints,
                                     vtkTypedArray<double> *vec, const int vecIsPoints,
                                     vtkTypedArray<double> *output)
{

  int nrM = mat->GetExtents()[0].GetSize();
  int ncM = mat->GetExtents()[1].GetSize();
  if (matIsPoints && mat->GetExtents()[2].GetSize() != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  int nrV = vec->GetExtents()[0].GetSize();
  if (vecIsPoints && vec->GetExtents()[1].GetSize() != 3)
  {
    fprintf(stderr,"Second dimension of vector should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  if (ncM != nrV)
  {
    fprintf(stderr,"Matrix vector dimensions do not match\n");
    fprintf(stderr,"Matrix: %d by %d, Vec: %d\n", nrM, ncM, nrV);
    return 0;
  }

  int either = 0;
  output->Resize(nrM);
  if (matIsPoints)
  {
    either = 1;
    output->Resize(nrM, 3);
  }
  if (vecIsPoints)
  {
    either = 1;
    output->Resize(nrM, 3);
  }

  for (int i=0; i<nrM; i++)
  {
    double updateVal[3];
    for (int j=0; j<3; j++)
    {
      updateVal[j] = 0.0;
    }
    for (int j=0; j<ncM; j++)
    {
      double matVal[3];
      double vecVal[3];
      for (int k=0; k<3; k++)
      {
        if (matIsPoints)
        {
          matVal[k] = mat->GetValue(i, j, k);
        }
        else
        {
          matVal[k] = mat->GetValue(i, j);
        }
        if (vecIsPoints)
        {
          vecVal[k] = vec->GetValue(j, k);
        }
        else
        {
          vecVal[k] = vec->GetValue(j);
        }
      }
      for (int k=0; k<3; k++)
      {
        updateVal[k] += matVal[k] * vecVal[k];
      }
    }
    if (either == 1)
    {
      for (int j=0; j<3; j++)
      {
        output->SetValue(i, j, updateVal[j]);
      }
    }
    else
    {
      output->SetValue(i, updateVal[0]);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixMatrixMultiply(vtkTypedArray<double> *mat0, const int mat0IsPoints,
                                        vtkTypedArray<double> *mat1, const int mat1IsPoints,
                                        vtkTypedArray<double> *output)
{
  int nrM0 = mat0->GetExtents()[0].GetSize();
  int ncM0 = mat0->GetExtents()[1].GetSize();
  if (mat0IsPoints && mat0->GetExtents()[2].GetSize() != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  int nrM1 = mat1->GetExtents()[0].GetSize();
  int ncM1 = mat1->GetExtents()[1].GetSize();
  if (mat1IsPoints && mat1->GetExtents()[2].GetSize() != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  if (ncM0 != nrM1)
  {
    fprintf(stderr,"Matrix matrix dimensions do not match\n");
    fprintf(stderr,"Matrix 0: %d by %d, Matrix 1: %d by %d\n", nrM0, ncM0, nrM1, ncM1);
    return 0;
  }

  int either = 0;
  if (mat0IsPoints || mat1IsPoints)
  {
    either = 1;
    output->Resize(nrM0, ncM1, 3);
  }
  else
  {
    output->Resize(nrM0, ncM1);
  }

  if (!mat0IsPoints && !mat1IsPoints)
  {
    vtkNURBSUtils::MatrixMatrixForDGEMM(mat0, mat1, output);
  }
  else if (mat0IsPoints && mat1IsPoints)
  {
    vtkNURBSUtils::PointMatrixPointMatrixForDGEMM(mat0, mat1, output);
  }
  else if (mat0IsPoints)
  {
    vtkNURBSUtils::PointMatrixMatrixForDGEMM(mat0, mat1, output);
  }
  else
  {
    vtkNURBSUtils::MatrixPointMatrixForDGEMM(mat0, mat1, output);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixMatrixForDGEMM(vtkTypedArray<double> *mat0,
                                        vtkTypedArray<double> *mat1,
                                        vtkTypedArray<double> *output)
{
  int nrM0 = mat0->GetExtents()[0].GetSize();
  int ncM0 = mat0->GetExtents()[1].GetSize();
  int nrM1 = mat1->GetExtents()[0].GetSize();
  int ncM1 = mat1->GetExtents()[1].GetSize();

  if (ncM0 != nrM1)
  {
    fprintf(stderr,"Matrix matrix dimensions do not match\n");
    fprintf(stderr,"Matrix 0: %d by %d, Matrix 1: %d by %d\n", nrM0, ncM0, nrM1, ncM1);
    return 0;
  }

  double *mat0Vec = new double[nrM0*ncM0];
  double *mat1Vec = new double[nrM1*ncM1];
  double *outVec  = new double[nrM0*ncM1];

  vtkNURBSUtils::MatrixToVector(mat0, mat0Vec);
  vtkNURBSUtils::MatrixToVector(mat1, mat1Vec);
  if (vtkNURBSUtils::DGEMM(mat0Vec, nrM0, ncM0,
                           mat1Vec, nrM1, ncM1, outVec) != 1)
  {
    delete [] mat0Vec;
    delete [] mat1Vec;
    delete [] outVec;
    return 0;
  }
  vtkNURBSUtils::VectorToMatrix(outVec, nrM0, ncM1, output);

  delete [] mat0Vec;
  delete [] mat1Vec;
  delete [] outVec;

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PointMatrixPointMatrixForDGEMM(vtkTypedArray<double> *mat0,
                                             vtkTypedArray<double> *mat1,
                                             vtkTypedArray<double> *output)
{
  int nrM0 = mat0->GetExtents()[0].GetSize();
  int ncM0 = mat0->GetExtents()[1].GetSize();
  int nrM1 = mat1->GetExtents()[0].GetSize();
  int ncM1 = mat1->GetExtents()[1].GetSize();
  if (mat0->GetExtents()[2].GetSize() != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  if (ncM0 != nrM1)
  {
    fprintf(stderr,"Matrix matrix dimensions do not match\n");
    fprintf(stderr,"Matrix 0: %d by %d, Matrix 1: %d by %d\n", nrM0, ncM0, nrM1, ncM1);
    return 0;
  }

  double *mat0Vecs[3], *mat1Vecs[3], *outVecs[3];
  for (int i=0; i<3; i++)
  {
    mat0Vecs[i] = new double[nrM0*ncM0];
    mat1Vecs[i] = new double[nrM1*ncM1];
    outVecs[i]  = new double[nrM0*ncM1];
  }
  vtkNURBSUtils::PointMatrixToVectors(mat0, mat0Vecs);
  vtkNURBSUtils::PointMatrixToVectors(mat1, mat1Vecs);
  for (int i=0; i<3; i++)
  {
    if (vtkNURBSUtils::DGEMM(mat0Vecs[i], nrM0, ncM0,
                             mat1Vecs[i], nrM1, ncM1, outVecs[i]) != 1)
    {
      for (int i=0; i<3; i++)
      {
        delete [] mat0Vecs[i];
        delete [] mat1Vecs[i];
        delete [] outVecs[i];
      }
      return 0;
    }
  }
  vtkNURBSUtils::VectorsToPointMatrix(outVecs, nrM0, ncM1, output);

  for (int i=0; i<3; i++)
  {
    delete [] mat0Vecs[i];
    delete [] mat1Vecs[i];
    delete [] outVecs[i];
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PointMatrixMatrixForDGEMM(vtkTypedArray<double> *mat0,
                                             vtkTypedArray<double> *mat1,
                                             vtkTypedArray<double> *output)
{
  int nrM0 = mat0->GetExtents()[0].GetSize();
  int ncM0 = mat0->GetExtents()[1].GetSize();
  int nrM1 = mat1->GetExtents()[0].GetSize();
  int ncM1 = mat1->GetExtents()[1].GetSize();
  if (mat0->GetExtents()[2].GetSize() != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  if (ncM0 != nrM1)
  {
    fprintf(stderr,"Matrix matrix dimensions do not match\n");
    fprintf(stderr,"Matrix 0: %d by %d, Matrix 1: %d by %d\n", nrM0, ncM0, nrM1, ncM1);
    return 0;
  }

  double *mat1Vec = new double[nrM1*ncM1];
  double *mat0Vecs[3], *outVecs[3];
  for (int i=0; i<3; i++)
  {
    mat0Vecs[i] = new double[nrM0*ncM0];
    outVecs[i]  = new double[nrM0*ncM1];
  }
  vtkNURBSUtils::PointMatrixToVectors(mat0, mat0Vecs);
  vtkNURBSUtils::MatrixToVector(mat1, mat1Vec);
  for (int i=0; i<3; i++)
  {
    if (vtkNURBSUtils::DGEMM(mat0Vecs[i], nrM0, ncM0,
                             mat1Vec, nrM1, ncM1, outVecs[i]) != 1)
    {
      delete [] mat1Vec;
      for (int i=0; i<3; i++)
      {
        delete [] mat0Vecs[i];
        delete [] outVecs[i];
      }
      return 0;
    }
  }
  vtkNURBSUtils::VectorsToPointMatrix(outVecs, nrM0, ncM1, output);

  delete [] mat1Vec;
  for (int i=0; i<3; i++)
  {
    delete [] mat0Vecs[i];
    delete [] outVecs[i];
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixPointMatrixForDGEMM(vtkTypedArray<double> *mat0,
                                       vtkTypedArray<double> *mat1,
                                       vtkTypedArray<double> *output)
{
  int nrM0 = mat0->GetExtents()[0].GetSize();
  int ncM0 = mat0->GetExtents()[1].GetSize();
  int nrM1 = mat1->GetExtents()[0].GetSize();
  int ncM1 = mat1->GetExtents()[1].GetSize();
  if (mat1->GetExtents()[2].GetSize() != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  if (ncM0 != nrM1)
  {
    fprintf(stderr,"Matrix matrix dimensions do not match\n");
    fprintf(stderr,"Matrix 0: %d by %d, Matrix 1: %d by %d\n", nrM0, ncM0, nrM1, ncM1);
    return 0;
  }

  double *mat0Vec = new double[nrM0*ncM0];
  double *mat1Vecs[3], *outVecs[3];
  for (int i=0; i<3; i++)
  {
    mat1Vecs[i] = new double[nrM1*ncM1];
    outVecs[i]  = new double[nrM0*ncM1];
  }
  vtkNURBSUtils::MatrixToVector(mat0, mat0Vec);
  vtkNURBSUtils::PointMatrixToVectors(mat1, mat1Vecs);
  for (int i=0; i<3; i++)
  {
    if (vtkNURBSUtils::DGEMM(mat0Vec, nrM0, ncM0,
                             mat1Vecs[i], nrM1, ncM1, outVecs[i]) != 1)
    {
      delete [] mat0Vec;
      for (int i=0; i<3; i++)
      {
        delete [] mat1Vecs[i];
        delete [] outVecs[i];
      }
      return 0;
    }
  }
  vtkNURBSUtils::VectorsToPointMatrix(outVecs, nrM0, ncM1, output);

  delete [] mat0Vec;
  for (int i=0; i<3; i++)
  {
    delete [] mat1Vecs[i];
    delete [] outVecs[i];
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::GetMatrixComp(vtkTypedArray<double> *mat,  const int loc, const int comp, const int matIsPoints, vtkTypedArray<double> *vec)
{
  int numVals = mat->GetExtents()[comp].GetSize();
  int check = mat->GetExtents()[2].GetSize();
  if (matIsPoints && check != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates\n");
    return 0;
  }

  if (matIsPoints)
  {
    vec->Resize(numVals, 3);
  }
  else
  {
    vec->Resize(numVals);
  }
  for (int i=0; i<numVals; i++)
  {
    double val[3];
    if (comp == 0)
    {
      if (matIsPoints)
      {
        for (int j=0; j<3; j++)
        {
          val[j] = mat->GetValue(i, loc, j);
        }
      }
      else
      {
        val[0] = mat->GetValue(i, loc);
      }
    }
    else if (comp == 1)
    {
      if (matIsPoints)
      {
        for (int j=0; j<3; j++)
        {
          val[j] = mat->GetValue(loc, i, j);
        }
      }
      else
      {
        val[0] = mat->GetValue(loc, i);
      }
    }
    if (matIsPoints)
    {
      for (int j=0; j<3; j++)
      {
        vec->SetValue(i, j, val[j]);
      }
    }
    else
    {
      vec->SetValue(i, val[0]);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::SetMatrixComp(vtkTypedArray<double> *vec,  const int loc, const int comp, const int matIsPoints, vtkTypedArray<double> *mat)
{
  int numVals = vec->GetExtents()[0].GetSize();
  int dim[3];
  int cSize = mat->GetExtents()[comp].GetSize();
  int check = mat->GetExtents()[2].GetSize();
  if (cSize != numVals)
  {
    fprintf(stderr,"Number of values in array and component of matrix are not equal\n");
    fprintf(stderr,"Size of Matrix in comp direction: %d\n", cSize);
    fprintf(stderr,"Size of Vector: %d\n", numVals);
    return 0;
  }
  if (matIsPoints && check != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates\n");
    return 0;
  }

  for (int i=0; i<numVals; i++)
  {
    double val[3];
    if (matIsPoints)
    {
      for (int j=0 ;j<3; j++)
      {
        val[j] = vec->GetValue(i, j);
      }
    }
    else
    {
      val[0] = vec->GetValue(i);
    }
    if (comp == 0)
    {
      if (matIsPoints)
      {
        for (int j=0; j<3; j++)
        {
          mat->SetValue(i, loc, j, val[j]);
        }
      }
      else
      {
        mat->SetValue(i, loc, val[0]);
      }
    }
    else if (comp == 1)
    {
      if (matIsPoints)
      {
        for (int j=0; j<3; j++)
        {
          mat->SetValue(loc, i, j, val[j]);
        }
      }
      else
      {
        mat->SetValue(loc, i, val[0]);
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::StructuredGridToTypedArray(vtkStructuredGrid *grid, vtkTypedArray<double> *output)
{
  int dim[3];
  grid->GetDimensions(dim);

  if (dim[2] != 1)
  {
    fprintf(stderr,"3 Dimensions are not yet supported\n");
    return 0;
  }

  //2D array with third dimensions the coordinates
  output->Resize(dim[0], dim[1], 3);

  for (int i=0; i<dim[0]; i++)
  {
    for (int j=0; j<dim[1]; j++)
    {
      int pos[3]; pos[2] =0;
      pos[0] = i;
      pos[1] = j;
      int ptId = vtkStructuredData::ComputePointId(dim, pos);
      double pt[3];

      grid->GetPoint(ptId, pt);
      for (int k=0; k<3; k++)
      {
        output->SetValue(i, j, k, pt[k]);
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PointsToTypedArray(vtkPoints *points, vtkTypedArray<double> *output)
{
  int numPoints = points->GetNumberOfPoints();

  //2D array with third dimensions the coordinates
  output->Resize(numPoints, 3);

  for (int i=0; i<numPoints; i++)
  {
    double pt[3];
    points->GetPoint(i, pt);
    for (int j=0; j<3; j++)
    {
      output->SetValue(i, j, pt[j]);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::DoubleArrayToTypedArray(vtkDoubleArray *input, vtkTypedArray<double> *output)
{
  int numVals  = input->GetNumberOfTuples();
  int numComps = input->GetNumberOfComponents();

  output->Resize(numVals, numComps);
  for (int i=0; i< numVals; i++)
  {
    for (int j=0; j< numComps; j++)
    {
      double val = input->GetComponent(i, j);
      output->SetValue(i, j, val);
    }
  }

  return 1;
}


//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::TypedArrayToPoints(vtkTypedArray<double> *array, vtkPoints *output)
{
  int numVals = array->GetExtents()[0].GetSize();

  output->SetNumberOfPoints(numVals);
  for (int i=0; i<numVals; i++)
  {
    double pt[3];
    for (int j=0; j<3; j++)
    {
      pt[j] = array->GetValue(i, j);
    }
    output->SetPoint(i, pt);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::TypedArrayToStructuredGrid(vtkTypedArray<double> *array, vtkStructuredGrid *output)
{
  int dims = array->GetDimensions();
  //2D array with third dimensions the coordinates
  int dim[3];
  for (int i=0; i<3; i++)
  {
    dim[i] = array->GetExtents()[i].GetSize();
  }

  if (dims > 3)
  {
    fprintf(stderr,"3 Dimensions are not yet supported\n");
    return 0;
  }
  if (dim[2] != 3)
  {
    fprintf(stderr,"Third dimension should have xyz coordinates\n");
    return 0;
  }

  output->SetDimensions(dim[0], dim[1], 1);
  output->GetPoints()->SetNumberOfPoints(dim[0]*dim[1]);

  for (int i=0; i<dim[0]; i++)
  {
    for (int j=0; j<dim[1]; j++)
    {
      int pos[3]; pos[2] =0;
      pos[0] = i;
      pos[1] = j;
      int ptId = vtkStructuredData::ComputePointId(dim, pos);
      double pt[3];
      for (int k=0; k<3; k++)
      {
        pt[k] = array->GetValue(i, j, k);
      }
      output->GetPoints()->SetPoint(ptId, pt);
    }
  }

    return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PolyDatasToStructuredGrid(vtkPolyData **inputs, const int numInputs, vtkStructuredGrid *points)
{
  int numPoints = inputs[0]->GetNumberOfPoints();
  for (int i=0; i<numInputs; i++)
  {
    if (numPoints != inputs[i]->GetNumberOfPoints())
    {
      fprintf(stderr,"Input segments do not have the same number of points, cannot loft\n");
      return 0;
    }
  }

  int dim[3];
  dim[0] = numInputs;
  dim[1] = numPoints;
  dim[2] = 1;

  vtkNew(vtkPoints, tmpPoints);
  tmpPoints->SetNumberOfPoints(numInputs*numPoints);
  for (int i=0; i<numInputs; i++)
  {
    for (int j=0; j<numPoints; j++)
    {
      int pos[3]; pos[0] = i; pos[1] = j; pos[2] = 0;
      int ptId = vtkStructuredData::ComputePointId(dim, pos);
      double pt[3];
      inputs[i]->GetPoint(j, pt);
      tmpPoints->SetPoint(ptId, pt);;
    }
  }
  points->SetPoints(tmpPoints);
  points->SetDimensions(dim);

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::Intersect1D(vtkIntArray *v0, vtkIntArray *v1, vtkIntArray *result)
{
  int numVals0 = v0->GetNumberOfTuples();
  int numVals1 = v1->GetNumberOfTuples();
  if (numVals0 != numVals1)
  {
    fprintf(stderr,"Cannot do accurate comparison! Vectors are different lengths\n");
    return 0;
  }
  result->SetNumberOfValues(numVals1);
  for (int i=0; i< numVals1; i++)
  {
    int val0 = v0->GetValue(i);
    int val1 = v1->GetValue(i);
    if (val0 && val1)
    {
      result->SetValue(i, 1);
    }
    else
    {
      result->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::Add1D(vtkDoubleArray *v0, vtkDoubleArray *v1, double scalar, vtkDoubleArray *result)
{
  int numVals0 = v0->GetNumberOfTuples();
  int numVals1 = v1->GetNumberOfTuples();
  if (numVals0 != numVals1)
  {
    fprintf(stderr,"Cannot do accurate comparison! Vectors are different lengths\n");
    return 0;
  }
  result->SetNumberOfValues(numVals1);
  for (int i=0; i< numVals1; i++)
  {
    double val0 = v0->GetValue(i);
    double val1 = v1->GetValue(i);
    result->SetTuple1(i, val0 + scalar*val1);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::AddVal1D(vtkDoubleArray *v0, double val, double scalar, vtkDoubleArray *result)
{
  int numVals = v0->GetNumberOfTuples();
  result->SetNumberOfValues(numVals);
  for (int i=0; i< numVals; i++)
  {
    double val0 = v0->GetTuple1(i);
    result->SetTuple1(i, val0 + scalar*val);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::AddVal1D(double val, vtkDoubleArray *v0, double scalar, vtkDoubleArray *result)
{
  int numVals = v0->GetNumberOfTuples();
  result->SetNumberOfValues(numVals);
  for (int i=0; i< numVals; i++)
  {
    double val0 = v0->GetTuple1(i);
    result->SetTuple1(i, val + scalar*val0);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MultiplyVal1D(vtkDoubleArray *v0, double val, vtkDoubleArray *result)
{
  int numVals = v0->GetNumberOfTuples();
  result->SetNumberOfValues(numVals);
  for (int i=0; i< numVals; i++)
  {
    double val0 = v0->GetTuple1(i);
    result->SetTuple1(i, val0 * val);
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::WhereGreaterEqual(double val, vtkDoubleArray *in, vtkIntArray *out)
{
  int numVals = in->GetNumberOfTuples();
  out->SetNumberOfTuples(numVals);

  for (int i=0; i<numVals; i++)
  {
    double compVal = in->GetTuple1(i);
    if (val >= compVal)
    {
      out->SetValue(i, 1);
    }
    else
    {
      out->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::WhereGreater(double val, vtkDoubleArray *in, vtkIntArray *out)
{
  int numVals = in->GetNumberOfTuples();
  out->SetNumberOfTuples(numVals);

  for (int i=0; i<numVals; i++)
  {
    double compVal = in->GetTuple1(i);
    if (val > compVal)
    {
      out->SetValue(i, 1);
    }
    else
    {
      out->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::WhereLessEqual(double val, vtkDoubleArray *in, vtkIntArray *out)
{
  int numVals = in->GetNumberOfTuples();
  out->SetNumberOfTuples(numVals);

  for (int i=0; i<numVals; i++)
  {
    double compVal = in->GetTuple1(i);
    if (val <= compVal)
    {
      out->SetValue(i, 1);
    }
    else
    {
      out->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::WhereLess(double val, vtkDoubleArray *in, vtkIntArray *out)
{
  int numVals = in->GetNumberOfTuples();
  out->SetNumberOfTuples(numVals);

  for (int i=0; i<numVals; i++)
  {
    double compVal = in->GetTuple1(i);
    if (val < compVal)
    {
      out->SetValue(i, 1);
    }
    else
    {
      out->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::WhereEqual(double val, vtkDoubleArray *in, vtkIntArray *out)
{
  int numVals = in->GetNumberOfTuples();
  out->SetNumberOfTuples(numVals);

  for (int i=0; i<numVals; i++)
  {
    double compVal = in->GetTuple1(i);
    if (val == compVal)
    {
      out->SetValue(i, 1);
    }
    else
    {
      out->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::WhereNotEqual(double val, vtkDoubleArray *in, vtkIntArray *out)
{
  int numVals = in->GetNumberOfTuples();
  out->SetNumberOfTuples(numVals);

  for (int i=0; i<numVals; i++)
  {
    double compVal = in->GetTuple1(i);
    if (val != compVal)
    {
      out->SetValue(i, 1);
    }
    else
    {
      out->SetValue(i, 0);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PrintArray(vtkIntArray *arr)
{
  int num = arr->GetNumberOfTuples();
  fprintf(stdout,"Array: %d tuples\n", num);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<num; i++)
  {
    fprintf(stdout,"%.4f ", arr->GetTuple1(i));
  }
  fprintf(stdout,"\n");
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PrintArray(vtkDoubleArray *arr)
{
  int num = arr->GetNumberOfTuples();
  fprintf(stdout,"Array: %d tuples\n", num);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<num; i++)
  {
    fprintf(stdout,"%.4f ", arr->GetTuple1(i));
  }
  fprintf(stdout,"\n");
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PrintVector(vtkTypedArray<double> *vec)
{
  int dims = vec->GetDimensions();
  int num = vec->GetExtents()[0].GetSize();
  fprintf(stdout,"Array: %d tuples\n", num);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<num; i++)
  {
    fprintf(stdout,"| ");
    if (dims > 1)
    {
      for (int j=0; j<3; j++)
      {
        fprintf(stdout,"%.4f ", vec->GetValue(i, j));
      }
    }
    else
    {
      fprintf(stdout,"%.4f ", vec->GetValue(i));
    }
    fprintf(stdout,"|");
  }
  fprintf(stdout,"\n");
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PrintMatrix(vtkTypedArray<double> *mat)
{
  int dims = mat->GetDimensions();
  int nr = mat->GetExtents()[0].GetSize();
  int nc = mat->GetExtents()[1].GetSize();
  fprintf(stdout,"Matrix: %d by %d\n", nr, nc);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<nr; i++)
  {
    for (int j=0; j<nc; j++)
    {
      fprintf(stdout,"| ");
      if (dims > 2)
      {
        for (int k=0; k<3; k++)
        {
          fprintf(stdout,"%.4f ", mat->GetValue(i, j, k));
        }
      }
      else
      {
        fprintf(stdout,"%.4f ", mat->GetValue(i, j));
      }
      fprintf(stdout,"|");
    }
    fprintf(stdout,"\n");
  }
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PrintStructuredGrid(vtkStructuredGrid *mat)
{
  int dim[3];
  mat->GetDimensions(dim);
  fprintf(stdout,"Matrix: %d by %d\n", dim[0], dim[1]);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<dim[0]; i++)
  {
    for (int j=0; j<dim[1]; j++)
    {
      int pos[3]; pos[0] = i; pos[1] = j; pos[2] = 0;
      int ptId = vtkStructuredData::ComputePointId(dim, pos);
      double pt[3];
      mat->GetPoint(ptId, pt);
      fprintf(stdout,"| %.4f %.4f %.4f |", pt[0], pt[1], pt[2]);
    }
    fprintf(stdout,"\n");
  }
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PrintPoints(vtkPoints *points)
{
  int np = points->GetNumberOfPoints();
  fprintf(stdout,"Points: %d points\n", np);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<np; i++)
  {
    double pt[3];
    points->GetPoint(i, pt);
    fprintf(stdout,"Pt %d: ", i);
    for (int j=0; j<3; j++)
    {
      fprintf(stdout,"%.4f ",pt[j]);
    }
    fprintf(stdout,"\n");
  }
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::StructuredGridTranspose(vtkStructuredGrid *sg, vtkStructuredGrid *newSg)
{
  int dim[3];
  sg->GetDimensions(dim);
  newSg->SetDimensions(dim[1], dim[0], 1);
  vtkNew(vtkPoints, tmpPoints);
  tmpPoints->SetNumberOfPoints(sg->GetNumberOfPoints());


  for (int i=0; i<dim[0]; i++)
  {
    for (int j=0; j<dim[1]; j++)
    {
      int pos[3]; pos[2] = 0;
      pos[0] = i;
      pos[1] = j;
      int ptId = vtkStructuredData::ComputePointId(dim, pos);
      double pt[3];
      sg->GetPoint(ptId, pt);
      pos[0] = j;
      pos[1] = i;
      int newPtId = vtkStructuredData::ComputePointId(dim, pos);
      tmpPoints->SetPoint(newPtId, pt);
    }
  }

  newSg->SetPoints(tmpPoints);

  int newDims[3];
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixTranspose(vtkTypedArray<double> *mat, const int matIsPoints, vtkTypedArray<double> *newMat)
{
  int nr = mat->GetExtents()[0].GetSize();
  int nc = mat->GetExtents()[1].GetSize();
  int np = mat->GetExtents()[2].GetSize();
  if (matIsPoints && np != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  if (matIsPoints)
  {
    newMat->Resize(nc, nr, 3);
  }
  else
  {
    newMat->Resize(nc, nr);
  }

  for (int i=0; i<nr; i++)
  {
    for (int j=0; j<nc; j++)
    {
      if (matIsPoints)
      {
        for (int k=0; k<3; k++)
        {
          double val = mat->GetValue(i, j, k);
          newMat->SetValue(j, i, k, val);
        }
      }
      else
      {
        double val = mat->GetValue(i, j);
        newMat->SetValue(j, i, val);
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::MatrixToVector(vtkTypedArray<double> *mat, double *matVec)
{
  int nr = mat->GetExtents()[0].GetSize();
  int nc = mat->GetExtents()[1].GetSize();

  for (int i=0; i<nc; i++)
  {
    for (int j=0; j<nr; j++)
    {
      matVec[i*nr+j] = mat->GetValue(j, i);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::VectorToMatrix(double *matVec, const int nr, const int nc, vtkTypedArray<double> *mat)
{
  mat->Resize(nr, nc);

  for (int i=0; i<nc; i++)
  {
    for (int j=0; j<nr; j++)
    {
      double val = matVec[i*nr+j];
      mat->SetValue(j, i, val);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::PointMatrixToVectors(vtkTypedArray<double> *mat, double *matVecs[3])
{
  int nr = mat->GetExtents()[0].GetSize();
  int nc = mat->GetExtents()[1].GetSize();
  int np = mat->GetExtents()[2].GetSize();
  if (np != 3)
  {
    fprintf(stderr,"Third dimension of matrix should contain xyz coordinates, but doesn't!\n");
    return 0;
  }

  for (int i=0; i<nc; i++)
  {
    for (int j=0; j<nr; j++)
    {
      for (int k=0; k<3; k++)
      {
        matVecs[k][i*nr+j] = mat->GetValue(j, i, k);
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::VectorsToPointMatrix(double *matVecs[3], const int nr, const int nc, vtkTypedArray<double> *mat)
{
  for (int i=0; i<nc; i++)
  {
    for (int j=0; j<nr; j++)
    {
      for (int k=0; k<3; k++)
      {
        double val = matVecs[k][i*nr+j];
        mat->SetValue(j, i, k, val);
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::DGEMM(const double *A, const int nrA, const int ncA,
                         const double *B, const int nrB, const int ncB,
                         double *C)
{
  if (ncA != nrB)
  {
    fprintf(stderr,"Matrix dims do not match, cannot perform operation\n");
    return 0;
  }
  for (int i=0; i<ncB; i++)
  {
    for (int j=0; j<nrA; j++)
    {
      C[i+ncB*j] = 0.0;
    }
  }

  //fprintf(stdout,"A Mat Dims: %d %d\n", nrA, ncA);
  //fprintf(stdout,"B Mat Dims: %d %d\n", nrB, ncB);
  /* For each row i of A */
  for (int i = 0; i < nrA; ++i)
  {
  /* For each column j of B */
    for (int j = 0; j < ncB; ++j)
    {
           /* Compute C(i,j) */
      double cij = C[i+j*nrA];
      for(int k = 0; k < ncA; k++)
      {
        //fprintf(stdout,"A[%d] + B[%d] + ",i+k*nrA, k+j*nrB);
        cij += A[i+k*nrA] * B[k+j*nrB];
      }
      //fprintf(stdout," 0 -> C[%d]\n",i+j*nrA);
      C[i+j*nrA] = cij;
    }
  }
  //fprintf(stdout,"End\n");

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkNURBSUtils::Print2DArray(const double *arr, const int nr, const int nc)
{
  fprintf(stdout,"Matrix: %d by %d\n", nr, nc);
  fprintf(stdout,"----------------------------------------------------------\n");
  for (int i=0; i<nr*nc; i++)
  {
    fprintf(stdout,"| %.4f |\n", arr[i]);
  }
  fprintf(stdout,"----------------------------------------------------------\n");

  return 1;
}