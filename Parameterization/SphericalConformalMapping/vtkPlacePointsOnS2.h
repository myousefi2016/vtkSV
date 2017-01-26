/*=========================================================================
 *
 * Copyright (c) 2014 The Regents of the University of California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *=========================================================================*/


/** @file vtkPlacePointsOnS2.h
 *  @brief This is a vtk filter to map a triangulated surface to a sphere.
 *  @details This filter uses the heat flow method to map a triangulated
 *  surface to a sphere. The first step is to compute the Tutte Energy, and
 *  the second step is to perform the conformal map. For more details, see
 *  Gu et al., Genus Zero Surface Conformal Mapping and Its
 *  Application to Brain Surface Mapping, 2004.
 *
 *  @author Adam Updegrove
 *  @author updega2@gmail.com
 *  @author UC Berkeley
 *  @author shaddenlab.berkeley.edu
 */

#ifndef vtkPlacePointsOnS2_h
#define vtkPlacePointsOnS2_h

#include "vtkPolyDataAlgorithm.h"

#include "vtkEdgeTable.h"
#include "vtkFloatArray.h"
#include "vtkMatrix4x4.h"
#include "vtkPolyData.h"

class vtkPlacePointsOnS2 : public vtkPolyDataAlgorithm
{
public:
  static vtkPlacePointsOnS2* New();
  //vtkTypeRevisionMacro(vtkPlacePointsOnS2, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Print statements used for debugging
  vtkGetMacro(Verbose, int);
  vtkSetMacro(Verbose, int);

  // Description:
  // Macro to set/get the axis that the object aligns with in order to orient
  // the object with a unit cube
  vtkSetVector3Macro(ZAxis, double);
  vtkGetVector3Macro(ZAxis, double);
  vtkSetVector3Macro(XAxis, double);
  vtkGetVector3Macro(XAxis, double);
  vtkSetMacro(UseCustomAxisAlign, int);
  vtkGetMacro(UseCustomAxisAlign, int);


  static int ComputeMassCenter(vtkPolyData *pd, double massCenter[3]);
  static int GetRotationMatrix(double vec0[3], double vec1[3], vtkMatrix4x4 *rotMatrix);
  static int ApplyRotationMatrix(vtkPolyData *pd, vtkMatrix4x4 *rotMatrix);

protected:
  vtkPlacePointsOnS2();
  ~vtkPlacePointsOnS2();

  // Usual data generation method
  int RequestData(vtkInformation *vtkNotUsed(request),
		  vtkInformationVector **inputVector,
		  vtkInformationVector *outputVector);

  int MoveToOrigin();
  int RotateToCubeCenterAxis();
  int ScaleToUnitCube();
  int DumbMapToSphere();
  int TextureMap();
  int ConvertTextureFieldToPolyData();

private:
  vtkPlacePointsOnS2(const vtkPlacePointsOnS2&);  // Not implemented.
  void operator=(const vtkPlacePointsOnS2&);  // Not implemented.

  int Verbose;

  vtkPolyData *InitialPd;
  vtkPolyData *FinalPd;

  int UseCustomAxisAlign;
  double ZAxis[3];
  double XAxis[3];
};

#endif
