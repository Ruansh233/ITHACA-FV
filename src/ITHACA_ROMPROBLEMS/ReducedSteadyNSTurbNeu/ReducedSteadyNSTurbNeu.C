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

#include "ReducedSteadyNSTurbNeu.H"

// * * * * * * * * * * * * * * * Constructors * * * * * * * * * * * * * * * * //

// Constructor
ReducedSteadyNSTurbNeu::ReducedSteadyNSTurbNeu()
{
}

ReducedSteadyNSTurbNeu::ReducedSteadyNSTurbNeu(SteadyNSTurbNeu& fomProblem)
    :
    problem(& fomProblem)
{
    N_BC = problem->inletIndex.rows();
    Nphi_u = problem->B_matrix.rows();
    Nphi_p = problem->K_matrix.cols();
    nphiNut = problem->cTotalTensor.dimension(1);

    for (int k = 0; k < problem->liftfield.size(); k++)
    {
        Umodes.append((problem->liftfield[k]).clone());
    }

    for (int k = 0; k < problem->NUmodes; k++)
    {
        Umodes.append((problem->Umodes[k]).clone());
    }

    for (int k = 0; k < problem->NSUPmodes; k++)
    {
        Umodes.append((problem->supmodes[k]).clone());
    }

    newtonObjectSUP = newtonSteadyNSTurbNeuSUP(Nphi_u + Nphi_p, Nphi_u + Nphi_p,
                                            fomProblem);
}

int newtonSteadyNSTurbNeuSUP::operator()(const Eigen::VectorXd& x,
                                      Eigen::VectorXd& fvec) const
{
    Eigen::VectorXd aTmp(Nphi_u);
    Eigen::VectorXd bTmp(Nphi_p);
    aTmp = x.head(Nphi_u);
    bTmp = x.tail(Nphi_p);
    // Convective term
    Eigen::MatrixXd cc(1, 1);
    // Eddy Diffusion term
    Eigen::MatrixXd ee(1, 1);
    // Mom Term
    Eigen::VectorXd m1 = problem->B_matrix * aTmp * nu;
    // Diffusion Term
    Eigen::VectorXd m1_sym = problem->B_matrix_sym * aTmp * nu;
    // Gradient of pressure
    Eigen::VectorXd m2 = problem->K_matrix * bTmp;
    // Pressure Term
    Eigen::VectorXd m3 = problem->P_matrix * aTmp;
    // Penalty term
    Eigen::MatrixXd penaltyU = Eigen::MatrixXd::Zero(Nphi_u, N_BC);
    // Penalty term
    Eigen::MatrixXd penaltyGradU (Nphi_u, 1);
    // Neumann boundary term
    Eigen::MatrixXd neuTerm1(1, 1);
    Eigen::MatrixXd neuTerm2(1, 1);    

    // Term for penalty method
    if (problem->bcMethod == "penalty")
    {
        for (int l = 0; l < N_BC; l++)
        {
            penaltyU.col(l) = bc(l) * problem->bcVelVec[l] - problem->bcVelMat[l] *
                              aTmp;
        }
    }

    // Term for penalty of the Neumann boundary condition
    if (problem->neumannMethod == "penalty")
    {
        penaltyGradU = problem->bcGradVelVec[0] * NeuBC -
                       problem->bcGradVelMat[0] * aTmp;
    }

    if (problem->viscCoeff == "L2")
    {
    }
    else if (problem->viscCoeff == "RBF")
    {
        if (problem->rbfParams == "params")
        {            
        }
        else if (problem->rbfParams == "vel")
        {
            for (int i = 0; i < nphiNut; i++)
            {
                Eigen::MatrixXd coeffL2_tmp = aTmp.middleRows(problem->liftfield.size(), problem->NUmodes);

                gNut(i) = problem->rbfSplines[i]->eval(coeffL2_tmp);
            }
        }
        else if (problem->rbfParams == "velLift")
        {
            for (int i = 0; i < nphiNut; i++)
            {
                Eigen::MatrixXd coeffL2_tmp = aTmp.topRows(problem->liftfield.size() + problem->NUmodes);

                gNut(i) = problem->rbfSplines[i]->eval(coeffL2_tmp);
            }
        }
        else
        {
            FatalError << "Unknown rbfParams type: " << problem->rbfParams << endl;
            FatalError.exit();
        }
    }

    for (int i = 0; i < Nphi_u; i++)
    {
        cc = aTmp.transpose() * Eigen::SliceFromTensor(problem->C_tensor, 0,
             i) * aTmp;
        ee = gNut.transpose() * Eigen::SliceFromTensor(problem->cTotalTensor, 0, 
             i) * aTmp;

        if (problem->bcMethod == "penalty")
        {
            fvec(i) += ((penaltyU * tauU)(i, 0));
        }

        if (problem->neumannMethod == "penalty")
        {
            fvec(i) = m1(i) - cc(0,0) + ee(0,0) - m2(i);
            fvec(i) += penaltyGradU (i, 0) * tauGradU(0, 0);
        }
        else if (problem->neumannMethod == "NeuTerm")
        {
            neuTerm1 = problem->bc1_B_matrix_sym.row(i) * NeuBC * nu;
            neuTerm2 = problem->bc2_B_matrix_sym.row(i) * aTmp * nu;
            fvec(i) = - m1_sym(i) - cc(0,0) + ee(0,0) - m2(i) + neuTerm1(0, 0) + neuTerm2(0, 0);
        }
        else if (problem->neumannMethod == "none")
        {
            fvec(i) = m1(i) - cc(0,0) + ee(0,0) - m2(i);
        }
    }

    for (int j = 0; j < Nphi_p; j++)
    {
        int k = j + Nphi_u;
        fvec(k) = m3(j);
    }

    if (problem->bcMethod == "lift")
    {
        for (int j = 0; j < N_BC; j++)
        {
            fvec(j) = x(j) - bc(j);
        }
    }

    return 0;
}

