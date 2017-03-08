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

/** @file HausdorffDistance.cxx
 *  @brief This implements the vtkSVHausdorffDistance filter as a class
 *
 *  @author Adam Updegrove
 *  @author updega2@gmail.com
 *  @author UC Berkeley
 *  @author shaddenlab.berkeley.edu
 */

#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkDataArray.h"
#include "vtkDataWriter.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSVHausdorffDistance.h"
#include "vtkSVGlobals.h"
#include "vtkSVIOUtils.h"

#include <string>
#include <sstream>
#include <iostream>

int main(int argc, char *argv[])
{
  /* BEGIN PROCESSING COMMAND-LINE ARGUMENTS */
  /* Assume the command line is okay */
  bool BogusCmdLine = false;
  /* Assume no options specified at command line */
  bool RequestedHelp = false;
  bool SourceProvided = false;
  bool TargetProvided = false;
  bool OutputProvided = false;

  /* variables used in processing the commandline */
  int iarg, arglength;
  std::string tmpstr;

  //Filenames
  std::string sourceFileName;
  std::string targetFileName;
  std::string outputFileName;

  /* argc is the number of strings on the command-line */
  /*  starting with the program name */
  for(iarg=1; iarg<argc; iarg++){
      arglength = strlen(argv[iarg]);
      /* replace 0..arglength-1 with argv[iarg] */
      tmpstr.replace(0,arglength,argv[iarg],0,arglength);
      if(tmpstr=="-h"){
          RequestedHelp = true;
      }
      else if(tmpstr=="-source"){
          SourceProvided = true;
          iarg++;
          sourceFileName = argv[iarg];
      }
      else if(tmpstr=="-target"){
          TargetProvided = true;
          iarg++;
          targetFileName = argv[iarg];
      }
      else if(tmpstr=="-output"){
          OutputProvided = true;
          iarg++;
          outputFileName = argv[iarg];
      }
      else {
          BogusCmdLine = true;
      }
      /* reset tmpstr for next argument */
      tmpstr.erase(0,arglength);
  }

  if (RequestedHelp || !SourceProvided || !TargetProvided)
  {
    cout << endl;
    cout << "usage:" <<endl;
    cout << "  HausdorffDistance -source [Source Surface] -target [Target Surface]" << endl;
    cout << endl;
    cout << "COMMAND-LINE ARGUMENT SUMMARY" << endl;
    cout << "  -h                  : Display usage and command-line argument summary"<< endl;
    cout << "  -source             : Source file name (.vtp or .stl)"<< endl;
    cout << "  -target             : Target file name (.vtp or .stl)"<< endl;
    cout << "  -ouptut             : Output file name"<< endl;
    cout << "END COMMAND-LINE ARGUMENT SUMMARY" << endl;
    return EXIT_FAILURE;
  }
  if (!OutputProvided)
  {
    cout << "Setting output name based on the source and target filenames" <<endl;
    std::string newDirName = vtkSVIOUtils::GetPath(targetFileName)+"/"+vtkSVIOUtils::GetRawName(targetFileName);
    //Only mac and linux!!!
    system(("mkdir -p "+newDirName).c_str());
    outputFileName = vtkSVIOUtils::GetPath(targetFileName)+"/"+vtkSVIOUtils::GetRawName(targetFileName)+"/"+vtkSVIOUtils::GetRawName(targetFileName)+"_Distanced.vtp";
  }

  //Call Function to Read File
  std::cout<<"Reading Files..."<<endl;
  vtkNew(vtkPolyData, sourcePd);
  vtkSVIOUtils::ReadInputFile(sourceFileName,sourcePd);
  vtkNew(vtkPolyData, targetPd);
  vtkSVIOUtils::ReadInputFile(targetFileName,targetPd);

  //Filter
  vtkNew(vtkSVHausdorffDistance, Distancer);

  //OPERATION
  std::cout<<"Performing Operation..."<<endl;
  Distancer->SetInputData(0, sourcePd);
  Distancer->SetInputData(1, targetPd);
  Distancer->SetDistanceArrayName("Distance");
  Distancer->Update();

  std::cout<<"Hausdorff Distance: "<<Distancer->GetHausdorffDistance()<<endl;
  std::cout<<"Average Distance:   "<<Distancer->GetAverageDistance()<<endl;
  //Write Files
  std::cout<<"Writing Files..."<<endl;
  vtkSVIOUtils::WriteVTPFile(outputFileName,Distancer->GetOutput(0));
  std::cout<<"Done"<<endl;

  //Exit the program without errors
  return EXIT_SUCCESS;
}