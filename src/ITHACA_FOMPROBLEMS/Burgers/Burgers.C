/*---------------------------------------------------------------------------*\
     ██╗████████╗██╗  ██╗ █████╗  ██████╗ █████╗       ███████╗██╗   ██╗
     ██║╚══██╔══╝██║  ██║██╔══██╗██╔════╝██╔══██╗      ██╔════╝██║   ██║
     ██║   ██║   ███████║███████║██║     ███████║█████╗█████╗  ██║   ██║
     ██║   ██║   ██╔══██║██╔══██║██║     ██╔══██║╚════╝██╔══╝  ╚██╗ ██╔╝
     ██║   ██║   ██║  ██║██║  ██║╚██████╗██║  ██║      ██║      ╚████╔╝
     ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝      ╚═╝       ╚═══╝

 * In real Time Highly Advanced Computational Applications for Finite Volumes
 * Copyright (C) 2017 by the ITHACA-FV authors
-------------------------------------------------------------------------------

  License
  This file is part of ITHACA-FV

  ITHACA-FV is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ITHACA-FV is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with ITHACA-FV. If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

/// \file
/// Source file of the Burgers class.

#include "Burgers.H"

// * * * * * * * * * * * * * * * Constructors * * * * * * * * * * * * * * * * //
// Constructor
Burgers::Burgers() {}

Burgers::Burgers(int argc, char* argv[])
    :
    UnsteadyProblem()
{
    _args = autoPtr<argList>
            (
                new argList(argc, argv)
            );

    if (!_args->checkRootCase())
    {
        Foam::FatalError.exit();
    }

    argList& args = _args();
#include "createTime.H"
#include "createMesh.H"
    _simple = autoPtr<simpleControl>
              (
                  new simpleControl
                  (
                      mesh
                  )
              );
    simpleControl& simple = _simple();
#include "createFields.H"
#include "createFvOptions.H"
    ITHACAdict = new IOdictionary
    (
        IOobject
        (
            "ITHACAdict",
            runTime.system(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    );
    para = ITHACAparameters::getInstance(mesh, runTime);
    offline = ITHACAutilities::check_off();
    podex = ITHACAutilities::check_pod();
    setTimes(runTime);
}

void Burgers::truthSolve(word folder)
{
    Time& runTime = _runTime();
    fvMesh& mesh = _mesh();
    volVectorField& U = _U();
    surfaceScalarField& phi = _phi();
    fv::options& fvOptions = _fvOptions();
    simpleControl& simple = _simple();
    dimensionedScalar& nu = _nu();
    counter = 1;
    ITHACAstream::exportSolution(U, name(counter), folder + name(folderN));
    counter++;
    nextWrite = startTime;
    nextWrite += writeEvery;

    while (_simple().loop())
    {
        auto start = std::chrono::system_clock::now();
        Info << "Time = " << _runTime().timeName() << nl << endl;

        while (simple.correctNonOrthogonal())
        {
            solve
            (
                fvm::ddt(U)
                + fvm::div(phi, U)
                - fvm::laplacian(nu, U)
            );
        }

        phi = linearInterpolate(U) & mesh.Sf();
        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>
                       (end - start);
        Info << "Elapsed: " << elapsed.count() << " microseconds\n";

        if (checkWrite(runTime))
        {
            ITHACAstream::exportSolution(U, name(counter), folder + name(folderN));
            counter++;
            Ufield.append(U.clone());
            nextWrite += writeEvery;
        }
    }

    folderN++;
}

void Burgers::restart()
{
    _U.clear();
    _phi.clear();
    _fvOptions.clear();
    _nu.clear();
    _transportProperties.clear();
    argList& args = _args();
    Time& runTime = _runTime();
    runTime.setTime(0, 1);
    Foam::fvMesh& mesh = _mesh();
#include "createFields.H"
#include "createFvOptions.H"
}