int newtonSteadyNSTurbNeuSUP::df(const Eigen::VectorXd& x,
                              Eigen::MatrixXd& fjac) const
{
    Eigen::NumericalDiff<newtonSteadyNSTurbNeuSUP> numDiff(* this);
    numDiff.df(x, fjac);
    return 0;
}

// * * * * * * * * * * * * * * * Solve Functions  * * * * * * * * * * * * * //

void ReducedSteadyNSTurbNeu::solveOnlineSUP(Eigen::MatrixXd vel, Eigen::VectorXd neuVal)
{    
    if (problem->bcMethod == "lift")
    {
        if (problem->nonUniformbc)
        {
            vel_now = setOnlineVelocity(vel, true);
        }
        else
        {
            vel_now = setOnlineVelocity(vel);
        }
    }
    else if (problem->bcMethod == "penalty")
    {
        vel_now = vel;
    }

    y.resize(Nphi_u + Nphi_p, 1);
    y.setZero();

    // Change initial condition for the lifting function
    if (problem->bcMethod == "lift")
    {
        for (int j = 0; j < N_BC; j++)
        {
            y(j) = vel_now(j, 0);
        }
    }

    Color::Modifier red(Color::FG_RED);
    Color::Modifier green(Color::FG_GREEN);
    Color::Modifier def(Color::FG_DEFAULT);
    Eigen::HybridNonLinearSolver<newtonSteadyNSTurbNeuSUP> hnls(newtonObjectSUP);
    newtonObjectSUP.bc.resize(N_BC);
    newtonObjectSUP.tauU = tauU;
    newtonObjectSUP.tauGradU = tauGradU;

    for (int j = 0; j < N_BC; j++)
    {
        newtonObjectSUP.bc(j) = vel_now(j, 0);
    }

    newtonObjectSUP.NeuBC = neuVal;

    if (problem->viscCoeff == "L2")
    {
        for (int i = 0; i < nphiNut; i++)
        {
            newtonObjectSUP.gNut = problem->nutCoeff;
        }
    }
    else if (problem->viscCoeff == "RBF")
    {
        if (problem->rbfParams == "params")
        {            
            label caseIdx = count_online_solve-1;
            std::cout << "The rbfparameter is: " << onlineMu.row(caseIdx) << std::endl;
            for (int i = 0; i < nphiNut; i++)
            {                
                newtonObjectSUP.gNut(i) = problem->rbfSplines[i]->eval(onlineMu.row(caseIdx));
            }
            rbfCoeff = newtonObjectSUP.gNut;
        }
    }
    else
    {
        Info << "The way to compute the eddy viscosity coefficients has to be either L2 or RBF"
             << endl;
        exit(0);
    }

    newtonObjectSUP.nu = nu;
    hnls.solve(y);
    Eigen::VectorXd res(y);
    newtonObjectSUP.operator()(y, res);
    std::cout << "################## Online solve N° " << count_online_solve <<
              " ##################" << std::endl;

    if (res.norm() < 1e-5)
    {
        std::cout << green << "|F(x)| = " << res.norm() << " - Minimun reached in " <<
                  hnls.iter << " iterations " << def << std::endl << std::endl;
    }
    else
    {
        std::cout << red << "|F(x)| = " << res.norm() << " - Minimun reached in " <<
                  hnls.iter << " iterations " << def << std::endl << std::endl;
    }

    count_online_solve += 1;

    if (problem->rbfParams == "vel" || problem->rbfParams == "velLift")
    {
        rbfCoeff = newtonObjectSUP.gNut;
    }
}

void ReducedSteadyNSTurbNeu::reconstruct(bool exportFields, fileName folder,
                                      int printevery)
{
    if (exportFields)
    {
        mkDir(folder);
        ITHACAutilities::createSymLink(folder);
    }

    int counter = 0;
    int nextWrite = 0;
    List <Eigen::MatrixXd> CoeffU;
    List <Eigen::MatrixXd> CoeffP;
    List <Eigen::MatrixXd> CoeffNut;
    CoeffU.resize(0);
    CoeffP.resize(0);
    CoeffNut.resize(0);

    for (int i = 0; i < online_solution.size(); i++)
    {
        if (counter == nextWrite)
        {
            Eigen::MatrixXd currentUCoeff;
            Eigen::MatrixXd currentPCoeff;
            Eigen::MatrixXd currentNutCoeff;
            currentUCoeff = online_solution[i].block(1, 0, Nphi_u, 1);
            currentPCoeff = online_solution[i].bottomRows(Nphi_p);
            currentNutCoeff = rbfCoeffMat.block(0, i, nphiNut, 1);
            CoeffU.append(currentUCoeff);
            CoeffP.append(currentPCoeff);
            CoeffNut.append(currentNutCoeff);
            nextWrite += printevery;
        }

        counter++;
    }

    volVectorField uRec("uRec", Umodes[0]);
    volScalarField pRec("pRec", problem->Pmodes[0]);
    volScalarField nutRec("nutRec", problem->nutModes[0]);
    uRecFields = problem->L_U_SUPmodes.reconstruct(uRec, CoeffU, "uRec");
    pRecFields = problem->Pmodes.reconstruct(pRec, CoeffP, "pRec");
    nutRecFields = problem->nutModes.reconstruct(nutRec, CoeffNut, "nutRec");

    if (exportFields)
    {
        ITHACAstream::exportFields(uRecFields, folder,
                                   "uRec");
        ITHACAstream::exportFields(pRecFields, folder,
                                   "pRec");
        ITHACAstream::exportFields(nutRecFields, folder,
                                   "nutRec");
    }
}

Eigen::MatrixXd ReducedSteadyNSTurbNeu::setOnlineVelocity(Eigen::MatrixXd vel)
{
    assert(problem->inletIndex.rows() == vel.rows()
           && "Imposed boundary conditions dimensions do not match given values matrix dimensions");
    Eigen::MatrixXd vel_scal;
    vel_scal.resize(vel.rows(), vel.cols());

    for (int k = 0; k < problem->inletIndex.rows(); k++)
    {
        int p = problem->inletIndex(k, 0);
        int l = problem->inletIndex(k, 1);
        scalar area = gSum(problem->liftfield[0].mesh().magSf().boundaryField()[p]);
        scalar u_lf = gSum(problem->liftfield[k].mesh().magSf().boundaryField()[p] *
                           problem->liftfield[k].boundaryField()[p]).component(l) / area;
        vel_scal(k, 0) = vel(k, 0) / u_lf;
    }

    return vel_scal;
}

Eigen::MatrixXd ReducedSteadyNSTurbNeu::setOnlineVelocity(Eigen::MatrixXd vel, bool nonUniform)
{
    assert(problem->inletIndex.rows() == vel.rows()
           && "Imposed boundary conditions dimensions do not match given values matrix dimensions");
    Eigen::MatrixXd vel_scal(vel);

    return vel_scal;
}

// ************************************************************************* //

